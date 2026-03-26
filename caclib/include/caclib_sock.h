#ifndef SOCK_H
#define SOCK_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


/* define SOCK_DEF_BUFCT 4096  */   /* default buffer count             */
/* define SOCK_SEL_STDIN    0  */   /* message came from standard input */
/* define SOCK_SEL_TIMEOUT -1  */   /* socket timeout occurred          */
/* define SOCK_SEL_ERROR   -2  */   /* socket error occurred            */
/* define SOCK_SEL_SIGNAL  -3  */   /* interrupted by a signal          */
/* define SOCK_ONLY_OK       0 */
/* define SOCK_ONLY_TIMEOUT -1 */
/* define SOCK_ONLY_ERROR   -2 */
/* define SOCK_ONLY_SIGNAL  -3 */
/* define SOCK_INTR_RETRY    0 */
/* define SOCK_INTR_NO_RETRY 1 */



struct BOUND {
	int                bind_fd;
	int                interrupt;
	int		   pad;
	unsigned int       bufct;

	struct DEST       *dest;
	struct SOCK       *head; 		/* head of list */
	struct sockaddr_in sin;

	int                isxview; /* true if xview */
//	int                isaio;   /* true if aioread is turned on (Solaris2) */
};

struct SOCK {
	int                fd;
	int		   pad;

	struct SOCK       *next;
	struct DEST       *dest;
	struct sockaddr_in sin;
};

#endif

