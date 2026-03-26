/* 
Copyright 2021 Arizona Board of Regents on behalf of the University of Arizona.
Author: Thomas Folkers; tfolkers@arizona.edu; Unless otherwise noted.

For commercial uses, to obtain a license to sell and or sublicense copies of
the Software please contact the University of Arizona at Tech Launch Arizona:
lewish@tla.arizona.edu.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction for non-commercial use educational and
research purposes, including without limitation the rights to use, copy,
modify, merge, publish, distribute, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The entire above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

1. Additional Required Provisions

1.1.  Arbitration. The parties agree that if a dispute arises between them
concerning this Agreement, the parties may be required to submit the matter to
arbitration pursuant to Arizona law.

1.2.  Applicable Law and Venue. This Agreement shall be interpreted pursuant
to the laws of the State of Arizona. Any arbitration or litigation between the
Parties shall be conducted in Pima County, ARIZONA, and LICENSEE hereby
submits to venue and jurisdiction in Pima County, ARIZONA.

1.3.  Non-Discrimination. The Parties agree to be bound by state and federal
laws and regulations governing equal opportunity and non-discrimination and
immigration.

1.4.   Appropriation of Funds. The Parties recognize that performance by
ARIZONA may depend upon appropriation of funds by the State Legislature of
ARIZONA. If the Legislature fails to appropriate the necessary funds, or if
ARIZONA’S appropriation is reduced during the fiscal year, ARIZONA may cancel
this Agreement without further duty or obligation. ARIZONA will notify
LICENSEE as soon as reasonably possible after it knows of the loss of funds.

1.5.   Conflict of Interest. This Agreement is subject to the provisions of
A.R.S. 38-511 and other conflict of interest regulations. Within three years
of the EFFECTIVE DATE, ARIZONA may cancel this Agreement if any person
significantly involved in initiating, negotiating, drafting, securing, or
creating this Agreement for or on behalf of ARIZONA becomes an employee or
consultant in any capacity of LICENSEE with respect to the subject matter of
this Agreement.

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "caclib_proto.h"

/* Program to manaully, using the terminal, send a socket message
   to another program;
 */
int main(int argc, char *argv[])
{
  char *sourcename, *destname;
  char *message;
  struct SOCK *source;
  int len, timeout;
  int quit_flag = 0;
  int quiet_flag = 0;
  int response_flag = 0;
  int slam_flag = 0;
  int ct_flag = 8192;

  sourcename = NULL;
  destname   = "ANONYMOUS";

  if( argc <= 1 )
  {
    printf( "\nusage: %s [-s] [-r] [-d destname] [sourcename]\n", argv[0] );
    printf( "  -d destname - the destination mailbox name\n" );
    printf( "  sourcename  - the source mailbox name\n" );
    printf( "  -r response is expected\n\n" );
    printf( "  -s  slam it down one line at a time with out looking for a response\n");
    printf( "  -ct  ct  size of buffer\n");
    printf( "  -q Quiet!  Skip \"message received\" prefix\n");

    exit(0);
  }

  while( *++argv )
  {
    if( strcmp( *argv , "-d" ) == 0 )
    {
      destname = *++argv;
    }
    else
    if( strcmp( *argv , "-r" ) == 0 )
    {
      response_flag = 1;
    }
    else
    if( strcmp( *argv , "-s" ) == 0 )
    {
      slam_flag = 1;
    }
    else
    if( strcmp( *argv , "-ct" ) == 0 )
    {
      ct_flag = atoi(*++argv);
    }
    else
    if( strcmp( *argv , "-q" ) == 0 )
    {
      quiet_flag = 1;
    }
    else
    {
      sourcename = *argv;
    }
  }

  message = (char *)malloc(ct_flag);

  if( destname )
  {
    sock_bind(destname);
  }

  sock_bufct(ct_flag);

  if( sourcename )
  {
    source = sock_connect(sourcename);
  }

  if( response_flag )
  {
    timeout = 1;
  }
  else
  {
    timeout = 0;
  }

  if( slam_flag ) 
  {
    while( fgets(message, ct_flag-1, stdin))
    {
      sock_send( source, message );
    }
  }
  else
  {
    while(1) 
    {
      switch(sock_sel( message, &len, NULL, 0, timeout, !quit_flag)) 
      {
        case 0: 
          if (!sock_send( source, message) && destname && !sourcename)
          {
	    source = 0; 
          }

          break;
        case -1:
          if( quit_flag )
	  {
            exit(1);
          }

        case -2:
        case -3:
          if( response_flag )
	  {
            quit_flag = 1;
	  }
          else
	  {
            exit(1);
	  }

          break;
        default:
	  if (source == 0)
          {
	    source = (struct SOCK *)last_msg();
	  }

          if( response_flag || quiet_flag)
	  {
            printf("%s\n", message );
	  }
          else
	  {
            printf("message received %s\n", message );
	  }

          break;
      }
    }
  }

  return(EXIT_SUCCESS);  /* never reached */
}

