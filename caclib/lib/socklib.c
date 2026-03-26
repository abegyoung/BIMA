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
#include <stdlib.h>
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
    Jeff Hagen, former NRAO/Steward Obs enployee's socket library

    -  is used for socket communication

    -  is simpler - without signals and the asynchronous maintainance of structures.

    -  Only one bound socket is permitted

    -  Binding must be done to receive messages

    Routines:
      sock_bind(name)  - binds this process to name as defined in mailboxdefs

      sock_connect(name)  - returns an integer that is passed to sock_send

      sock_send( s, message ) - sends message to mailbox associated with s from sock_connect

      sock_write( s, data, n ) - sends data to mailbox associated with s from sock_connect

      sock_sel(message, l, p, n, tim, rd) - blocks on connections and messages.
        message is the buffer where a socket message will be put.
        l is the length of message (returned);
        p is an array of file descriptors that sock_sel will select on
        n is the length of p;
        tim is a timeout in seconds
        rd true means that sock_sel will read stdin
        sock_sel returns:
          -1 if a timeout.
          -2 if an error
          -3 if interrupted by a signal
          0 if standard in is ready for input
          or the file descriptor from 'p' that is ready.
          or the file descriptor of the socket that filled in message.

      sock_ssel(s, ns, message, l, p, n, tim, rd) - Same as sock_sel, but
	does not block on messages for the sockets in array, s (sock_connect 
	return values).
	ns is the length of s
	rest of parameters same as sock_sel.

      sock_close(name) - close opened connection - if NULL close all

      sock_name(last_msg()) - returns a status string pointing to the mbname

      sock_find(name) - returns handle for name

      sock_only(handle, message, tim) - blocks only on handle or timeout

      sock_only_f(handle, message, ftime) - same but timeout is floating point

      sock_poll(handle, message, tim, plen) - polls one socket and returns len

      sock_fd(handle) - returns a fd associated with handle 

      sock_intr(flag) - sock_sel will return when select is interrupted if flag is true.

      sock_bufct(n) - set out-of-sync size normally 4096
*/

static struct DEST  sock_ano   = { 0, 0, "localhost", "ANONYMOUS", 0, { 0, 0, 0 } };

       struct BOUND sock_bound = { 0, 0, 0, 4096, &sock_ano, NULL, {0} };


struct BOUND *bs = &sock_bound;
struct SOCK sock_kbd = { 0, 0, NULL, &sock_ano, {0} };
struct SOCK *last_conn;


/*
    sock_bind is used to establish a destination socket that is defined in the mailboxdefs file

    you should only call this once.
*/
struct BOUND *sock_bind(mbname)
char *mbname;
{
  int i, on, count, anon;
  struct hostent *hploc;
  char host[80];

  if( bs->bind_fd >0 )
  {
    return(bs);							/* Sock_bind was already called */
  }

  anon = strcmp(mbname, "ANONYMOUS") == 0;

  if(anon)
  {
    bs->dest = &sock_ano;
  }
  else 
  if( (bs->dest = find_dest(mbname)) == NULL )
  {
    log_msg("sock_bind:unknown mailbox");

    exit(1);
  }
 
  if (strcmp(bs->dest->host, "localhost"))
  {
								/* Not explicitly "localhost".  
								   Check that this mailbox is owned by this host */
    gethostname(host, 80);					/* Current host name (w/o) domain */

    count = strlen(host);
    i     = strlen(bs->dest->host);

    if(strncmp(host, bs->dest->host, i < count ? i : count))
    {
      log_msg("sock_bind: This program is not running on %s", bs->dest->host );

      exit(1);
    }
  }

  if(!anon && (hploc = gethostbyname(bs->dest->host)) == NULL)
  {
    log_msg("sock_bind:can't find %s", bs->dest->host );

    return(0);
  }


  if((bs->bind_fd = socket( AF_INET, SOCK_STREAM, 0)) < 0)
  {
    log_perror("socket");

    return(0);
  }

  bs->sin.sin_port        = htons(bs->dest->dport);
  bs->sin.sin_family      = AF_INET;
  bs->sin.sin_addr.s_addr = INADDR_ANY;
  on                      = 1;

  setsockopt(bs->bind_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));

  count = 0;

  while(bind( bs->bind_fd, (void *)&bs->sin, sizeof(struct sockaddr_in) ) < 0)
  {
    log_perror("bind");						/* Someone already has this bound */

    if(++count > 20)						/* Try 20 times for former to exit*/
    {
      exit(0);							/* Forget it */
    }

    sleep(1);
  }

  if(count > 0)
  {
    log_msg("finally bound");
  }

  if(listen( bs->bind_fd, 5 ) < 0)
  {
    log_perror("listen");

    exit(0);
  }

  signal(SIGPIPE, SIG_IGN);

  return(bs);
}


