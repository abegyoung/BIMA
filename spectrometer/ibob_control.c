/*******************************************************************
 *                                                                 *
 *                            ibob_control.c                       *
 *                                                                 *
 *******************************************************************
 *
 * This program takes data from the IBOB spectrometer and passes it to the catcher program.
 * Commands are received through the IBOB_CONTROL socket, rather than from the keyboard.
 *
 * Revision history
   12Oct2016 DF  Copied from ~cactus/spectral/ffb/utils/fbmon.c
   03Feb2017 DF  Forked from arows-mon.c
   08Feb2017 DF  Changed to 0.9 to 2.1 GHz IF input
   09Oct2017 DF  Changed socket names, adding commands per Tom 
   18Mar2026 AY  Copied from ~/cactus/spectral/aro/utils/arows_control.c
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

//for PRI64
#include <inttypes.h>

//Includes for IBOB
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <byteswap.h>

#define BUFLEN 1048
#define PORT 59000
#define SERVER_IP "192.168.1.8"
#define SERVER_PORT 23

// Socket Globals
// UDP
int udpsock;		// UDP socket to receive IBOB DATA
struct sockaddr_in si_me, si_other;
socklen_t slen = sizeof(si_other);

// TCP
int tcpsock;			// TCP socket to communicate to IBOB

// mboxdef
struct SOCK *client;		// MBOX socket to send data to CATCHER

// STRUCT GLOBALS
int MODE = 0; //0=sig, 1=hot, 2=sky
int BIN  = 0; //0=sig, 1=ref

void diep(char *s)
{
        perror(s);
        exit(1);
}

int running = 0;		// whether we are sending data out

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
#define N_USABLE_POINTS 1024
#define NPARTS          1
#define WHICH_SPEC      2

#define MAX_SOCK 1 + sizeof(struct SPEC_DATA_SET)  /* Biggest sock msg */

struct SPEC_DATA_SET outgoing;   // for catcher to catch via sock write

int zero_factor = 1;

struct timespec start_time, end_time;   // one iteration

char msgbuf[MAX_SOCK];
struct SPEC_CONFIG cfg;		// local spectrometer config 

int do_config()
{
  log_msg("DO_CONFIG Got config - Starting configuration of IBOB");

  running = 0;
  usleep(10000);
  sock_send(client, "done config");
  usleep(10000);
  running = 1;

  return 0;
}

void clear_udp_buffer(int sockfd) {
  char buffer[1500]; // Use a buffer large enough for a typical Ethernet frame (MTU)
  ssize_t bytes_received;

  while (1) {
    bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT, NULL, NULL);

    if (bytes_received < 0) {
      // Check for the non-blocking error code
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        // Buffer is empty, so we can stop.
        break;
      } else {
      perror("recvfrom");
      // Handle a different kind of error here
        break;
        }
    }
  // Packet was received successfully, but we do nothing with it.
  // The loop will continue until the buffer is drained.
  }
}

