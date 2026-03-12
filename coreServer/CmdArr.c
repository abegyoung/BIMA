#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "CmdArr.h"


/// Returns 0 if c1 is not the array terminator, non-zero otherwise.
int is_cmdarr_null( const struct cmd *c1 )
{
  return ( c1 == NULL ) ? ( 1 ) : ( ( c1->func == NULL ) ? ( 1 ) : ( 0 ) );
}

/// Write out the cmd array to fd, preceeded by msg if it is not NULL.
int cmdarr_write( int fd, const char *msg, const struct cmd *c1 )
{
  char buf[CMDARR_BUFLEN];
  if ( msg && strlen( msg ) )
  {
    snprintf( buf, CMDARR_BUFLEN, "%s\n", msg );
    write( fd, buf, strlen( buf ) );
  }

  int err = 0;
  while ( !is_cmdarr_null( c1 ) && err >= 0 )
  {
    // Basic command info.
    if (( err = snprintf( buf, CMDARR_BUFLEN, "name = '%s'\n", c1->name )) < 0 )
      break;
    write( fd, buf, strlen( buf ) );
    if (( err = snprintf( buf, CMDARR_BUFLEN, "  description = '%s'\n", c1->descrip )) < 0 )
      break;
    write( fd, buf, strlen( buf ) );

    // Info for the args.
    if ( c1->argc > 0 )
    {
      if ( ( err = snprintf( buf, CMDARR_BUFLEN, "  %d args: ", c1->argc ) ) < 0 )
        break;
      for ( int gg = 0; gg < c1->argc; gg++ )
      {
        strcat( buf, " " );
        strcat( buf, c1->argl[gg].name );
        if ( c1->argl[gg].flags == CMDARR_ARGF_NONE )
          strcat( buf, " <ANY OK>");
        if ( c1->argl[gg].flags & CMDARR_ARGF_REQ )
          strcat( buf, " <REQ>");
        if ( c1->argl[gg].flags & CMDARR_ARGF_NUM )
          strcat( buf, " <NUM>");
        if ( c1->argl[gg].flags & CMDARR_ARGF_RANGE )
        {
          strcat( buf, " <RANGE {" );
          strcat( buf, c1->argl[gg].bounds[0] );
          strcat( buf, ".." );
          strcat( buf, c1->argl[gg].bounds[1] );
          strcat( buf, "}>" );
        }
        else if ( c1->argl[gg].flags & CMDARR_ARGF_LIST )
        {
          int boundc = c1->argl[gg].boundc;
          boundc = ( boundc < CMDARR_MAX_ARGS ? boundc : CMDARR_MAX_ARGS );
          if ( boundc == 0 )
            strcat( buf, " <LIST {<EMPTY>}>" );
          else
          {
            strcat( buf, " <LIST {" );
            for ( int bb = 0; bb < boundc; bb++ )
            {
              strcat( buf, c1->argl[gg].bounds[bb] );
              if ( bb < boundc-1 )
                strcat( buf, "," );
            }
            strcat( buf, "}>" );
          }
        }
      }
      strcat( buf, "\n" );
      write( fd, buf, strlen( buf ) );
    }
    c1++;
  }
  return err;
}

/// Returns length of c1.
int cmdarr_len( struct cmd *c1 )
{
  int ii = 0;
  while ( !is_cmdarr_null( c1+ii ) )
    ii++;
  return ii;
}

/// Sets c1 to be the cmd array terminator.
struct cmd *cmdarr_null( struct cmd *c1 )
{
  memset( c1->name, 0, CMDARR_NAME_LEN );
  memset( c1->descrip, 0, CMDARR_DESCRIP_LEN );
  c1->func = NULL;
  c1->argc = 0;
  memset( c1->argl, 0, CMDARR_MAX_ARGS * sizeof( struct arginfo ) );
  return c1;
}

/// Copies cmd array c2 into cmd array c1. Assumes terminator at end.
struct cmd *cmdarr_cpy( struct cmd* c1, const struct cmd* c2 )
{
  int ii = 0;
  while ( !is_cmdarr_null( c2+ii ) )
  {
    strcpy( (c1+ii)->name, (c2+ii)->name );
    strcpy( (c1+ii)->descrip, (c2+ii)->descrip );
    (c1+ii)->func = (c2+ii)->func;
    (c1+ii)->argc = (c2+ii)->argc;
    memcpy( (c1+ii)->argl, (c2+ii)->argl, CMDARR_MAX_ARGS * sizeof( struct arginfo ) );
    ii++;
  }
  return c1;
}

/// Returns a pointer to the first occurrence of name in cmd array c1.
/// If not found, return NULL.
struct cmd *cmdarr_name( const struct cmd *c1, const char *name )
{
  while ( !is_cmdarr_null( c1 ) && strcmp( c1->name, name ) != 0 )
    c1++;
  return ( is_cmdarr_null( c1 ) ) ? ( NULL ) : ( ( struct cmd * )( c1 ) );
}

/// Returns a pointer to the first occurrence of arg name in cmd.
/// If not found, return NULL.
struct arginfo *cmd_arg( struct cmd *c1, const char *arg )
{
  for ( int aa = 0; aa < c1->argc; aa++ )
  {
    // strcmp will be 0 if it is found (don't compare the '--' at the start).
    if ( strcmp( arg, c1->argl[aa].name ) == 0 )
      return &( c1->argl[aa] );
  }
  return NULL;
}