/* sock_connect returns a pointer to a structure that can 
   be later passed to sock_send for sending a message
 */
struct SOCK *sock_connect(mbname)
char *mbname;
{
  struct SOCK *s;
  struct DEST *d;
  struct hostent *hp;
  int bflag = 1;
  extern struct DEST sock_ano;

  s = bs->head;

  if(strcmp( mbname, "ANONYMOUS" ))				/* Mailbox is not ANONYMOUS */
  {
    while(s) 
    {
      if( s->dest && strcmp( mbname, s->dest->dname )==0 )
      {
        return(s);	  					/* Already have done a sock_connect to this mailbox */
      }

      s = s->next;
    }

								/* It's a new sock_connect */
    if((d= find_dest(mbname)) == NULL) 
    {
      log_msg("sock_connect:unknown mailbox %s", mbname );

      return(0);
    }
  }
  else 
  {
								/* sock_connect to ANONYMOUS (done by accept_sock) */
								/* Look for an ANONYMOUS SOCK struct with inactive fd */
    while(s) 
    {
      if( s->dest && strcmp( mbname, s->dest->dname )==0 && s->fd < 0 )
      {
        break;							/* ANONYMOUS with an inactive file descriptor */
      }

      s = s->next;
    }

    d     = &sock_ano;
    bflag = 0;
  }

  if(!s)							/* Didn't find a SOCK for this mailbox, so allocate a new one */
  {
    s = (struct SOCK *)malloc(sizeof(struct SOCK));

    bzero(s, sizeof(struct SOCK ));

    s->fd    = -1;
    s->dest  = d;
    s->next  = bs->head;
    bs->head = s;
  }

  if(bflag)							/* if anon then it wont ever connect */
  {
    if((hp = gethostbyname(s->dest->host)) == NULL) 
    {
      log_msg("sock_connect:can't find host");

      return(0);
    }

    s->sin.sin_family = hp->h_addrtype;				/* Will always be AF_INET */

    bcopy( hp->h_addr, &s->sin.sin_addr, hp->h_length );	/* IP number */

    s->sin.sin_port   = htons(s->dest->dport);			/* Port number */
  }
 
  return(s);
}


int read_sock(s, msg, l)
struct SOCK *s;
char *msg;
int *l;
{
  unsigned long count, ct, net;
  int n;
  
  last_conn = &sock_kbd;

  bs = &sock_bound;

  if (s->dest->raw || bs->dest->raw)				/* It's a `raw' socket; read up to max */
  {
    if( (n = read( s->fd, msg, sock_bound.bufct-1 ))<=0)	/* Error return */
    {
//      log_perror("read_sock, connection closed");

      *l    = 0;

      close(s->fd);

      s->fd = -1;

      return(-2);
    }

    *l        = n;
    msg[n]    = '\0';
    last_conn = s;

    return(s->fd);
  }


  if( (n=read(s->fd, &net, sizeof(unsigned int) )) >0 )		/* Normal socket.  Read the byte count */
  {
    count = ntohl(net);						/* Change from network byte order to host byte order */

    if( count <= sock_bound.bufct )				/* (May have to do several reads to get all `count' bytes) */
    {
      ct = count;

      while((n = read(s->fd, &msg[count - ct], ct )) < ct && n > 0 )
      {
	ct -= n;
      }
    }
    else							/* count is too big */
    {
      log_msg("read count out of sync %d %d\n", count, net);
    }
  }

  *l = count;

  if(n < 0)
  {
    log_perror("read_sock, read");
  }

  if(n<=0 || count > sock_bound.bufct)
  {
    close(s->fd);

    s->fd = -1;

    return(-2);
  }

  msg[count] = '\0';
  last_conn  = s;

  return(s->fd);
}



