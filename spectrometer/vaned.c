/*******************************************************************
 *                                                                 *
 *                            vaned.c                       *
 *                                                                 *
 *******************************************************************
 *
 * Responds to VANED sock commands from spectral line system control
 * Commands are received through the VANED socket
 * "VANEIN"   puts the CAL load in place (GPIB +5V) and responds "VANEIN"
 * "VANEHOME" removes  CAL load in       (GPIB  0V) and responds "VANEHOME"
 *
 * Revision history
   05Jun2026 AY  Initial
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

// mboxdef
struct SOCK *client;		// MBOX socket to send data to CATCHER

#define MAX_SOCK 1 + 64*sizeof(char)  /* Biggest sock msg */

char msgbuf[MAX_SOCK];

int do_sky()
{
  system("echo \"cal --state 0\n\" | nc -w 1 localhost 9000");
  sock_send(client, "VANEHOME");
  return 0;
}

int do_hot()
{
  system("echo \"cal --state 1\n\" | nc -w 1 localhost 9000");
  sock_send(client, "VANEIN");
  return 0;
}

int main(int argc, char **argv)
{
  int n, sel;   // sockets
	
  setCactusEnvironment();               // get to all the telescope environment variables
   
  putenv("LOGDIR=/tmp");
  log_open("vaned", 3);

  //sock_bind("IBOB_CONTROL");		// our commands come in here
  sock_bind("VANED");		// our commands come in here

  log_msg("VANED Started");
  //

  /* main loop: send data if available, get command, parse, execute */
  while(1) 
  {

    usleep(10);   // wait a few milliseconds

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
          client = sock_connect("SPEC_CONTROL");

	  log_msg("connected to CONTROL");
	}

        /* Look for the command to do */
        if (msgbuf[n-1] == '\n') msgbuf[n-1] = 0;

        log_msg("Got Command: %s", msgbuf);

        if(!strcmp(msgbuf,"VANEIN"))
        {
          do_hot();	// put the hold load in
        }
        else
        if(!strcmp(msgbuf,"VANEHOME"))
        {
          do_sky();		// take the hot load out
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
