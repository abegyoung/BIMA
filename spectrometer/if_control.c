/*******************************************************************
 *                                                                 *
 *                            if_control.c                       *
 *                                                                 *
 *******************************************************************
 *
 * This program takes total power from the BIMA IF and passes it to the stuffer program.
 * Commands are received through the NEWDBE socket, rather than from the keyboard.
 *
 * Revision history
   12Oct2016 DF  Copied from ~cactus/spectral/ffb/utils/fbmon.c
   03Feb2017 DF  Forked from arows-mon.c
   08Feb2017 DF  Changed to 0.9 to 2.1 GHz IF input
   09Oct2017 DF  Changed socket names, adding commands per Tom 
   18Mar2026 AY  Copied from ~/cactus/spectral/aro/utils/arows_control.c
   02Apr2026 AY  recycled from ibob_control with added CAN or SORAL TCP
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>         
#include <time.h>         
#include <unistd.h>       
#include <sys/types.h>    
#include <sys/stat.h>     
#include <errno.h>        
#include <stddef.h>       
#include <stdarg.h>       

#include <caclib_proto.h>

#ifndef PROCESS_DBEDBEFAKEDBE
#define PROCESS_DBEDBEFAKEDBE 0
#endif

#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>


// mboxdef
struct SOCK *client;		// MBOX socket to send data to CATCHER

// Globals
int ret;
int running = 0;		// whether we are sending data out
int phase = 0;			// current phase set

union {
    float f;
    unsigned char b[4];
} u;

/* Need to declare start_time and end_time because timespec is 64 bit here */
/* Define here instead */

#define SPEC_CONFIG_MAGIC   "SPECCONF"
#define MAGIC_SIZE                 16
#define MAX_SPEC_CHANS          32768
#define MAX_PARTS                   4

struct SPEC_DATA_SUB_SET {
  char   magic[MAGIC_SIZE];     // fill with SPEC_CONFIG_MAGIC (16 bytes)
  int    which_spec;            // Which spec this data belongs to.
  int    nparts;                // number of spectra for this spectrometer
  int    nchans;                // number of channels in each spectrum
  int    depth;                 // Number of seperate spectra. (for OTF / DBE?)
  int    bin;                   // bin 0 for sig, 1 for ref, 2 trash
  int    walsh;                 // walsh function value for this dataset
  int    int_time;              // integration time in microseconds
  int    error_bits;            // zero for good, error bits defined in header
  int    mode;                  // Observing modes: SIG, HOT, SKY etc
  float  obsTime;               // Mid-Time for observation (OTF Mapping)
  float  raoff;                 // Right Assension Offset for OTF Mapping;
  float  decoff;                // Declination Offset for OTF Mapping;
  float  azoff;                 // Azinuth Offset for OTF Mapping;
  float  eloff;                 // Elevation Offset for OTF Mapping;
  int    start_time[2];         // when integration started (2 longs)
  int    end_time[2];           // when integration ended   (2 longs)
}; /* 16 + (14 * 4 ) + (2 * 8) = 88 bytes in size */

struct SPEC_DATA_SET {
        struct SPEC_DATA_SUB_SET sdss;
        int data[MAX_SPEC_CHANS];       // raw spectral data array
};


struct SPEC_CONFIG {
  char magic[MAGIC_SIZE];       // fill with SPEC_CONFIG_MAGIC
  int spec_enable;              // This backend is usable.
  int data_enable;              // Data acq enable flags for each backend
  int which_spec;               // Which spec this config belongs to.
  int nparts;                   // number of spectra for this spectrometer
  int valid_mask;               // SAMbus blanking mask
  int valid_bits;               // SAMbus blanking valid bits
  int mode;                     // Backend Mode; i.e. AROWS mode; FBS and MAC probably won't use.
  int image_offset;             // Obsolete!image offset(kHz) for 4-IF backends
  int doppler_khz[MAX_PARTS];   // doppler shift of alt. sideband per FFB part
  int offset_khz[MAX_PARTS];    // manual offset added for each FFB part
  double res;                   // backend resolutions in MHz
  double center;                // and center channels in MHz from sky LO
};

#define NPOINTS         1024	// FFT size
#define N_USABLE_POINTS (6400)
#define NPARTS          1
#define WHICH_SPEC      0

#define MAX_SOCK 1 + sizeof(struct SPEC_DATA_SET)  /* Biggest sock msg */

struct SPEC_DATA_SET outgoing;   // for catcher to catch via sock write

struct timespec start_time, end_time;   // one iteration