int sock_print_fds()
{
  struct SOCK *s;

  s = bs->head;

  while(s) 
  {
    if(strlen(s->dest->dname))
    {
      log_msg("sock_print_fds: %d %s", s->fd, s->dest->dname);
    }

    s = s->next;
  }

  return(0);
}

/* 
   findSocketsButThese() -- Call this function to fill in an array of
   struct SOCK *s values whose sock name does not contain the 'name'
   pasted parameter. 

   This is useful for filling in an array of bound sockets that you are
   NOT interested in so you can sock_ssel()  on bound sockets for those
   you are interested in.

   Use the results of this call, rss & rssn, and pass to a sock_ssel() call.

   See  sock_ssel() below for more info.

 */
int findSocketsButThese(name, rss, rssn)
char *name;
struct SOCK *rss[];
int *rssn;
{
  int n=0;
  struct SOCK *s;

  if(!name)
  {
    log_msg("findSocketsButThese(): Name = null");
    return(0);
  }

  s = bs->head;
  while(s) 
  {
    if(strlen(s->dest->dname))
    {
      if(strstr(s->dest->dname, name))
      {
//        log_msg("Found a match: %s; Ignoring", s->dest->dname);
      }
      else
      {
//        log_msg("Found a NON-match: %s; Adding", s->dest->dname);
        rss[n++] = s;
      }
    }

    s = s->next;
  }

  *rssn = n;

  return(0);
}



int sock_close(name)
char *name;
{
  struct SOCK *s, *p;

  if(name)							/* close name and de-queue */
  {
    s = bs->head;
    p = NULL;

    while(s) 
    {
      if( s->dest && strcmp( name, s->dest->dname )==0 )
      {
        break;
      }

      p = s;
      s = s->next;
    }

    if(s)
    {
      close(s->fd);

      if( p )
      {
        p->next = s->next;
      }
      else
      {
        bs->head = NULL;
      }

      free(s);
    }
  }
  else								/* close all */
  {
    s = bs->head;

    while(s)
    {
      close( s->fd );

      p = s->next;

      free(s);

      s = p;
    }

    close(bs->bind_fd);

    bs->bind_fd = -1;
    bs->dest = NULL;
  }

  return(0);
}


/* Return the *sock struct of last message received */
struct SOCK *last_msg()
{ 
  return(last_conn);
}


/* Given a struct *sock, return connection name */
char *sock_name(handle)
struct SOCK *handle;
{
  return( handle ? handle->dest->dname: NULL );
}


/* Given a *sock struct, return the fd */
int sock_fd(handle)
struct SOCK *handle;
{
  return( handle ? handle->fd: -1 );
}


int sock_bufct(bufct)
int bufct;
{
  if(bufct)
    sock_bound.bufct = bufct;

  return(bufct);
}


struct SOCK *sock_find(mbname)
char *mbname;
{
  struct SOCK *s;

  s = bs->head;

  while(s) 
  {
    if( s->dest && strcmp( mbname, s->dest->dname )==0 )
    {
      return(s);
    }

    s = s->next;
  }

  return(0);
}


/* Set / unSet interrupt flag */
int sock_intr(flag)
int flag;
{ 
  sock_bound.interrupt = flag;

  return(0);
};



/* blocks for a message on all bound sockets
     p is a pointer to an array of additional sockets to block on.
     n is the length of p.
      returns -1 on timeout.
              or the socket number on success
     timeout is in seconds
     the size of message is up to the caller
     returns without reading when the 'p' descriptors are detected
*/
int sock_sel(message, l, p, n, tim, rd_in)
char *message;
int *l;
int *p;
int n, tim, rd_in;
{
  return(sock_ssel(NULL, 0, message, l, p, n, tim, rd_in));
}


