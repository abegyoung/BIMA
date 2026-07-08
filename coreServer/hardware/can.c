#include "can.h"
#include "sock.h"
#include "hardware.h"
#include "../Dispatch.h"
#include "../Server.h"
#include "../Logging.h"
#include <pthread.h>
#include <syslog.h>

extern int tellUser(int, const char *fmt, ...);

union {
  float f;
  unsigned char b[4];
} u;

// data.frame[0] is sent first, data.frame[7] is sent last
// This function reverses the 8 byte data payload from the argument
unsigned long long reverse_payload(unsigned long long val) {
  return ((val & 0x00000000000000FFULL) << 56) |
         ((val & 0x000000000000FF00ULL) << 40) |
         ((val & 0x0000000000FF0000ULL) << 24) |
         ((val & 0x00000000FF000000ULL) << 8)  |
         ((val & 0x000000FF00000000ULL) >> 8)  |
         ((val & 0x0000FF0000000000ULL) >> 24) |
         ((val & 0x00FF000000000000ULL) >> 40) |
         ((val & 0xFF00000000000000ULL) >> 56);
}


/* Send a CAN frame */
int writeCan(uint32_t canid, uint64_t candata) 
{
  int s;
  struct sockaddr_can addr;
  struct ifreq ifr;

  // Open a socket
  s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (s < 0) {
    perror("Socket");
    return 1;
  }

  // Specify the CAN interface ("can0")
  strcpy(ifr.ifr_name, "can0");
  if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
    perror("SIOCGIFINDEX");
    return 1;
  }

  // Bind the socket to can0
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Bind");
    return 1;
  }

  struct can_frame frame;

  // Prepare the CAN frame
  memset(&frame, 0, sizeof(frame));
  frame.can_id = canid | CAN_EFF_FLAG;  // EFF flag indicates extended frame
  frame.can_dlc = 8;                        // 8 bytes of data

  // Fill data bytes
  candata = reverse_payload(candata);
  memcpy(frame.data, &candata, sizeof(candata));

  // Send the frame
  if (write(s, &frame, sizeof(frame)) != sizeof(frame)) {
    perror("Write");
    return 1;
  }

  close(s);
  return 0;

}

/* Start a worker thread that receives CAN frames
 * 1) open CAN socket
 * 2) set interface stuff
 * 3) bind to it
 * 4) Go into an infinite while() loop listening for CAN frames
 * 5) if we get a msgid that we recognize, decode and send to client
 * 6) if there's a mutex in fdout, set a new client to receive can frames */
