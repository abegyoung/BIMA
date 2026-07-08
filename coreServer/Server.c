#include "Server.h"
#include "Logging.h"
#include "CmdArr.h"
#include "Dispatch.h"
#include "hardware/can.h"

#include <pthread.h>

sig_atomic_t pipe_deadflag = 0;
extern struct cmd commands[ ];
extern int server_initialize(void);
extern int server_shutdown(void);


void pipe_handler (int signum) {
    pipe_deadflag = 1;
}

/// Write out the conn array to fd, preceeded by msg if it is not NULL.
int connarr_write( int fd,
                   const char *msg,
                   const struct conn *c1,
                   const int count )
{
  char buf[CONN_BUFLEN];
  if ( msg && strlen( msg ) )
  {
    snprintf( buf, CONN_BUFLEN, "%s\n", msg );
    write( fd, buf, strlen( buf ) );
  }

  int err = 0;
  for ( int cc = 0; cc < count; cc++ )
  {
    if ( ( err = snprintf( buf, CONN_BUFLEN,
        "#%d: fdin = %d, fdout = %d, fderr = %d, readp = %d, readbuf='%s'\n",
        cc, c1[cc].fdin, c1[cc].fdout, c1[cc].fderr, c1[cc].readp,
        c1[cc].readbuf ) ) < 0 )
      break;
    write( fd, buf, strlen( buf ) );
  }
  return err;
}


/// Returns an index to the first occurrence of fdin in conn array c1.
/// If not found, return -1.
int connarr_fdin( const int fdin,
                  const struct conn *c1,
                  const int count )
{
  for ( int cc = 0; cc < count; cc++ )
    if ( c1[cc].fdin == fdin )
      return cc;
  return -1;
}

/// Returns an fdset with all the fdins in this conn array.
fd_set connarr_fdin_set( struct conn* c1,
                         const int count )
{
  fd_set conn_set;
  FD_ZERO( &conn_set );
  for ( int cc = 0; cc < count; cc++ )
    if ( c1[cc].fdin >= 0 )
      FD_SET( c1[cc].fdin, &conn_set );
  return conn_set;
}

/// Clears all connections (sets to a default).
void connarr_init( struct conn* c1,
                   const int count )
{
  for ( int cc = 0; cc < count; cc++ )
  {
    c1[cc].fdin = -1;
    c1[cc].fdout = -1;
    c1[cc].fderr = -1;
    c1[cc].readp = 0;
    memset( c1[cc].readbuf, 0, CONN_BUFLEN );
  }
}

/// Insert a new fd into this list if there is room.
int connarr_insert( const int fdin,
                    struct conn *c1,
                    const int count )
{
  for ( int cc = 0; cc < count; cc++ )
  {
    if ( c1[cc].fdin < 0 )
    {
      c1[cc].fdin = fdin;
      c1[cc].fdout = fdin;
      c1[cc].fderr = fdin;
      c1[cc].readp = 0;
      return cc;
    }
  }
  return -1;
}

/// Close an fd in this list.
void connarr_close( const int fdin,
                    struct conn *c1,
                    const int count )
{
  for ( int cc = 0; cc < count; cc++ )
  {
    if ( c1[cc].fdin == fdin )
    {
      if ( c1[cc].fdin >= 0 )
        close( c1[cc].fdin );
      c1[cc].fdin = -1;
      c1[cc].fdout = -1;
      c1[cc].fderr = -1;
      c1[cc].readp = 0;
      memset( c1[cc].readbuf, 0, CONN_BUFLEN );
    }
  }
}

// Establish the tcp/ip socket server
int start_conn(int server_port)
{
  int err = 0;
  // Buffer to hold the complete lines.

  // Assign a signal handler for SIGPIPE, if a client dies.
  struct sigaction pipe_action;
  memset( &pipe_action, 0, sizeof ( pipe_action ) );
  pipe_action.sa_handler = &pipe_handler;
  sigaction( SIGPIPE, &pipe_action, NULL );

  // Open a TCP socket (an Internet stream socket).
  int server_fd = -1;
  if ( ( server_fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
  {
    err = errno;
    perror("socket: ");
    log_server_msg( LOG_ERR, "::%s::socket (err=%d) %s",
        __FUNCTION__, err, strerror( err ) );
    exit(1);
  }

  int optval = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (void *)(&optval), sizeof(int)) < 0)
  {
    err = errno;
    perror("setsocketopt: ");
    log_server_msg( LOG_ERR, "::%s::setsockopt (err=%d) %s",__FUNCTION__, err, strerror( err ) );
    exit(1);
  }
  
  // Bind our local address so that the client can send to us.
  struct sockaddr_in serv_addr;
  bzero( (char *) &serv_addr, sizeof(serv_addr) );
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons( server_port );

  if ( bind( server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr) ) < 0 )
  {
    err = errno;
    perror("bind: ");
    log_server_msg( LOG_ERR, "::%s::bind (err=%d) %s",__FUNCTION__, err, strerror( err ) );
    exit(1);
  }

  if ( listen( server_fd, 5 ) < 0 )
  {
    err = errno;
    perror("listen: ");
    log_server_msg( LOG_ERR, "::%s::listen (err=%d) %s",__FUNCTION__, err, strerror( err ) );
    exit(1);
  }
  log_server_msg( LOG_INFO, "::%s listening on port %d!", __FUNCTION__, server_port );
  return(server_fd);
}


