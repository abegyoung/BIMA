#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <libgen.h>

#include <pthread.h>

#define MAX_CLIENTS 10

#define DELAY_S 0
#define DELAY_U 100000
#define ERROR 1
#define CONN_BUFLEN 1024
#define ACTIVE 0
#define INACTIVE 1
#define HIGH 1
#define LOW 0

struct conn
{
  int fdin;
  int fdout;
  int fderr;
  int readp;
  char readbuf[CONN_BUFLEN];
};

extern int destination;
extern pthread_mutex_t destination_lock;
