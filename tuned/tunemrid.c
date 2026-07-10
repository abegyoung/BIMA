/*******************************************************************
 *                                                                 *
 *                            tuned.c                       *
 *                                                                 *
 *******************************************************************
 *
 * Responds to TUNEMRID sock commands from RAMBO
 * Commands are received through the TUNEMRID socket
 * "lofreq    <freq_GHz>" Sets Gunn oscillator fundamental in GHz
 * "skyfreq   <freq_GHz>"
 * "skyfreq2  <freq_GHz>"
 * "sideband  <1 or2>"    Selects upper or lower sideband
 * "sideband2 <1 or2>"    Selects upper or lower sideband
 * "band_select <n>"      Selects band (
 * "band_select_lock"
 *
 * Revision history
   10Jun2026 AY  Initial
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
#include <fcntl.h>

#include <sys/mman.h>
#include <caclib_proto.h>

// mboxdef
struct SOCK *client;		// MBOX socket to send data back to RAMBO

#define MAX_SOCK 1 + 64*sizeof(char)  /* Biggest sock msg */

char msgbuf[MAX_SOCK];
char cmdbuf[64];

int do_freq(float freq)
{
  sprintf(cmdbuf, "echo \"setfreq --freq %.4f\n\" | nc -w 1 localhost 9000", freq);
  system(cmdbuf);
  sock_send(client, "done frequency set");

  return 0;
}

int do_null()
{
  return 0;
}

int main(int argc, char **argv)
{
  int n, sel;   // sockets
  char *cp;

  setCactusEnvironment();               // get telescope environment variables
   
  putenv("LOGDIR=/tmp");
  log_open("tunemrid", 3);

  sock_bind("TUNEMRID");		// our commands come in here

  log_msg("TUNEMRID Started");
  //

  /* main loop: send data if available, get command, parse, execute */
  while(1) 
  {

    usleep(10000);   // wait a few milliseconds

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
          client = sock_connect("RAMBO");

	  log_msg("connected to RAMBO");
	}

        /* Look for the command to do */
        if (msgbuf[n-1] == '\n') msgbuf[n-1] = 0;

        log_msg("Got Command: %s", msgbuf);

	cp = strtok(msgbuf, " ");

        if(!strncmp(msgbuf,"lofreq", 6))
        {
	  cp = strtok(NULL, " ");
          do_freq(atof(cp));	// Set Gunn
        }
        else
        if(!strncmp(msgbuf,"skyfreq", 7))
        {
          do_null();
        }
        else
        if(!strncmp(msgbuf,"sideband", 8))
        {
          do_null();
        }
        else
        if(!strncmp(msgbuf,"band_select", 11))
        {
          do_null();
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