/* blocks for a message on all bound sockets except those listed in ss.
     ss is a pointer to an array of sockets (sock_connect return values)
     ns is the length of ss
     p is a pointer to an array of additional sockets to block on.
     n is the length of p.
      returns -1 on timeout.
              or the socket number on success
     timeout is in seconds; if <0, does a poll only (returns 0 if nothing ready)
     the size of message is up to the caller
     returns without reading when the 'p' descriptors are detected
*/
int sock_ssel(ss, ns, message, l, p, n, tim, rd_in)
struct SOCK *ss[];
int ns;
char *message;
int *l;
int *p;
int n, tim, rd_in;
{
  fd_set rfd;
  int sel, i;
  static struct timeval timeout;
  static int rd_dead = 0;
  struct timeval *pt;
  struct SOCK *s;

  if( !message && tim >= 0)
  {
    return(-3);
  }

  while(1)
  {
    FD_ZERO(&rfd);

    if( rd_in && !rd_dead )
    {
      FD_SET(0, &rfd );
    }

    if(bs->bind_fd > 0 )
    {
      FD_SET(bs->bind_fd, &rfd );
    }

    for(s=bs->head; s; s = s->next )
    {
      if( s->fd > 0 )
      {
        for(i=0; i < ns && ss[i] != s; i++ )
	{
	  continue;
	}

	if (i >= ns)
	{
          FD_SET( s->fd, &rfd );
	}
      }
    }

    if( p && n > 0 )
    {
      for(i=0; i<n; i++ )
      {
        FD_SET( p[i], &rfd );
      }
    }

    if(tim > 0 )
    {
      timeout.tv_sec = tim;
      timeout.tv_usec = 0; 
      pt = &timeout;
    } 
    else 
    if( tim == 0 )
    {
      pt = NULL;
    }
    else 
    {
      timeout.tv_sec = 0;		/* Do a poll only */
      timeout.tv_usec = 0; 
      pt = &timeout;
    }

    sel = select( FD_SETSIZE, &rfd, NULL, NULL, pt );

    if(sel < 0) 
    {
      if(errno != EINTR)
      {
        log_perror("select");
      }
      else 
      if(bs->interrupt)
      {
        return(-3);
      }

      continue;
    }

    if(tim < 0)
    {
      return sel;			/* Was only polling */
    }

    if( sel == 0 )
    {
      return(-1);
    }

    if( bs->bind_fd > 0 && FD_ISSET(bs->bind_fd, &rfd ))
    {
      accept_sock();
    }

    if( p && n > 0 )
    {
      for(i=0; i<n; i++ )
      {
        if(FD_ISSET( p[i], &rfd ))
	{
          return(p[i]);
	}
      }
    }

    for(s=bs->head; s; s = s->next )
    {
      if( s->fd > 0 ) 
      {
        if( FD_ISSET( s->fd, &rfd ))
	{
          if((sel = read_sock(s, message, l)) > 0 )
	  {
            return(sel);
	  }
	}
      }
    }

    if(rd_in && ! rd_dead && FD_ISSET(0, &rfd ))
    {
      last_conn = &sock_kbd;

      if( (i=read(0, message, 256 ))<=0)
      {
        rd_dead = 1;
        return(-2);
      }

      *l = i;

      message[i] = '\0';

      return(0);
    }
  }

  return(0);
}



