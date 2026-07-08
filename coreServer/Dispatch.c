#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>          // strtod
#include <stdarg.h>
#include <ctype.h>           // isspace
#include <string.h>
#include <errno.h>
#include "Logging.h"
#include "CmdArr.h"
#include "Dispatch.h"

int tellUser(int fdout, const char *fmt, ...)
{
  char buf[DISPATCH_BUFLEN];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, DISPATCH_BUFLEN, fmt, ap);
  va_end(ap);
  write( fdout, buf, strlen(buf) );
  return(0);
}

/// Return the index to an arg in argv if found, -1 otherwise. skipc
/// will skip the starting dashes.
int argv_arg( int argc, char **argv, char *arg, int skipc )
{
  for ( int tt = 0; tt < argc; tt++ )
  {
    // strcmp will be 0 if it is found.
    if ( strcmp( arg, argv[tt]+skipc ) == 0 )
      return tt;
  }
  return -1;
}

/// Are all given argument names valid for this command? This means they
/// exist as an argument name for the command.
int all_args_valid( int argc, char **argv, struct cmd *cmdp, char *errmsg )
{
  // reset errmsg to be nothing.
  errmsg[0] = '\0';

  for ( int cc = 1; cc < argc; cc++ )
  {
    // All argument names start with "--".
    if ( strlen( argv[cc] ) > 2 && argv[cc][0] == '-' && argv[cc][1] == '-' )
    {
      struct arginfo *argi = NULL;
      // Try to find it in the command arg list (skip '--' at the front).
      if ( ( argi = cmd_arg( cmdp, (argv[cc]+2) ) ) == NULL )
      {
        snprintf( errmsg, DISPATCH_BUFLEN,
            "::%s Unknown argument '%s' for command '%s'\n",
            __FUNCTION__, (argv[cc]+2), cmdp->name );
        return DISPATCH_ERR;
      }
    }
  }
  return DISPATCH_OK;
}


/// Are all required argument names (along with values) in this command?
int all_required_args( int argc, char **argv, struct cmd *cmdp, char *errmsg )
{
  // reset errmsg to be nothing.
  errmsg[0] = '\0';

  // Are all the required arguments in the given command?
  for ( int aa = 0; aa < cmdp->argc; aa++ )
  {
    int arg_index = -1;
    // Is this argument required?
    if ( cmdp->argl[aa].flags & CMDARR_ARGF_REQ )
    {
      // Make sure the arg name is in the command in the right place
      if ( ( arg_index = argv_arg( argc, argv, cmdp->argl[aa].name, 2 ) ) != 2*aa+1 )
      {
	if(arg_index == -1)
	  {
	    snprintf( errmsg, DISPATCH_BUFLEN,
		      "::%s Missing required argument '%s' for command '%s'\n",
		      __FUNCTION__, cmdp->argl[aa].name, cmdp->name );
	    return DISPATCH_ERR;
	  }
	else  // it's there, just not in the required order
	  {
	    snprintf( errmsg, DISPATCH_BUFLEN,
		      "::%s Misordered argument '%s' for command '%s'\n",
		      __FUNCTION__, cmdp->argl[aa].name, cmdp->name );
	    return DISPATCH_ERR;
	  }
      }
      // Make sure there is a value associated with this argument.
      if ( argc <= arg_index + 1 )
      {
        snprintf( errmsg, DISPATCH_BUFLEN,
            "::%s Missing value for argument '%s' for command '%s'\n",
            __FUNCTION__, (argv[arg_index]+2), cmdp->name );
        return DISPATCH_ERR;
      }
    }
  }
  return DISPATCH_OK;
}

