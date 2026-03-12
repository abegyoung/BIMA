#include <stdarg.h>         // va_list
#include <stdio.h>
#include <stdlib.h>
#include "Logging.h"

/// Initialize the logging mechanism for this server.
void log_init( const char *name )
{
  openlog( name, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1 );
}

/// Log a message of a certain priority using a format similar to printf.
/// Priorities:
/// LOG_ERR        error conditions
/// LOG_WARNING    warning conditions
/// LOG_NOTICE     normal, but significant, condition
/// LOG_INFO       informational message
/// LOG_DEBUG      debug-level message
void log_msg( int priority, const char *format, ... )
{
  va_list args;
  va_start( args, format );
  vsyslog( priority, format, args );
  va_end( args );
}

int writeState(const char *filename, int num)
{
  FILE *fd;
  if((fd = fopen(filename, "w")))
    {
      fprintf(fd, "%d", num);
      fclose(fd);
    }
  return 0;
}

int readState(const char *filename)
{
  FILE *fd;
  int num=-1;
  if((fd = fopen(filename, "r")))
    {
      fscanf(fd, "%d", &num);
      fclose(fd);
    }
  return num;
}
