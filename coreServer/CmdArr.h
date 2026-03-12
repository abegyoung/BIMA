
/// Lengths of our command fields.
#define CMDARR_MAX_ARGS 10
#define CMDARR_NAME_LEN 64
#define CMDARR_DESCRIP_LEN 128
#define CMDARR_BUFLEN 1024

/// We have argument flags which describe each argument.
#define CMDARR_ARGF_NONE    0  // No restrictions.
#define CMDARR_ARGF_REQ     1  // Required.
#define CMDARR_ARGF_NUM     2  // Must be numeric.
#define CMDARR_ARGF_RANGE   4  // Take the two items given as an incl. range.
#define CMDARR_ARGF_LIST    8  // Take the items given as a list.

/// A structure which holds information about each argument.
struct arginfo
{
  char name[CMDARR_NAME_LEN];
  unsigned int flags;
  int boundc;
  char bounds[CMDARR_MAX_ARGS][CMDARR_MAX_ARGS];
};

/// A structure that contains information on the commands. These can be placed
/// in an array and iterated over.
struct cmd
{
  char name[CMDARR_NAME_LEN];
  int ( *func )( int argc, char **argv, int fdout, int fderr );
  char descrip[CMDARR_DESCRIP_LEN];
  int argc;
  struct arginfo argl[CMDARR_MAX_ARGS];
};

int cmdarr_write( int fd, const char *msg, const struct cmd *c1 );
struct cmd *cmdarr_name( const struct cmd *c1, const char *name );
struct arginfo *cmd_arg( struct cmd *c1, const char *arg );