/// Manage and handle connections to this server.
int manage_conn( int server_fd, const struct cmd *cmds )
{
  int err = 0;
  char linebuf[DISPATCH_BUFLEN];

  log_server_msg( LOG_INFO, "::%s Initializing connection list.", __FUNCTION__ );
  struct conn client_list[MAX_CLIENTS];
  connarr_init( client_list, MAX_CLIENTS);

  while (1)
  {
    // clear the server and client incoming sets with the server fd.
    fd_set server_set = connarr_fdin_set( client_list, MAX_CLIENTS );
    FD_SET( server_fd, &server_set );

    // set the timeout value
    struct timeval delay;
    delay.tv_sec  = DELAY_S;
    delay.tv_usec = DELAY_U;

    // wait for an event to occur, at 'delay' timeout
    if ( select( FD_SETSIZE, &server_set, NULL, NULL, &delay ) )
      {
	// Is a new client is connecting?
	if ( FD_ISSET( server_fd, &server_set ) )
	  {
	    struct sockaddr_in cli_addr;
	    int clilen = sizeof (cli_addr);
	    int client_fd = accept( server_fd, (struct sockaddr *) &cli_addr,
				    (socklen_t *)&clilen);
	    if ( client_fd >= 0 )
	      {
		// Find the first entry in the array we can use.
		int newc = connarr_insert( client_fd, client_list, MAX_CLIENTS );
		// Could we find an entry? If not, we are maxed out.
		if ( newc == -1 )
		  {
		    log_server_msg( LOG_ERR, "::%s Maximum load... denied client.", __FUNCTION__ );
		    close( client_fd );
		  }
		else
		  {
		    log_server_msg( LOG_INFO, "::%s Opened connection fd #%d.",
			     __FUNCTION__, client_fd );
		  }
	      }
	  }
	// Do we have data arriving on the socket?
	for ( int cc = 0; cc < MAX_CLIENTS; cc++ )
	  {
	    if ( client_list[cc].fdin > -1 )
	      {
		if ( FD_ISSET( client_list[cc].fdin, &server_set ) )
		  {
		    // Read a line, did we get an error?
		    err = readline( client_list[cc].fdin,
				    client_list[cc].readbuf, DISPATCH_BUFLEN, &client_list[cc].readp,linebuf);
		    if ( err < 0 )
		      {
			log_server_msg( LOG_ERR, "::%s::readline (err=%d) message: %s",
				 __FUNCTION__, err, linebuf );
		      }
		    // Did the connection close?
		    else if ( err == 0 || pipe_deadflag )
		      {
			log_server_msg( LOG_ERR, "::%s connection closed (err=%d) fd #%d",
				 __FUNCTION__, err, client_list[cc].fdin );
			connarr_close( client_list[cc].fdin, client_list, MAX_CLIENTS );
			if(pipe_deadflag)  // reset the flag if needed
			  pipe_deadflag=0;
		      }
		    // Do we only have a partial line?
		    else if ( err > 0 && strlen( linebuf ) == 0 )
		      {
			log_server_msg( LOG_ERR, "::%s Only got a partial line (err=%d) fd #%d",
				 __FUNCTION__, err, client_list[cc].fdin );
		      }
		    // Did we get a complete line?
		    else
		      {
			err = dispatch( linebuf,client_list[cc].fdout, client_list[cc].fderr, cmds );
			if ( err == DISPATCH_ERR )
			  {
			    log_server_msg( LOG_ERR, "::%s Error dispatching line (err=%d) fd #%d",
				     __FUNCTION__, err, client_list[cc].fdin );
			  }
			else if ( err == DISPATCH_QUIT )
			  {
			    err=0;
			    log_server_msg( LOG_ERR, "::%s connection closed (err=%d) fd #%d",
				     __FUNCTION__, err, client_list[cc].fdin );
			    connarr_close( client_list[cc].fdin, client_list, MAX_CLIENTS );
			  }
		      }
		  }
	      }
	  }
      }
  }
  return 0;
}

int destination;
pthread_mutex_t destination_lock = PTHREAD_MUTEX_INITIALIZER;

int main (int argc, char **argv)
{
    char *p;
    int err=0, serv_port, fd;

    pthread_t tid;
    destination = 6;

    // Set Server struct initial values
    //server.BandSelect    = 1;	//1 MM Band Selected
    //server.YIGHarmonicM  = 8;	//8th harmonic ( 1MM is always 8th YIG Harmonic )
    //server.GunnHarmonicN = 9;	//9th Gunni harmonic ( The upper 1MM frequencies 237-282 GHz )
    //server.GunnFreq      = 241.80; //GHz
    //server.L_Band        = 1120.0; //MHz

    // Start CAN receiver thread
    if (pthread_create(&tid, NULL, can_receiver_thread, NULL) != 0) {
	    perror("pthread_create");
	    return 1;
    }
    
    log_init( basename( argv[0] ) );
    if(argc == 1)
      serv_port=9000;
    else
      {      
	serv_port = strtol(argv[1], &p, 10);
	if(*p != '\0' || serv_port > 9010 || serv_port < 9000)
	  {
	    log_server_msg( LOG_ERR, "Invalid port number %d\n", (int)serv_port);
	    return ERROR;
	  }
      }
    if((err = server_initialize()) )
      log_server_msg(LOG_ERR, "Error in server initialization\n");
    
    fd = start_conn((int)serv_port); 
    err = manage_conn( fd, commands );

    if( (err = server_shutdown()) )
      log_server_msg(LOG_ERR, "Error in server shutdown\n");
    
    return err;
}