/// Is an argument's value is required to be a number?
int args_value_number( int argc, char **argv, struct cmd *cmdp, char *errmsg )
{
  // reset errmsg to be nothing.
  errmsg[0] = '\0';

  // Is an argument's value is required to be a number?
  for ( int cc = 1; cc < argc; cc++ )
  {
    // All argument names start with "--".
    if ( strlen( argv[cc] ) > 2 && argv[cc][0] == '-' && argv[cc][1] == '-' )
    {
      struct arginfo *argi = NULL;

      // Try to find it in the command arg list (skip '--' at the front).
      if ( ( argi = cmd_arg( cmdp, argv[cc]+2 ) ) == NULL )
      {
        snprintf( errmsg, DISPATCH_BUFLEN,
            "::%s Unknown argument '%s' for command '%s'\n",
            __FUNCTION__, (argv[cc]+2), cmdp->name );
        return DISPATCH_ERR;
      }

      // Check to see if the argument must be a number.
      if ( argi->flags & CMDARR_ARGF_NUM )
      {
        // Make sure there is a value associated with this argument.
        if ( argc <= cc + 1 )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Missing value for argument '%s' for command '%s'\n",
              __FUNCTION__, (argv[cc]+2), cmdp->name );
          return DISPATCH_ERR;
        }

        // If we can make it a double, it's a number.
        char *ptr = argv[cc+1];
        errno = 0;
        strtod( argv[cc+1], &ptr );

        // Any error at this point means this is blatently not a number,
        // but we must check to see if there are other details.
        if ( errno != 0 )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Argument '%s' value for command '%s' can't be interpreted as a number\n",
              __FUNCTION__, (argv[cc]+2), cmdp->name );
          return DISPATCH_ERR;
        }

        // If the pointer was not moved, no conversion was done, so we get 0.
        if ( ptr == argv[cc+1] )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Argument '%s' value for command '%s' could only be interpreted as zero\n",
              __FUNCTION__, (argv[cc]+2), cmdp->name );
          return DISPATCH_ERR;
        }

        // The pointer was moved, but if there are chars after the number,
        // we actually have digits prefixed on a text string. We know this
        // because we have already removed any whitespace.
        else if ( (int)( ptr - argv[cc+1] ) < strlen( argv[cc+1] ) )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Argument '%s' value for command '%s' has text as part of the number\n",
              __FUNCTION__, (argv[cc]+2), cmdp->name );
          return DISPATCH_ERR;
        }

      }
    }
  }
  return DISPATCH_OK;
}

// Make sure that arguments which should be in a range are in a range.
// This also implies they are numeric.
int args_value_range( int argc,
                      char **argv,
                      struct cmd *cmdp,
                      char *errmsg )
{
  // reset errmsg to be nothing.
  errmsg[0] = '\0';

  // Make sure that arguments which should be in a range are in a range.
  // This also implies they are numeric.
  for ( int cc = 1; cc < argc; cc++ )
  {
    // All argument names start with "--".
    if ( strlen( argv[cc] ) > 2 && argv[cc][0] == '-' && argv[cc][1] == '-' )
    {
      struct arginfo *argi = NULL;
      // Try to find it in the command arg list (skip '--' at the front).
      if ( ( argi = cmd_arg( cmdp, argv[cc]+2 ) ) == NULL )
      {
        snprintf( errmsg, DISPATCH_BUFLEN,
            "::%s Unknown argument '%s' for command '%s'\n",
            __FUNCTION__, (argv[cc]+2), cmdp->name );
        return DISPATCH_ERR;
      }
      // Check to see if the argument must be within a range.
      if ( argi->flags & CMDARR_ARGF_RANGE )
      {
        // Make sure there is a value associated with this argument.
        if ( argc <= cc + 1 )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Missing value for argument '%s' for command '%s'\n",
              __FUNCTION__, (argv[cc]+2), cmdp->name );
          return DISPATCH_ERR;
        }
        // If we can make it a double, it's a number.
        errno = 0;
        double val = strtod( argv[cc+1], NULL );
        if ( errno != 0 )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Value for argument '%s' for command '%s' must be a number\n",
              __FUNCTION__, (argv[cc]+2), cmdp->name );
          return DISPATCH_ERR;
        }
        // Now actually get the minimum.
        errno = 0;
        double min = strtod( argi->bounds[0], NULL );
        if ( errno != 0 )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Argument '%s' minimum for command '%s' is invalid\n",
              __FUNCTION__, (argv[cc]+2), cmdp->name );
          return DISPATCH_ERR;
        }
        // Now actually get the maximum.
        errno = 0;
        double max = strtod( argi->bounds[1], NULL );
        if ( errno != 0 )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Argument '%s' maximum for command '%s' is invalid\n",
              __FUNCTION__, (argv[cc]+2), cmdp->name );
          return DISPATCH_ERR;
        }
        // Actually perform the min check.
        if ( val < min )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Value '%s' for argument '%s' for command '%s' is less than than minimum '%s'\n",
              __FUNCTION__, argv[cc+1], (argv[cc]+2), cmdp->name, argi->bounds[0] );
          return DISPATCH_ERR;
        }
        // Actually perform the max check.
        if ( val > max )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Value '%s' for argument '%s' for command '%s' is greater than than maximum '%s'\n",
              __FUNCTION__, argv[cc+1], (argv[cc]+2), cmdp->name, argi->bounds[1] );
          return DISPATCH_ERR;
        }
      }
    }
  }
  return DISPATCH_OK;
}