char msgbuf[MAX_SOCK];
struct SPEC_CONFIG cfg;		// local spectrometer config 

char stufBuf[256];
double ch1, ch2, ch3, ch4;
int indx = 0;

int do_config()
{
  //log_msg("DO_CONFIG Got config - Starting configuration of IBOB");

  sock_send(client, "done config");

  return 0;
}


// Do all steps necessary to initialize CAN and start watching for IFTOTPOW
int do_initialize()
{
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    // Open CAN raw socket
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        perror("Socket");
        return 1;
    }

    // Set interface name (can0)
    strcpy(ifr.ifr_name, "can0");
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        close(s);
        return 1;
    }

    // Bind socket to the CAN interface
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        close(s);
        return 1;
    }
    int found=0;
    // Continuously receive CAN messages
    while (!found) {
        int nbytes = read(s, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            perror("Read");
            break;
        }

        // Print the sender ID and data payload
        //printf("Sender ID: 0x%X | Data: ", (frame.can_id) & 0x1FFFFFFF);

        unsigned modType = (frame.can_id >>  9) & 0xFF;
        unsigned msgType = (frame.can_id >> 17) & 0x3FF;
        if(modType == 224){
            switch(msgType){
                case 0x0E0: {
                    u.b[0] = frame.data[3]; u.b[1] = frame.data[2]; u.b[2] = frame.data[1]; u.b[3] = frame.data[0]; //IFTOTPOW
                    printf("IFTOTPOW %.3f mW\n",u.f);
		    found=1;
                }
            }
        }
    }
    close(s);

    return 0;
}


/* Start command */
int do_start()
{

  //sock_send(client, "done start\n");

  running = 1;

  return 0;
}

/* Stop command */
int do_stop()
{

  //sock_send(client, "done stop\n");

  running = 0;
  
  return 0;
}



// send a dwiline of total power data to stuffer
int send_data()
{
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    // Open CAN raw socket
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        perror("Socket");
        return 1;
    }

    // Set interface name (can0)
    strcpy(ifr.ifr_name, "can0");
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        close(s);
        return 1;
    }

    // Bind socket to the CAN interface
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        close(s);
        return 1;
    }
    int found=0;
    // Continuously receive CAN messages until you find IFTOTPOW
    while (!found) {
        int nbytes = read(s, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            perror("Read");
            break;
        }

        // Print the sender ID and data payload
        //printf("Sender ID: 0x%X | Data: ", (frame.can_id) & 0x1FFFFFFF);

        unsigned modType = (frame.can_id >>  9) & 0xFF;
        unsigned msgType = (frame.can_id >> 17) & 0x3FF;
        if(modType == 224){
            switch(msgType){
                case 0x0E0: {
                    u.b[0] = frame.data[3]; u.b[1] = frame.data[2]; u.b[2] = frame.data[1]; u.b[3] = frame.data[0]; //IFTOTPOW
		    ch1 = 10000. * u.f;
		    ch2 = 10000. * u.f;
		    ch3 = 10000. * u.f;
		    ch4 = 10000. * u.f;
		    found=1;
                }
            }
        }
    }
    close(s);


  if(!(indx%2))
  {
    sprintf(stufBuf, "TP 0 1 12500 %5.0f %5.0f %5.0f %5.0f \r", ch1, ch2, ch3, ch4);
  }
  else
  {
    sprintf(stufBuf, "TP 0 0 12500 %5.0f %5.0f %5.0f %5.0f \r", ch1, ch2, ch3, ch4);
  }

  ret = sock_send(client, stufBuf);
  printf("%s", stufBuf);

  if(ret){
    printf("sent Report: %s\n", stufBuf);
  }

  if(!ret)
  {
    //log_msg("Lost connection to stuffer");
    printf("Lost connection to stuffer\n");
    running = 0;
  }

  indx++;

  if(!(indx%8))
  {
    sendTimeStamp(PROCESS_DBEDBEFAKEDBE);
    //printf("send time stamp\n");
  }

  return 0;

}