int sock_fastsel(message, l, p, n, tim, rd_in)
char *message;
int *l;
int *p;
int n; 
struct timeval *tim;
int rd_in;
{
  fd_set rfd;
  int sel, i;
  static int rd_dead = 0;
  struct SOCK *s;

  if( !message )
  {
    return(-3);
  }

  while(1) 
  {
    FD_ZERO(&rfd);

    if( rd_in && !rd_dead )
    {
      FD_SET(0, &rfd );
    }

    if(bs->bind_fd > 0 )
    {
      FD_SET(bs->bind_fd, &rfd );
    }

    for(s=bs->head; s; s = s->next ) 
    {
      if( s->fd > 0 ) 
      {
        FD_SET( s->fd, &rfd );
      }
    }

    if( p && n > 0 )
    {
      for(i=0; i<n; i++ )
      {
        FD_SET( p[i], &rfd );
      }
    }

    sel = select( FD_SETSIZE, &rfd, NULL, NULL, tim );

    if(sel < 0 ) 
    {
      if( errno != EINTR)
      {
        log_perror("select");
      }
      else
      if( bs->interrupt )
      {
        return(-3);
      }

      continue;
    }

    if( sel == 0 )
    {
      return(-1);
    }

    if( bs->bind_fd > 0 && FD_ISSET(bs->bind_fd, &rfd ))
    {
      accept_sock();
    }

    if( p && n > 0 )
    {
      for(i=0; i<n; i++ )
      {
        if(FD_ISSET( p[i], &rfd ))
	{
          return(p[i]);
	}
      }
    }

    for(s=bs->head; s; s = s->next )
    {
      if( s->fd > 0 ) 
      {
        if( FD_ISSET( s->fd, &rfd ))
	{
          if((sel = read_sock(s, message, l))>0 )
	  {
            return(sel);
	  }
	}
      }
    }

    if(rd_in && ! rd_dead && FD_ISSET(0, &rfd )) 
    {
      last_conn = &sock_kbd;

      if((i=read(0, message, 256 ))<=0) 
      {
        rd_dead = 1;
        return(-2);
      }

      *l         = i;
      message[i] = '\0';

      return(0);
    }
  }

/* NOT REACHED */

  return(0);
}



/* Wait on the single named socket only; ignoring all others */
int sock_only(handle, message, tim )
struct SOCK *handle;
char *message;
int tim;
{
  return sock_only_f(handle, message, (double)tim);
}


/* sock_only_f is the same as sock_only but with a floating point time out */
int sock_only_f(handle, message, ftime)
struct SOCK *handle;
char *message;
double ftime;
{
  fd_set rfd;
  int sel, len;
  static struct timeval timeout;
  struct timeval *pt;

  if( !handle )
  {
    return(-3);
  }

  while(1) 
  {
    FD_ZERO(&rfd);

    if( bs->bind_fd > 0 )
    {
      FD_SET(bs->bind_fd, &rfd );
    }

    if( handle->fd > 0 )
    {
      FD_SET( handle->fd, &rfd );
    }

    if(ftime > 0 ) 
    {
       timeout.tv_sec = ftime;
       timeout.tv_usec = 1.e6*(ftime-timeout.tv_sec);
       pt = &timeout;

    }
    else
    if( ftime == 0 )
    {
      pt = NULL;
    }
    else
    {
      pt = &timeout;
    }

    if((sel = select( FD_SETSIZE, &rfd, NULL, NULL, pt )) < 0) 
    {
      if(errno == EINTR)
      {
        return(-3);
      }

      log_perror("select");

      continue;
    }

    if( sel == 0 )
    {
      return(-1);
    }

    if( bs->bind_fd > 0 && FD_ISSET(bs->bind_fd, &rfd ))
    {
      accept_sock();
    }

    if( handle->fd > 0 )
    {
      if( FD_ISSET( handle->fd, &rfd ))
      {
        if((sel = read_sock(handle, message, &len))>0 )
	{
          return(len); /* change return from 'sel' by twf 11/12/02 */
	}
      }
    }
  }

/* NOT REACHED */

  return(0);
}



