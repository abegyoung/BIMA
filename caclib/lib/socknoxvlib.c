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

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include "caclib_mbdef.h"
#include "caclib_sock.h"

#include "caclib_proto.h"


/*
      This includes the definitions of sock_write and sock_send 
*/

extern struct BOUND *bs;
extern struct SOCK sock_kbd;

extern int last_conn;
extern int logCallingSockFlag;

int sockWriteNoBitch = 0;


/* Send a ascii message */
int sock_send( s, message )
struct SOCK *s;
char *message;
{
  return( sock_write( s, message, strlen(message)));
}


/* like sock_send but takes pointer and count as in write */
int sock_write( s, message, l )
struct SOCK *s;
char *message;
int l;
{
  int ret, err, len, flag = 0;
  unsigned long count;
  unsigned long net;
  char *name;

  if( (long)s == -1 || !message || l <= 0 )
  {
    return(0);
  }

  if(!s)
  {
    if(!logCallingSockFlag)
    {
      log_msg("no mailbox for: %s", message );
    }

    return(0);
  }

  while( flag < 2 )
  {
    if( s->fd < 0 )
    {
      if( strcmp( s->dest->dname, "ANONYMOUS") == 0 )
      {
        return(0);
      }

      if( (s->fd = socket(s->sin.sin_family, SOCK_STREAM, 0 )) <0) 
      {
	if(strlen(s->dest->dname))
        {
          char buf[80];

          sprintf(buf,"sock_write, socket(%s)", s->dest->dname);
          log_perror(buf);
	}
	else
        {
          log_perror("sock_write, socket");
        }

        return(0);
      }

      bind_any(s->fd);

      if( connect( s->fd, (struct sockaddr *)&s->sin, sizeof(struct sockaddr_in) ) <0 )
      {
        close(s->fd);
        s->fd = -1;

	if(strlen(s->dest->dname))
        {
          if(!sockWriteNoBitch)
          {
            char buf[80];

            sprintf(buf,"sock_write, connect(%s)", s->dest->dname);
            log_perror(buf);
          }
	}
	else
        {
          log_perror("sock_write, connect");
        }

        return(0);
      }

	/* It's not a raw socket.  Send the mailbox name */
      if( ! s->dest->raw && ! bs->dest->raw)
      {
	if( bs->bind_fd >0 ) 
        {
	  count = strlen(bs->dest->dname);
	  name =  bs->dest->dname;
	} 
	else /* if he never bound then send ANONYMOUS */
	{
	  count = 9;
	  name = "ANONYMOUS";
	}

	net = htonl(count);

	if( (ret = write( s->fd, &net, sizeof(unsigned int)))>0 )
        {
	  ret = write( s->fd, name, count);
        }
      }
    }

    count = l;

	/* It's not a raw socket.  Send the byte count first */
    if(!s->dest->raw && !bs->dest->raw)
    {
      net = htonl(count);
      ret = write( s->fd, &net, sizeof(unsigned int));
    } 
    else
    {
      ret = 1;			/* Raw socket always writes the data */
    }

    if(ret > 0) 
    {
      int ct, ix;

      ct = count;
      ix = 0;

      while( ct > 0 && ret > 0 )
      {
        ret = write( s->fd, &message[ix], ct);

        ct -= ret;
        ix += ret;
      }
    }

/*
   this checks to see if vxworks box (or sun) has rebooted 
*/
    usleep(1);
    len = 4;
    err = 0;

    if( getsockopt( s->fd, SOL_SOCKET, SO_ERROR, (char *)&err, (socklen_t *__restrict)&len ) < 0 || err != 0 )
    {
      ret = count + 1;
    }

    if( ret != count ) 
    {
      if(ret <0 )
      {
	if(strlen(s->dest->dname)) 
        {
          char buf[80];

          sprintf(buf,"sock_write(%s)", s->dest->dname);
          log_perror(buf);
	}
	else
        {
          log_perror("sock_write");
        }
      }

      if(!logCallingSockFlag)
      {
        log_msg("closing out mailbox: %s", s->dest->dname );
      }

      close( s->fd);

      s->fd = -1;
      flag += 1;
    } 
    else
    {
      break;
    }
  }

  return(1);
}
