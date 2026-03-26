/*
 * structure of mailboxdefs file
 */

/* every destination has a port # as defined by the mailboxdefs file */

#define MAXPORT  10 
#define MAXNAME  30
#define MAXDEST 256

struct DEST {
	int dport;				/* Port number */
	int ipad;

	char host[MAXNAME];			/* Computer that owns this mailbox */
	char dname[MAXNAME];			/* Mailbox name */

	short raw;				/* Non-zero if this is a `raw' socket */
	short spad[3];
};

//struct DEST *find_dest();
//struct DEST *find_port();