// Make sure that arguments which should be in a list are in a list.
// This also implies they are NOT numeric.
int args_value_list( int argc, char **argv, struct cmd *cmdp, char *errmsg )
{
  // reset errmsg to be nothing.
  errmsg[0] = '\0';

  // Make sure that arguments which should be in a list are in a list.
  // This also implies they are NOT numeric.
  for ( int cc = 1; cc < argc; cc++ )
  {
    // All argument names start with "--".
    if ( strlen( argv[cc] ) > 2 && argv[cc][0] == '-' && argv[cc][1] == '-' )
    {
      struct arginfo *argi = NULL;
      // Try to find it in the command arg list (skip '--' at the front).
      if ( ( argi = cmd_arg( cmdp, (argv[cc]+2) ) ) == NULL )
      {
        snprintf( errmsg, DISPATCH_BUFLEN,
            "::%s Unknown argument '%s' for command '%s'\n",
            __FUNCTION__, (argv[cc]+2), cmdp->name );
        return DISPATCH_ERR;
      }
      // Check to see if the argument must be within a list.
      if ( argi->flags & CMDARR_ARGF_LIST )
      {
        // Make sure there is a value associated with this argument.
        if ( argc <= cc + 1 )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Missing value for argument '%s' for command '%s'\n",
              __FUNCTION__, (argv[cc]+2), cmdp->name );
          return DISPATCH_ERR;
        }
        // Now find a match in the bounds list.
        int found = -1;
        for ( int bb = 0; bb < argi->boundc; bb++ )
        {
          if ( strcmp( argv[cc+1], argi->bounds[bb] ) == 0 )
            found = bb;
        }
        if ( found < 0 )
        {
          snprintf( errmsg, DISPATCH_BUFLEN,
              "::%s Unlisted value '%s' for argument '%s' for command '%s'\n",
              __FUNCTION__, argv[cc+1], (argv[cc]+2), cmdp->name );
          return DISPATCH_ERR;
        }
      }
    }
  }
  return DISPATCH_OK;
}

/// Creates an arg list of the same form as the arguments
/// to main. If quotes appear in the line, what is between them is treated as
/// one token. This function will modify 'line', as 'argv' consists of a
/// series of pointers into the modified array.
/// \param line The line, possibly modified with a series of '\0' chars.
/// \param line_len The initial length of the line before modification.
/// \param max_argc The maximun number of args that 'argv' can point to.
/// \param argc Modified to refect the number of args in 'argv'.
/// \param argv The array of pointers to each of the arguments.
/// \return 0 if no errors occurred, -1 if the number of arguments were exceeded
/// or -2 if a quote was not properly closed. If there are too many arguments
/// or a dangling quote, the return values will either have as many aruments
/// that could be generated before the maximum was reached, or will assume that
/// the end of the line is the end of the quote.
int create_arg_list( char *line,
                     int line_len,
                     int max_argc,
                     int *argc,
                     char **argv )
{
  int err = 0;

  // Start from a known place.
  *argc = 0;
  memset( argv, 0, max_argc );

  char last_ch = '\0';
  int in_quote = 0;

  for ( int ii = 0; ii < line_len && err == 0; ii++ )
  {
    // Replace any spaces with the null char if we are not in a quote.
    if ( in_quote == 0 && isspace( (unsigned char)line[ii] ) )
    {
      last_ch = line[ii] = '\0';
    }
    // Otherwise, replace any quote with the null char if we are not in a quote
    // & set the quote flag.
    else if ( in_quote == 0 && line[ii] == '"' )
    {
      in_quote = 1;
      last_ch = line[ii] = '\0';
    }

    // Is the last char the null char but not the current one?
    if ( last_ch == '\0' && line[ii] != '\0' )
    {
      // Do we have too many args?
      if ( *argc >= max_argc )
      {
        err = -1;
      }
      // Otherwise, we have the start of a new token.
      else
      {
        argv[(*argc)++] = &line[ii];
        last_ch = line[ii];
      }
    }

    // Capture and remove the closing quote if it is here.
    if ( line[ii] == '"' )
    {
      last_ch = line[ii] = '\0';
      in_quote = 0;
    }
  }
  // Do we have a dangling quote?
  // Return that error if an error is not already set.
  return ( err != 0 ) ? ( err ) : ( ( in_quote != 0 ) ? ( -2 ) : ( 0 ) );
}

