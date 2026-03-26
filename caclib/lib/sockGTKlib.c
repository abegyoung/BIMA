/*
      Socket library gtk support is obtained by calling sock_gtk_bind();
      Under gtk do not call sock_sel, sock_ssel or sock_only as they
      will break the gtk library since they call select(). 
      sock_write and sock_send are replaced in this library so be sure to
      link this object module before including the cactus.a library.
      sock_connect works normally. 
      gtk timers should be used for timing.

      sock_gtk_bind( mbname, read_routine );
      read_routine is the routine that you want to be called when there is 
      data input from sockets. It has the following parameters:

       read_routine(handle, message, len )
         struct SOCK *handle;
         char *message;
         int len;
*/

#include <gtk/gtk.h>
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

#include "sdd.h"
#include "header.h"
#include "caclib_mbdef.h"
#include "caclib_sock.h"
#include "caclib_proto.h"

/* gtk support for applications that want gtk and sockets */

static gint tag;

static int (*gtk_read_routine)(struct SOCK *, char *, int) = NULL;
int sock_gtk_fd(struct SOCK *s, int fd, GdkInputCondition cond);

static int sock_gtk_accept(bs, bfd )
struct BOUND *bs;
int bfd;
{
  int len;
  char dname[2*4096];
  struct SOCK sock, *s;
  struct DEST sock_ano = { 0, 0, "localhost", "ANONYMOUS", 0, { 0, 0, 0 } };

  if((sock.fd = accept( bs->bind_fd, NULL, 0 ))<0) {
    log_perror("sock_gtk_accept, accept");
    return(1);
  }
                                                  /* can't call select in gtk */
  sock.dest = &sock_ano;
  if(read_sock(&sock,dname, &len) < 0){
    log_perror("sock_gtk_accept, sock_read");
    return(1);
  }

  if( (s = (struct SOCK *)sock_connect(dname)) == 0 )
    s = (struct SOCK *)sock_connect("ANONYMOUS"); 

  if( s->fd >= 0 && tag) {
    gdk_input_remove(tag);
    close(s->fd);
  }

  tag = gdk_input_add(sock.fd, GDK_INPUT_READ, (GdkInputFunction)sock_gtk_fd, (gpointer)s );

  s->fd = sock.fd; 

  return(0);
}


int sock_gtk_bind(mbname, read_routine)
char *mbname;
int (*read_routine)(struct SOCK *, char *, int);
{
  struct BOUND *pfd = (struct BOUND *)sock_bind(mbname);
  gtk_read_routine = read_routine;
  if ( pfd ) 
  {
    pfd->isxview = 1;

    tag = gdk_input_add(pfd->bind_fd, GDK_INPUT_READ, (GdkInputFunction)sock_gtk_accept, (gpointer)pfd );
  }

  signal( SIGPIPE, SIG_IGN );

  return(0);
}

int sock_gtk_fd(s, fd, cond)
struct SOCK *s;
int fd;
GdkInputCondition cond;
{
  extern struct BOUND sock_bound;
  static char *buf = NULL;
  int len;

  if(!buf)
    buf = (char *)malloc(sock_bound.bufct);

  if( read_sock( s, buf, &len) == -2 )
  {
    if(tag)
      gdk_input_remove(tag);
  }
  else
  if( gtk_read_routine )
  {
    (*gtk_read_routine)( s, buf, len );
  }

  return(0);
}

/*
  like sock_send but takes pointer and count as in write
*/

int sock_write( s, message, l )
struct SOCK *s;
char *message;
int l;
{
  int ret, err, len, flag = 0, net;
  unsigned int count;
  char *name;
  extern struct BOUND sock_bound;
  struct BOUND *bs = &sock_bound;

  if( (long int)s == -1 || !message || l <= 0 )
    return(0);

  if( !s ) {
    log_msg("no mailbox for: %s", message );
    return(0);
  }

  while( flag < 2 ) {
    if( s->fd < 0 ) {
      if( strcmp( s->dest->dname, "ANONYMOUS") == 0 )
        return(0);

      if( (s->fd = socket(s->sin.sin_family, SOCK_STREAM, 0 )) <0) {
        log_perror("sock_write, socket");
        return(0);
      }
      bind_any(s->fd);

      if( connect( s->fd, (struct sockaddr *)&s->sin, sizeof(struct sockaddr_in) ) <0 ) {
        close(s->fd);
        s->fd = -1;
        if(strlen(s->dest->dname))
        {
          char buf[80];

          sprintf(buf,"sock_write, connect(%s)", s->dest->dname);
          log_perror(buf);
        }
        else
          log_perror("sock_write, connect");

        return(0);
      }

      if( bs->isxview )
      {
        tag = gdk_input_add( s->fd, GDK_INPUT_READ, (GdkInputFunction)sock_gtk_fd, (gpointer)s );
      }

      if( !s->dest->raw && !bs->dest->raw) 
      {
        if( bs->bind_fd >0 ) 
        {
          count = strlen(bs->dest->dname);
          name =  bs->dest->dname;
        }
        else                         /* if he never bound then send ANONYMOUS */
        {
          count = 9;
          name = "ANONYMOUS";
        }

        net = htonl(count);
        if( (ret = write( s->fd, &net, sizeof(unsigned int)))>0 )
          ret = write( s->fd, name, count);
      }
    }

    count = l;

    if( !s->dest->raw && !bs->dest->raw)
    {
      net = htonl(count);
      ret = write( s->fd, &net, sizeof(unsigned int));
    }
    else
      ret = 1;  /* Raw socket always writes the data */

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
    if( getsockopt( s->fd, SOL_SOCKET, SO_ERROR, (char *)&err, (socklen_t *)&len ) < 0 || err != 0 )
      ret = count + 1;

    if( ret != count ) {
      if(ret <0 )
        log_perror("sock_write");
      log_msg("closing out mailbox: %s", s->dest->dname );
      close( s->fd);
      if( bs->isxview && tag)
        gdk_input_remove(tag);

      s->fd = -1;
      flag += 1;
   } else
      break;
  }
  return(1);
}



int sock_send( s, message )
struct SOCK *s;
char *message;
{
  return( sock_write( s, message, strlen(message)));
}