void *can_receiver_thread(void *arg) {
  int s;
  struct sockaddr_can addr;
  struct ifreq ifr;
  struct can_frame frame;
  char buf[22];

  int last_dest = -1;

  int fdout = 6; //TODO: FIX ME
		 //fdout is the client stdout
		 //hardcoding this here limits CAN TELLUSER to only the
		 //1st client.  Subsequent clients aren't sent CAN data
		 //
		 //FIXED
		 //fdout is reset during the while() loop by a mutex and
		 //passed back to Server main() via pthread_mutex
  int numP;

   // Open CAN raw socket
  s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (s < 0) {
    perror("Socket");
  }

  // Set interface name (can0)
  strcpy(ifr.ifr_name, "can0");
  if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
    perror("SIOCGIFINDEX");
    close(s);
  }

  // Bind socket to the CAN interface
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Bind");
    close(s);
  }

  printf("[CAN thread] Listening on can0...\n");

  // Main loop: receive CAN frames
  while (1) {
    // Check which client we should send received CAN frames to
    pthread_mutex_lock(&destination_lock);
    if (destination != last_dest) {
       printf("destination changed to %d\n", destination);
       last_dest = destination;
       fdout = destination;
    }
    // Release mutex when done
    pthread_mutex_unlock(&destination_lock);
    
    int nbytes = read(s, &frame, sizeof(struct can_frame));
    if (nbytes < 0) {
      perror("Read");
      break;
    }

    unsigned modType = (frame.can_id >>  9) & 0xFF;	// Decode Module Type (    Receiver Ctrl == 144) 
                                                        //                    (alternate SIS     == 208)
                                                        //                    (Frontend SIS Bias == 209)
                                                        //                    (           IF PAM == 224)
    unsigned msgType = (frame.can_id >> 17) & 0x3FF;	// Decode Message Type indicating what's in the data payload
    if(modType == 144){
      switch(msgType){
        case 0x0E0: {
          tellUser(fdout, "DAC0 %d\n", frame.data[0]<<8|frame.data[1]);
          tellUser(fdout, "DAC1 %d\n", frame.data[2]<<8|frame.data[3]);
          tellUser(fdout, "DAC2 %d\n", frame.data[4]<<8|frame.data[5]);
          tellUser(fdout, "DAC3 %d\n", frame.data[6]<<8|frame.data[7]);
	  break;
        }
        case 0x0E1: {
          tellUser(fdout, "ADC0 %.03f\n", (float)(int16_t)(frame.data[0]<<8|frame.data[1])*4.096/32768.);
          tellUser(fdout, "ADC1 %.03f\n", (float)(int16_t)(frame.data[2]<<8|frame.data[3])*4.096/32768.);
          tellUser(fdout, "ADC2 %.03f\n", (float)(int16_t)(frame.data[4]<<8|frame.data[5])*4.096/32768.);
          tellUser(fdout, "ADC3 %.03f\n", (float)(int16_t)(frame.data[6]<<8|frame.data[7])*4.096/32768.);
	  break;
        }
        case 0x0E2: {
          tellUser(fdout, "ADC4 %d\n", frame.data[0]<<8|frame.data[1]);
          tellUser(fdout, "ADC5 %d\n", frame.data[2]<<8|frame.data[3]);
          tellUser(fdout, "ADC6 %d\n", frame.data[4]<<8|frame.data[5]);
          tellUser(fdout, "ADC7 %d\n", frame.data[6]<<8|frame.data[7]);
	  break;
        }
        case 0x0E3: {
          tellUser(fdout, "TTL0 " BYTE_TO_BINARY_PATTERN"\n", BYTE_TO_BINARY(frame.data[7]));
          tellUser(fdout, "TTL1 " BYTE_TO_BINARY_PATTERN"\n", BYTE_TO_BINARY(frame.data[6]));
	  sprintf(buf, "W %2X 80", ((~frame.data[6] & 0x01)<<7));	//TTL1 bit 1 = X lock
	  writeSock("SAMBUSD", buf);
	  //log_server_msg( LOG_INFO, "Sock sent: %s", buf);
	  sprintf(buf, "W %2X 100", ((~frame.data[6] & 0x02)<<7));	//TTL1 bit 2 = MM lock
	  writeSock("SAMBUSD", buf);
	  //log_server_msg( LOG_INFO, "Sock sent: %s", buf);
	  //tellUser(fdout, "SERVER %d %d %d %.3f %.3f", server.BandSelect, server.YIGHarmonicM, server.GunnHarmonicN, server.GunnFreq, server.L_Band);
	  break;
        }
      }
    }
    if(modType == 208){
      switch(msgType){
        case 0x20F: {
          tellUser(fdout, "VMIX_GAIN %.02f\n",      (float)(uint16_t)(frame.data[0]<<8|frame.data[1])/10.);	//VmixGain (UI)
          tellUser(fdout, "VMIX_OFFSET %.02f\n",    (float) (int16_t)(frame.data[2]<<8|frame.data[3])/1000.);	//VmixOffset (SI)
	  break;
        }
        case 0x211: {
          tellUser(fdout, "IMIX_GAIN %.02f\n",      (float)(uint16_t)(frame.data[0]<<8|frame.data[1])/10.);	//VmixGain (UI)
          tellUser(fdout, "IMIX_OFFSET %.02f\n",    (float) (int16_t)(frame.data[2]<<8|frame.data[3])/1000.);	//VmixOffset (SI)
	  break;
        }
        case 0x213: {
          tellUser(fdout, "VMIX_DACOFFSET %.02f\n", (float) (int16_t)(frame.data[0]<<8|frame.data[1])/1000.);	//VmixDACOffset (SI)
	  break;
        }
      }
    }
    if(modType == 209){
      switch(msgType){
        case 0x0E0: {
          tellUser(fdout, "VSET %.3f\n", (float) (int16_t)(frame.data[0]<<8|frame.data[1])/1000.);	//Vset (SI)
          tellUser(fdout, "VMON %.3f\n", (float) (int16_t)(frame.data[2]<<8|frame.data[3])/1000.);	//Vmon (SI)
          tellUser(fdout, "ISET %.3f\n", (float) (int16_t)(frame.data[4]<<8|frame.data[5])/10.);	//Iset (SI)
          tellUser(fdout, "IMON %.3f\n", (float) (int16_t)(frame.data[6]<<8|frame.data[7])/10.);	//Imon (SI)
          break;
        }
        case 0x0E1: {
          tellUser(fdout, "VLOOP %d\n", (uint8_t)(frame.data[2]));					//VLOOP (UB)
	  break;
        }
        case 0x0E2: {
          tellUser(fdout, "VGAP %0.1f\n", (int16_t)(frame.data[0]<<8|frame.data[1])/1000.);		//VGAP (SI)
	  break;
        }
        case 0x0E3: {
          tellUser(fdout, "FETEMP %.2f\n", (float)(frame.data[2]<<8|frame.data[3])/100.);		//FETEMP (SI)
	  break;
        }
        case 0x0E4: {
          tellUser(fdout, "VD1 %.02f\n", (float)(uint16_t)(frame.data[0]<<8|frame.data[1])/1000.);	//Vd1 (UI)
          tellUser(fdout, "ID1 %.02f\n", (float)(uint16_t)(frame.data[2]<<8|frame.data[3])/1000.);	//Id1 (UI)
          tellUser(fdout, "VG1 %.02f\n", (float) (int16_t)(frame.data[4]<<8|frame.data[5])/1000.);	//Vg1 (SI)
          break;
        }
        case 0x0E5: {
          tellUser(fdout, "VG2 %.02f\n", (float) (int16_t)(frame.data[4]<<8|frame.data[5])/1000.);	//Vg2 (SI)
          break;
        }
        case 0x170: {
          int16_t bitsV = frame.data[4]<<8|frame.data[5];
          int16_t bitsI = frame.data[6]<<8|frame.data[7];
          if (bitsV==0 && bitsI==0) {
            tellUser(fdout, "IV END\n");
            break;
          }
          tellUser(fdout, "IV %.3f ", (float)bitsV/1000.);
          tellUser(fdout, "%.3f\n",  (float)bitsI/10.);

        }
      }
    }
    if(modType == 224){
      switch(msgType){
        case 0x0E0: {
          u.b[0] = frame.data[3]; u.b[1] = frame.data[2]; u.b[2] = frame.data[1]; u.b[3] = frame.data[0];
          tellUser(fdout, "IFTOTPOW %.6f\n",u.f);
          u.b[0] = frame.data[7]; u.b[1] = frame.data[6]; u.b[2] = frame.data[5]; u.b[3] = frame.data[4]; //PAMTEMP
          tellUser(fdout, "PAMTEMP %.1f\n", u.f); //PAMTEMP  (FL)
          break;
        }
        case 0x0E1: {
          u.b[0] = frame.data[3]; u.b[1] = frame.data[2]; u.b[2] = frame.data[1]; u.b[3] = frame.data[0];
          tellUser(fdout, "ATTENSET %.1f\n", u.f);
          break;
        }
        case 0x0E3: {
          u.b[0] = frame.data[3]; u.b[1] = frame.data[2]; u.b[2] = frame.data[1]; u.b[3] = frame.data[0];
          tellUser(fdout, "INATTEN %.1f\n", u.f);
          u.b[0] = frame.data[7]; u.b[1] = frame.data[6]; u.b[2] = frame.data[5]; u.b[3] = frame.data[4];
          tellUser(fdout, "OUTATTEN %.1f\n", u.f);
          break;
        }
        case 0x171: {
          // Fast IFTOTPOW
          // Optionally read with IVP sweep
          u.b[0] = frame.data[5];
          u.b[1] = frame.data[4];
          u.b[2] = frame.data[3];
          u.b[3] = frame.data[2];
          numP = frame.data[0]<<8|frame.data[1];
	  if (numP==301) {
            tellUser(fdout, "P %d %.3f\n", frame.data[0]<<8|frame.data[1], u.f);
	    tellUser(fdout, "P END\n");
	    break;
	  }
          tellUser(fdout, "P %d %.3f\n", frame.data[0]<<8|frame.data[1], u.f);
        }
      }
    }


  }

  close(s);
  pthread_exit(NULL);

}
