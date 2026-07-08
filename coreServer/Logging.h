#include <syslog.h>

void log_init( const char *name );
void log_server_msg( int priority, const char *format, ... );