int main(int argc, char **argv)
{
  int n, sel, value;

  // variables
  int predel = 0;
  int blank  = 0;
  int active = 0;
  int pstime = 0;
  int pulse  = 0;

  // states
  // report is a global
  int nutate = 0;
  int fbank  = 0;

  setCactusEnvironment();               // get to all the telescope environment variables

  log_open("if_control", 3);

  sock_bind("NEWDBE");		// our commands come in here


  /* main loop: send data if available, get command, parse, execute */
  while(1) 
  {

    usleep(100000);   // wait a few milliseconds

    if(running) // new data wants to be sent to catcher
    {
      if(client)
      {
        send_data();
      }
    }

    // Ask sock_sel if any messages waiting
    sel = sock_sel(msgbuf, &n, 0, 0, -1, 0); 
    if (sel > 0)
    {    // poll request
      sel = sock_sel(msgbuf, &n, 0, 0, 1, 0);    // read the msg
      if (sel < 0)
      {
        //log_msg("Sock_sel returned %d", sel);
      }
      else
      {

	if (!client){
          client = sock_connect("DBE_STUFFER");
	  printf("connected to DBE_STUFFER\n");
	}

        if (n == sizeof(cfg)) 
        {
          /* this is the config parameter structure being sent to us */
          bcopy(msgbuf,&cfg,sizeof(cfg)); // make a local copy of it
          if (!strncmp(cfg.magic,SPEC_CONFIG_MAGIC,MAGIC_SIZE))
            do_config();   // act on it
        }
        /* Look for the command to do */
        if (msgbuf[n-1] == '\n') msgbuf[n-1] = 0;

        printf("Got Command: %s\n", msgbuf);

        if(!strcmp(msgbuf,"newlog"))
        {
          log_newlog();	// Rotate the log files
        }
        else
        if(!strcmp(msgbuf,"INIT"))
        {
          do_initialize();		// Initialize CAN
	  sock_send(client, "ACK ");
        }
        else
        if(!strcmp(msgbuf,"REPORT 1"))
        {
          do_start();			// Start reporting
	  sock_send(client, "ACK ");
        }
        else
        if(!strcmp(msgbuf,"REPORT 0"))
        {
          do_stop();			// Stop reporting
	  sock_send(client, "ACK ");
        }
        else
        if(!strcmp(msgbuf,"NUTATE 1"))
        {
          nutate = 1;
	  sock_send(client, "ACK ");
        }
        else
        if(!strcmp(msgbuf,"NUTATE 0"))
        {
          nutate = 0;
	  sock_send(client, "ACK ");
        }
        else
        if(!strcmp(msgbuf,"FBANK  1"))
        {
          fbank = 1;
	  sock_send(client, "ACK ");
        }
        else
        if(!strcmp(msgbuf,"FBANK  0"))
        {
          fbank = 0;
	  sock_send(client, "ACK ");
        }
        else
        if(!strncmp(msgbuf,"PREDEL", 6)){
          if(sscanf(msgbuf, "%*s %d", &value) == 1){
            predel = value;
	    sock_send(client, "ACK ");
          }
          else
          {
            sprintf(stufBuf, "ACK PREDEL %d \r", predel);
            sock_send(client, stufBuf);
          }
        }
        else
        if(!strncmp(msgbuf,"BLANK", 5)){
          if(sscanf(msgbuf, "%*s %d", &value) == 1){
            blank = value;
	    sock_send(client, "ACK ");
          }
          else
          {
            sprintf(stufBuf, "ACK BLANK %d \r", blank);
            sock_send(client, stufBuf);
          }
        }
        else
        if(!strncmp(msgbuf,"ACTIVE", 6)){
          if(sscanf(msgbuf, "%*s %d", &value) == 1){
            active = value;
	    sock_send(client, "ACK ");
          }
          else
          {
            sprintf(stufBuf, "ACK ACTIVE %d \r", active);
            sock_send(client, stufBuf);
          }
        }
        else
        if(!strncmp(msgbuf,"PSTIME", 6)){
          if(sscanf(msgbuf, "%*s %d", &value) == 1){
            pstime = value;
	    sock_send(client, "ACK ");
          }
          else
          {
            sprintf(stufBuf, "ACK PSTIME %d \r", pstime);
            sock_send(client, stufBuf);
          }
        }
        else
        if(!strncmp(msgbuf,"PULSE", 6)){
          if(sscanf(msgbuf, "%*s %d", &value) == 1){
            pulse = value;
	    sock_send(client, "ACK ");
          }
          else
          {
            sprintf(stufBuf, "ACK PULSE %d \r", pulse);
            sock_send(client, stufBuf);
          }
        }
        else
        if(!strcmp(msgbuf,"TP"))
        {
          sprintf(stufBuf, "TP 0 1 12500 %5.0f %5.0f %5.0f %5.0f \r", ch1, ch2, ch3, ch4);
          sock_send(client, stufBuf);

	  break; // (we don't want to send the ACK below)
        }
        else
        {
          printf("Unknown Command: %s\n", msgbuf);
        }

      }
    }
  }

  return 0;
}
