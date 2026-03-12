
/// Lengths of our command fields.
#define DISPATCH_OK 0
#define DISPATCH_ERR 1
#define DISPATCH_QUIT 2

#define DISPATCH_MAX_SIZE 256
#define DISPATCH_BUFLEN 1024

int dispatch( char *cmdbuf, int fdout, int fderr, const struct cmd *cmds );
int readline( int fdin, char *readbuf, int readsz, int *readp, char *linebuf);