// Do all steps necessary to initialize spectrometer and start IBOB UDP stream
void do_initialize()
{

        // OPEN UDP RECEIVE CHANNEL
        slen=sizeof(si_other);
        if ((udpsock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
                diep("socket");

        memset((char *) &si_me, 0, sizeof(si_me));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(PORT);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(udpsock, (const struct sockaddr *)&si_me, sizeof(si_me))==-1)
                diep("bind");


        char *message;

        // OPEN TCP CONTROL CHANNEL
        struct sockaddr_in serv_addr;
        // create socket file desc
        if ((tcpsock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                perror("Socket creation error");
                exit(EXIT_FAILURE);
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);

        // create IPv4 address in binary
        if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0){
                perror("Invalid address");
                exit(EXIT_FAILURE);
        }

        // Connect to server
        if (connect(tcpsock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                perror("Connection failed");
                exit(EXIT_FAILURE);
        }


        //Accumulator Integration length MAX 39,999
        message = "regwrite cfgspec/vacc/acc_len 39999\n";
        send(tcpsock, message, strlen(message), 0);

        //Integration period MAX 10239998
        message = "regwrite period 10239998\n";
        send(tcpsock, message, strlen(message), 0);

        //Start UDP broadcast to ADDR, PORT and also send raw ADC values '24'
        message = "startudp 192 168 1 52 59000 24\n";
        send(tcpsock, message, strlen(message), 0);

}


/* Start command */
int do_start()
{
  log_msg("IBOB_CONTROL got start");

  sock_send(client, "done start");

  running = 1;

  return 0;
}

/* Stop command */
int do_stop()
{
  log_msg("IBOB_CONTROL got stop");

  sock_send(client, "done stop");

  running = 0;
  
  return 0;
}

// Shutdown command
int do_shutdown()
{
  log_msg("IBOB_CONTROL got shutdown");

  sock_send(client, "done shutdown");

  // STOP UDP broadcast and close UDP and TCP SOCKETS
  char *message = "endudp";
  send(tcpsock, message, strlen(message), 0);

  close(tcpsock);
  close(udpsock);

  return 0;
}

int do_sky()
{
  MODE = 0;
  BIN  = 0;
  system("echo \"cal --state 0\n\" | nc -w 1 localhost 9000");

  running = 0;
  usleep(10000);
  sock_send(client, "done sky");
  usleep(10000);
  running = 1;

  return 0;
}
int do_hot()
{
  MODE = 1;
  BIN  = 1;
  system("echo \"cal --state 1\n\" | nc -w 1 localhost 9000");

  running = 0;
  usleep(10000);
  sock_send(client, "done hot");
  usleep(10000);
  running = 1;

  return 0;
}
// set IF signal to zero signal using fake math not a real relay (waiting for IF processor!)
int do_zero_in()
{

  MODE=1;
  BIN=1;

  // turn off observing
  running = 0;
  // clear the current UDP buffer from the spectrometer
  clear_udp_buffer(udpsock);
  // wait a moment
  usleep(40000);

  log_msg("Setting IF to zero signal.");
  zero_factor = 0;                    //     force all data to be multiplied by zero

  // clear the current UDP buffer from the spectrometer
  clear_udp_buffer(udpsock);
  // turn on observing
  running = 1;

  sock_send(client, "done zero_in");

  return 0;
}

// Restore the IF signal after zero check is done
int do_zero_out()
{

  MODE=0;
  BIN=0;

  // turn off observing
  running = 0;
  // clear the current UDP buffer from the spectrometer
  clear_udp_buffer(udpsock);
  // wait a moment
  usleep(40000);

  log_msg("Restoring IF from zero signal.");
  zero_factor = 1;                    //     force all data to be multiplied by one
				      //
  // clear the current UDP buffer from the spectrometer
  clear_udp_buffer(udpsock);
  // turn on observing
  running = 1;

  sock_send(client, "done zero_out");
  return 0;
}

int do_cold()
{
  running = 0;
  usleep(10000);
  sock_send(client, "done cold");
  usleep(10000);
  running = 1;

  return 0;
}




// send a packet of spectral data to the catcher or whatever
void send_data()
{
  // CATCHER SPECIFIC
  // Format data for catcher
  struct SPEC_DATA_SET outgoing;
  strcpy(outgoing.sdss.magic, "ARO_DATA_SDSS");
  outgoing.sdss.which_spec = WHICH_SPEC;
  outgoing.sdss.nparts = NPARTS;
  outgoing.sdss.nchans = NPOINTS;
  outgoing.sdss.depth = 1;

  outgoing.sdss.mode = MODE;
  outgoing.sdss.bin  = BIN;
  
  start_time.tv_sec = end_time.tv_sec;
  start_time.tv_nsec = end_time.tv_nsec;
  clock_gettime(CLOCK_REALTIME, &end_time);

  outgoing.sdss.start_time[0] = start_time.tv_sec;
  outgoing.sdss.start_time[1] = start_time.tv_nsec;
  outgoing.sdss.end_time[0] = end_time.tv_sec;
  outgoing.sdss.end_time[1] = end_time.tv_nsec;

  outgoing.sdss.error_bits = 0;
  outgoing.sdss.int_time = 400000.; //40 msec * 10 = 400,000 usec

  // IBOB SPECIFIC
  int i;
  int packet;
  int integration=10;
  char label;
  unsigned char bram;     //8
  unsigned char nbram;    //8
  unsigned short offset;  //16
  unsigned short depth;   //16
  unsigned int acc;       //32
  unsigned int count;     //32
  unsigned short load;    //16

  unsigned char ch[BUFLEN];
  uint64_t msb[1024]; //32 bits
  uint64_t lsb[1024]; //32 bits

  unsigned char buf[BUFLEN];

  for(i=0;i<1024;i++){
    lsb[i]=0;
    msb[i]=0;
  }

  // Clear the current UDP stream before receiving new spectra
  clear_udp_buffer(udpsock);

  while(1){
    recvfrom(udpsock, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen);
    memcpy(&bram,  buf+2, sizeof(int8_t));
    memcpy(&offset,buf+4, sizeof(int16_t));
    offset=bswap_16(offset);
    count=bswap_32(count);
    if(bram==1 && offset==768)
    break;
  }

  for(packet=0;packet<(8*integration);packet++){

    recvfrom(udpsock, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen);

    memcpy(&label, buf+0, sizeof(uint8_t));
    memcpy(&bram,  buf+2, sizeof(uint8_t));
    memcpy(&nbram, buf+3, sizeof(uint8_t));
    memcpy(&offset,buf+4, sizeof(uint16_t));
    memcpy(&depth, buf+6, sizeof(uint16_t));
    memcpy(&acc,   buf+8, sizeof(uint32_t));
    memcpy(&count, buf+12,sizeof(uint32_t));
    memcpy(&load,  buf+16,sizeof(uint16_t));
    offset=bswap_16(offset);
    count=bswap_32(count);

    for (i=0; i<1024; i++){
      memcpy(&ch[i], buf+24+i, sizeof(uint8_t));      //Fill the spectrometer channel numbers as they come in
    }

    //Fill the 32 LSB and 32 MSB by adding up interleaved BRAMS, rotating by 8 bits every time
    for (i=0; i<256; i++){
      if(offset==000 && bram==0) lsb[i]+=(ch[i*4] << 24) + (ch[i*4+1] << 16) + (ch[i*4+2] << 8) + ch[i*4+3];
      if(offset==256 && bram==0) lsb[i+256]+=(ch[i*4+0] << 24) + (ch[i*4+1] << 16) + (ch[i*4+2] << 8) + (ch[i*4+3] << 0);
      if(offset==512 && bram==0) lsb[i+512]+=(ch[i*4+0] << 24) + (ch[i*4+1] << 16) + (ch[i*4+2] << 8) + (ch[i*4+3] << 0);
      if(offset==768 && bram==0) lsb[i+768]+=(ch[i*4+0] << 24) + (ch[i*4+1] << 16) + (ch[i*4+2] << 8) + (ch[i*4+3] << 0);

      if(offset==000 && bram==1) msb[i+000]+=(ch[i*4+0] << 24) + (ch[i*4+1] << 16) + (ch[i*4+2] << 8) + (ch[i*4+3] << 0);
      if(offset==256 && bram==1) msb[i+256]+=(ch[i*4+0] << 24) + (ch[i*4+1] << 16) + (ch[i*4+2] << 8) + (ch[i*4+3] << 0);
      if(offset==512 && bram==1) msb[i+512]+=(ch[i*4+0] << 24) + (ch[i*4+1] << 16) + (ch[i*4+2] << 8) + (ch[i*4+3] << 0);
      if(offset==768 && bram==1) msb[i+768]+=(ch[i*4+0] << 24) + (ch[i*4+1] << 16) + (ch[i*4+2] << 8) + (ch[i*4+3] << 0);
    }

    //Output a spectra once a complete one has come in
    if(packet==(8*integration)-1){
      for(i=0; i<1024; i++){
        //printf("%d %" PRIu64 "\n", i, (uint64_t) (lsb[i]+(msb[i]<<32)) / integration);	// DEBUG OUTPUT TO SCREEN
        outgoing.data[i] = zero_factor * 500.*(uint64_t) (lsb[i]+(msb[i]<<32)) /integration;			// BUILD OUTPUT TO CATCHER
      }
    }
  }

  outgoing.data[510]=0.5*(outgoing.data[509]+outgoing.data[513]);
  outgoing.data[511]=0.5*(outgoing.data[509]+outgoing.data[513]);
  outgoing.data[512]=0.5*(outgoing.data[509]+outgoing.data[513]);

  sock_write(client, (char *)&outgoing, sizeof(outgoing.sdss) + NPARTS * NPOINTS * sizeof(int));	// SEND to CATCHER

}




int main(int argc, char **argv)
{
  int n, sel;   // sockets
	
  do_initialize();			//Start sending UDP streamed data to CATCHER
  usleep(3000);

  setCactusEnvironment();               // get to all the telescope environment variables
   
  putenv("LOGDIR=/tmp");
  log_open("ibob_control", 3);

  //sock_bind("IBOB_CONTROL");		// our commands come in here
  sock_bind("ARO_CONTROL");		// our commands come in here
  sock_bufct(sizeof(outgoing)+80);

  log_msg("IBOB_CONTROL Started");
  //

  /* main loop: send data if available, get command, parse, execute */
  while(1) 
  {

    usleep(10);   // wait a few milliseconds

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
        log_msg("Sock_sel returned %d", sel);
      }
      else
      {

	if (!client){
          client = sock_connect("SPEC_ARO_CATCHER");

	  log_msg("connected to CATCHER");
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

        log_msg("Got Command: %s", msgbuf);

        if(!strcmp(msgbuf,"newlog"))
        {
          log_newlog();	// Rotate the log files
        }
        else
        if(!strcmp(msgbuf,"init"))
        {
          do_initialize();		// Initialize hardware and UDP stream
        }
        else
        if(!strcmp(msgbuf,"start"))
        {
          do_start();			//Start sending UDP streamed data to CATCHER
        }
        else
        if(!strcmp(msgbuf,"stop"))
        {
          do_stop();			// Stop sending data out
        }
        else
        if(!strcmp(msgbuf,"shutdown"))
        {
          do_shutdown();		// Shutdown IBOB sending UDP stream
        }
        else
        if(!strcmp(msgbuf,"sky"))
        {
          do_sky();		// Do a Sky	*flip out the VANE
        }
        else
        if(!strcmp(msgbuf,"hot"))
        {
          do_hot();		// Do a Hot	*flip in the VANE
        }
        else
        if(!strcmp(msgbuf,"cold"))
        {
          do_cold();		//  Do a Cold	*we don't have a cold load
        }
        else
        if(!strcmp(msgbuf,"zero_in"))
        {
		do_zero_in();
        }
        else
        if(!strcmp(msgbuf,"zero_out"))
        {
		do_zero_out();
        }
        else
        if(!strncmp(msgbuf,"bsp",3))
        {
          log_msg("got bsp");
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