/// Wait for data on the file descriptors, return a full line or an err.
/// Returns -1 on error, linebuf holds the error message.
/// Returns 0 if the fd closed, linebuf will empty.
/// Returns linelen if we have a line, linebuf will hold the line.
/// Returns linelen if we have a partial line, linebuf will be empty.
int readline( int fdin, char *readbuf, int readsz, int *readp, char *linebuf)
{
  if ( fdin < 0 )
  {
    snprintf( linebuf, readsz, "::%s Bad fdin descriptor\n", __FUNCTION__ );
    return -1;
  }

  char *chp = NULL;
  char *readbufp = readbuf + *readp;
  int readbufsz = readsz - *readp;
  int readc = read( fdin, readbufp, readbufsz );
  int lfpos = 0, err = 0;

  // Did we close? If so, set the linebuf to nothing, return 0.
  if ( readc == 0 )
  {
    memset( linebuf, 0, readsz );
    memset( readbuf, 0, readsz );
    *readp = 0;
    return 0;
  }
  // Did we have an error? If so put the error message in linebuf, return -errno.
  else if ( readc == -1 )
  {
    err = errno;
    snprintf( linebuf, readsz, "::%s error: %s\n", __FUNCTION__, strerror( err ) );
    memset( readbuf, 0, readsz );
    *readp = 0;
    return -err;
  }
  // Do we have a full line now? If so, set linebuf to the line, return linelen.
  else if ( ( chp = strchr( readbuf, '\n' ) ) != NULL )
  {
    // We need the total number of chars in readbuf now.
    *readp += readc;
    // Copy the whole array to the line buffer.
    memcpy( linebuf, readbuf, *readp );
    // Now get the offset to the LF.
    lfpos = chp - readbuf;
    // Copy the portion of the line after the LF to the begining of readbuf
    memcpy( readbuf, linebuf+lfpos+1, *readp - 1 );
    // The new position to append to is just after the remainder copied.
    *readp = *readp - lfpos - 1;
    // Set the line feed to be the EOS.
    linebuf[lfpos] = '\0';
    return lfpos;
  }
  // Do we have a partial line?  If so, keep linebuf empty, return linelen.
  else
  {
    // We need the total number of chars in readbuf now.
    *readp += readc;
    memset( linebuf, 0, readsz );
    return *readp;
  }
}