/* Pool if the songle socket passed has pending messages; returns len */
int sock_poll(handle, message, plen )
struct SOCK *handle;
char *message;
int *plen;
{
  fd_set rfd;
  int sel;
  static struct timeval timeout;
  struct timeval *pt;

  *plen = 0;

  if( !handle )
  {
    return(-3);
  }

  while(1) 
  {
    FD_ZERO(&rfd);

    if( bs->bind_fd > 0 )
    {
      FD_SET(bs->bind_fd, &rfd );
    }

    if( handle->fd > 0 )
    {
      FD_SET( handle->fd, &rfd );
    }

    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;
    pt              = &timeout;

    if((sel = select( FD_SETSIZE, &rfd, NULL, NULL, pt )) < 0) 
    {
      if( errno == EINTR )
      {
        return(-3);
      }

      log_perror("select");

      continue;
    }

    if( sel == 0 )
    {
      return(-1);
    }

    if( bs->bind_fd > 0 && FD_ISSET(bs->bind_fd, &rfd ))
    {
      accept_sock();
    }

    if( handle->fd > 0 )
    {
      if( FD_ISSET( handle->fd, &rfd ))
      {
        if((sel = read_sock(handle, message, plen))>0 )
	{
          return(sel);
	}
      }
    }
  }

/* NOT REACHED */

  return(0);
}



int accept_sock()
{
  int  port;
  char dname[4096];
  struct SOCK sock, *s;
  struct DEST *d;
  unsigned long count, ct, net;
  int n, raw;
  struct sockaddr_in addr;
  struct hostent *hp;
  int addrlen;

  addrlen = sizeof(struct sockaddr_in);

  if((sock.fd = accept( bs->bind_fd, (struct sockaddr *)&addr, (socklen_t *__restrict)&addrlen ))<0)
  {
    log_perror("accept_sock, accept");

    return(1);
  }

  raw = bs->dest->raw;						/* Non-zero if we are a raw mailbox */
								/* Find out if the new connecton is from a `raw' socket mailbox */
  port = ntohs(addr.sin_port);					/* Port that has connected */

  do
  {
    d = find_port(port);

    if(d)							/* Has the same port #. See if it also has the same IP address */
    {
      hp = gethostbyname(d->host);

      if(hp && strncmp(hp->h_addr_list[0], (char *)&addr.sin_addr.s_addr, hp->h_length) == 0)
      {
	raw = raw || d->raw;					/* Found it! */

	break;
      }

      port = 0;							/* Continue the search for that port */
    }
  }while(d);

  if(raw)							/* Raw socket mailbox */
  {
    if(d) 
    {
      strcpy(dname, d->dname);					/* Fetch the mailbox name */
    }
    else
    {
      strcpy(dname, "ANONYMOUS");				/* Didn't find it.  Make it ANONYMOUS */
    }

    count = strlen(dname);
  }
  else								/* Normal mailbox */
  {
								/* can't use read_sock because I want a timeout on read */
    if( (n=readtm(sock.fd, (char *)&net, sizeof(unsigned int) )) >0 )
    {
      count = ntohl(net);

      if( count <= bs->bufct )
      {
	ct = count;

	while( (n=readtm(sock.fd, (char *)&dname[count - ct], ct )) < ct && n>0 )
        {
	  ct -= n;
        }
      }
      else
      {
	log_msg("read count out of sync\n");
      }
    }

    if(n<0)
    {
      log_perror("read_sock, read");
    }

    if(n<=0 || count > bs->bufct)
    {
      close(sock.fd);

      sock.fd = -1;

      return(1);
    }

    dname[count] = '\0';
  }

  if((s = (struct SOCK *)sock_connect(dname)) == 0)
  {
    s = (struct SOCK *)sock_connect("ANONYMOUS"); 
  }

  if(s->sin.sin_port == 0)
  {
    s->sin = addr;						/* ANONYMOUS socket.  Use values from accept() */
  }

  if( s->fd >= 0 )
  {
    close(s->fd);
  }

  s->fd = sock.fd; 

  return(0);
}



/* just like read with a 5 second timeout */
int readtm( fd, buf, len )
int fd;
char *buf;
int len;
{
  fd_set rfd;
  int sel;
  struct timeval timeout;

  FD_ZERO(&rfd);
  FD_SET( fd, &rfd );

  timeout.tv_sec = 5;
  timeout.tv_usec = 0; 

  if( (sel = select( FD_SETSIZE, &rfd, NULL, NULL, &timeout )) <0 )
  {
    log_perror("select");

    return(-3);
  }

  if( sel == 0 )
  {
    return(-1);
  }

  return(read( fd, buf, len ));
}