/// Read from a given fd, get a command, call the correct function. There are
/// some included in functionality which can be built on. What this means is
/// that for the following list, if there is a user-defined function with the
/// same name, it will be called first, then the listed behavior will happen.
///
/// 'quit': sets the quit flag and exits the function.
/// 'help': lists all the commands with a brief description.
/// '?': lists all the commands with a brief description.
/// 'echo': returns the name and unix time as unsigned long int.
int dispatch( char *cmdbuf, int fdout, int fderr, const struct cmd *cmds )
{
  char errmsg[DISPATCH_BUFLEN];
  char buf[DISPATCH_BUFLEN];
  int err = 0;
  struct cmd *cmdp = NULL;
  int argc = 0;
  char *argv[DISPATCH_MAX_SIZE/2];
  memset( argv, 0, sizeof(argv));
  memset( buf, 0, sizeof(buf));

  int cmdlen = strlen( cmdbuf );
  strncpy(buf, cmdbuf, sizeof(buf)-1); // keep a copy for the log
  err = create_arg_list( cmdbuf, cmdlen, DISPATCH_MAX_SIZE/2, &argc, argv );

  // Did we have too many arguments?
  if ( err == -1 )
  {
    snprintf( errmsg, DISPATCH_BUFLEN, "::%s Too many arguments '%s'\n",
        __FUNCTION__, cmdbuf );
    write( fderr, errmsg, strlen( errmsg ) );
    log_server_msg( LOG_ERR, errmsg );
    return DISPATCH_ERR;
  }

  // We only have one syntax error possible at this point.
  if ( err == -2 )
  {
    snprintf( errmsg, DISPATCH_BUFLEN, "::%s Unclosed quote '%s'\n",
        __FUNCTION__, cmdbuf );
    write( fderr, errmsg, strlen( errmsg ) );
    log_server_msg( LOG_ERR, errmsg );
    return DISPATCH_ERR;
  }

  // Do we have at least a command name?
  if ( argc <= 0 )
  {
    // Don't print anything, just go on.
    return DISPATCH_OK;
  }

  // The first token should be the name of the command.
  if ( ( cmdp = cmdarr_name( cmds, argv[0] ) ) == NULL )
  {
    snprintf( errmsg, DISPATCH_BUFLEN, "::%s Unknown command '%s'\n",
        __FUNCTION__, argv[0] );
    write( fderr, errmsg, strlen( errmsg ) );
    log_server_msg( LOG_ERR, errmsg );
    return DISPATCH_ERR;
  }

  // Are all given argument names valid for this command?
  if ( ( err = all_args_valid( argc, argv, cmdp, errmsg ) ) != DISPATCH_OK )
  {
    write( fderr, errmsg, strlen( errmsg ) );
    log_server_msg( LOG_ERR, errmsg );
    return err;
  }

  // Is an argument's value is required to be a number?
  if ( ( err = args_value_number( argc, argv, cmdp, errmsg ) ) != DISPATCH_OK )
  {
    write( fderr, errmsg, strlen( errmsg ) );
    log_server_msg( LOG_ERR, errmsg );
    return err;
  }

  // Make sure that arguments which should be in a range are in a range.
  // This also implies they are numeric.
  if ( ( err = args_value_range( argc, argv, cmdp, errmsg ) ) != DISPATCH_OK )
  {
    write( fderr, errmsg, strlen( errmsg ) );
    log_server_msg( LOG_ERR, errmsg );
    return err;
  }

  // Make sure that arguments which should be in a list are in a list.
  // This also implies they are NOT numeric.
  if ( ( err = args_value_list( argc, argv, cmdp, errmsg ) ) != DISPATCH_OK )
  {
    write( fderr, errmsg, strlen( errmsg ) );
    log_server_msg( LOG_ERR, errmsg );
    return err;
  }

  // Are all required argument names (and values) in this command?
  if ( ( err = all_required_args( argc, argv, cmdp, errmsg ) ) != DISPATCH_OK )
  {
    write( fderr, errmsg, strlen( errmsg ) );
    log_server_msg( LOG_ERR, errmsg );
    return err;
  }

  // At this point, the input command is completely verified.
  else
  {
    // Make the call to the function (finally!)
    log_server_msg( LOG_INFO, "Command sent: %s", buf);
    if ( ( err = ( *( cmdp->func ) )( argc, argv, fdout, fderr ) ) != 0 )
    {
      snprintf( errmsg, DISPATCH_BUFLEN, "'%s' returned %d\n", argv[0], err );
      write( fderr, errmsg, strlen( errmsg ) );
      log_server_msg( LOG_ERR, errmsg );
      return err;
    }

    // If we called ?', return the command array.
    else if ( strncmp( "?", argv[0], 2 ) == 0 )
    {
      cmdarr_write( fdout, "", cmds );
    }

    // If we called 'help', return the command array.
    else if ( strncmp( "help", argv[0], 5 ) == 0 )
    {
      cmdarr_write( fdout, "Command list:", cmds );
    }

    // If we called 'echo', return the name and unix time.
    else if ( strncmp( "echo", argv[0], 5 ) == 0 )
    {
      snprintf( errmsg, DISPATCH_BUFLEN, "%s\n", argv[2] );
      write( fdout, errmsg, strlen( errmsg ) );
    }

    // If we called 'quit', boot the client.
    else if ( strncmp( "quit", argv[0], 5 ) == 0 )
    {
      return DISPATCH_QUIT;
    }
  }
  return DISPATCH_OK;
}
