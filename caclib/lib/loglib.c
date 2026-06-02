/* 
Copyright 2021 Arizona Board of Regents on behalf of the University of Arizona.
Author: Thomas Folkers; tfolkers@arizona.edu; Unless otherwise noted.

For commercial uses, to obtain a license to sell and or sublicense copies of
the Software please contact the University of Arizona at Tech Launch Arizona:
lewish@tla.arizona.edu.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction for non-commercial use educational and
research purposes, including without limitation the rights to use, copy,
modify, merge, publish, distribute, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The entire above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

1. Additional Required Provisions

1.1.  Arbitration. The parties agree that if a dispute arises between them
concerning this Agreement, the parties may be required to submit the matter to
arbitration pursuant to Arizona law.

1.2.  Applicable Law and Venue. This Agreement shall be interpreted pursuant
to the laws of the State of Arizona. Any arbitration or litigation between the
Parties shall be conducted in Pima County, ARIZONA, and LICENSEE hereby
submits to venue and jurisdiction in Pima County, ARIZONA.

1.3.  Non-Discrimination. The Parties agree to be bound by state and federal
laws and regulations governing equal opportunity and non-discrimination and
immigration.

1.4.   Appropriation of Funds. The Parties recognize that performance by
ARIZONA may depend upon appropriation of funds by the State Legislature of
ARIZONA. If the Legislature fails to appropriate the necessary funds, or if
ARIZONA’S appropriation is reduced during the fiscal year, ARIZONA may cancel
this Agreement without further duty or obligation. ARIZONA will notify
LICENSEE as soon as reasonably possible after it knows of the loss of funds.

1.5.   Conflict of Interest. This Agreement is subject to the provisions of
A.R.S. 38-511 and other conflict of interest regulations. Within three years
of the EFFECTIVE DATE, ARIZONA may cancel this Agreement if any person
significantly involved in initiating, negotiating, drafting, securing, or
creating this Agreement for or on behalf of ARIZONA becomes an employee or
consultant in any capacity of LICENSEE with respect to the subject matter of
this Agreement.

 */

/* *** change log ***

date    who Description
------- --- ----------------------------------------------------
19nov02 twf added different time stamp with fractional seconds()
 */

/*
    This is an attempt at a logger library that will handle message logging in 
    a uniform way. The idea is to support log files in a given directory.
    When the newlog command is issued the logfiles are purged - that is
    olderlog is deleted, oldlog is renamed to olderlog and log is renamed to
    oldlog, and a new file log is created.

    Here is a brief description of the routines you can use.

    log_msg( a, b, c... ) - parameters like printf. This will log a message 
       to the log file. Log_msg will prepend the current time.

    log_perror( msg ) - Like perror, this will log a unix message 
       to the log file. Log_perror will prepend the current time.

    log_newlog() - causes the logfile to be closed and renamed. Should be 
       executed when a newlog command is received.

    log_open(name, how) - opens and initializes log file. name is the 
      full unix name.  of the logfile - e.g. /home/corona/cactus/rambo/rambo. 
      This will create files in the /home/corona/cactus/rambo directory called:
      rambo.log, rambo.oldlog and rambo.olderlog. If how is 1 also write 
      message to stderr.

    log_eventdEnable(enable) - Turns on event logging. All messages sent to
      event daemon too. (Hopefully EVENTD doesn't turn this feature on
      too or we will have a loop. This assumes that you did a sock_bind()
      first.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#ifdef USE_THREADS
#include <pthread.h>
#endif

#include "caclib_loglib.h"
#include "caclib_proto.h"

int loglib_raw     = 0;
int log_dup2stdout = 0;
int log_how        = 4;

static char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
			  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


static FILE *logfile = NULL;
static char *logname = NULL;
int          sendToEventd;
int          logCallingSockFlag = 0;
struct SOCK *eventdSock;

char loglib_buffer[4096];

extern char procName[256];


/* Methods:
 *
 * how = 0: log only using ctime timestamp.
 * how = 1: log & write to stderr using ctime timestamp.
 * how = 2: log & write to stderr using no timestamp.
 * how = 3: log & write to stderr using millisec timestamps.
 * how = 4: log only using millisec timestamps.
 * how = 5: log & write to stderr using millisec timestamps + threadname.
 * how = 6: log only using millisec timestamps + threadname.
 *
 */

int log_msg( const char *fmt, ...)
{
  FILE *out;
  time_t tm;
  int how, ret;
  char buf[4096*2], rslt[4096*2], *t;
  char timestr[256];
  va_list args;
#ifdef USE_THREADS
  pthread_t thID;
  char tname[80];
#endif

  if(!strlen(procName))
  {
    strcpy(procName, "NO_PROC_NAME");
  }

  if( logfile ) 
  {
    how = log_how;
    out = logfile;
  } 
  else 
  {
    how = 4;
    out = stderr;
  }
  
#ifdef USE_THREADS
  if( how == 3 || how == 4 || how == 5 | how == 6)
  {
    t = getNewCtime(0);

    thID = pthread_self();
    pthread_getname_np(thID, tname, sizeof(tname));
  }
#else
  if( how == 3 || how == 4)
  {
    t = getNewCtime(0);
  }
#endif
  else
  if( how == 2 )
  {
    t ="";
  }
  else 
  { 
    time(&tm);
    t = ctime(&tm);
    if( !t || strlen(t) == 0 )
    {
      t = " "; 
    }
    else 
    {
      strcpy( timestr, t );
      t = timestr;
      t[strlen(t)-1] = ' ';
    }
  }

  va_start(args, fmt);

  vsprintf(buf, fmt, args );

  va_end(args);

  if(eventdSock)
  {
    logCallingSockFlag = 1;
    ret = sock_send(eventdSock, buf);

    if(ret == 0)  /* was there an error ?? */
    {
      log_eventdEnable(0);	/* Disable further logging */
    }

    logCallingSockFlag = 0;
  }

  if(log_dup2stdout)
  {
    fprintf(stdout, "%s\n", buf ); 
    fflush(stdout);
  }

  if(loglib_raw)
  {
#ifdef USE_THREADS
    if(how > 4)
    {
      sprintf(rslt, "%s (%s): %s", t, tname, buf ); 
    }
    else
#endif
    {
      sprintf(rslt, "%s%s", t, buf ); 
    }
  }
  else
  {
#ifdef USE_THREADS
    if(how > 4)
    {
      sprintf(rslt, "%s (%s): %s\n", t, tname, buf ); 
    }
    else
#endif
    {
      sprintf(rslt, "%s%s\n", t, buf ); 
    }
  }

  strcpy(loglib_buffer, rslt);

  fwrite(rslt, 1, strlen(rslt), out );

  if( (how == 1) || (how == 2) || (how == 3) ) 
  {
    fwrite(rslt, 1, strlen(rslt), stderr );
    fflush(stderr);
  }

  fflush(out);

  return(0);
}


/* Like perror, pnly to the log file too */
int log_perror( msg )
char *msg;
{
  FILE *out;
  time_t tm; // AGY changed from long
  char err[4096], *t;
  char timestr[256];
  int how;

  if( logfile ) 
  {
    how = log_how;
    out = logfile;
  } 
  else 
  {
    how = 0;
    out = stderr;
  }
 
  if( how > 1 )
  {
    t ="";
  }
  else 
  { 
    time(&tm);
    t = ctime(&tm);

    if( !t || strlen(t) == 0 )
    {
      t = " "; 
    }
    else 
    {
      strcpy( timestr, t );
      t = timestr;
      t[strlen(t)-1] = ' ';
    }
  }

  sprintf( err, "%s %s, %s\n", t, msg, strerror(errno) );

  fwrite(err, 1, strlen(err), out );
  fflush(out);

  if( how ) 
  {
    fwrite(err, 1, strlen(err), stderr );
    fflush(stderr);
  }

  return(0);
}


/* Turn on/off echoing to the LOGGER daemon: ACS_LOGGER */
int log_eventdEnable(enable)
int enable;
{
  static int lastEnable = -1;

  if(enable == lastEnable)
  {
    return(0);
  }

  if(enable == LOG_TO_LOGGER)
  {
    if(strlen(procName))
    {
//      printf("%s: Enabling Logging\n", procName);
    }

    eventdSock = sock_connect("ACS_LOGGER");
  }

  if(!enable)
  {
    if(strlen(procName))
    {
//      printf("%s: Dis-Enabling Logging\n", procName);
    }

    eventdSock = (struct SOCK *)NULL;
  }

  lastEnable = enable;

  return(enable);
}

 

/* Open the log file using the passed name and method */
int log_open(name, how)
char *name;
int how;
{
  char buf[4096], *cp;

  if(logname) 
  {
    log_msg("log_open already set");

    return(0);
  }

  cp = getenv("LOGDIR");
  if(cp)
  {
    sprintf(buf, "%s/%s.log", cp, name);
  }
  else
  {
    sprintf(buf, "%s.log", name);
  }

  if((logfile = fopen( buf, "a+" )) == 0 ) 
  {
    log_perror("log_open");

    return(0);
  }

  logname = name;
  log_how = how;

  return(1);
}



/* Rotate logs to a date-named file */
int newlog2datefile()
{
  char log[4096], oldlog[4096], buf[4096], *cp;
  extern char procName[];

  log_msg("%s: --NewLog--", procName);

  cp = getenv("LOGDIR");

  if(cp)
  {
    sprintf(log, "%s/%s.log",   cp, logname);
    sprintf(oldlog, "%s/%s.%s", cp, logname, getDate(0));
  }
  else
  {
    sprintf(log, "%s.log",   logname);
    sprintf(oldlog, "%s.%s", logname, getDate(0));
  }


  if( logfile )
  {
    fclose(logfile);
  }
  else 
  {
    log_msg("newlog2datefile() - no logfile");

    return(1);
  }

  sprintf(buf, "touch %s", log);
  system(buf);

  sprintf(buf, "mv %s %s", log, oldlog);
  system(buf);

  if( (logfile = fopen( log, "a+" )) == NULL )
  {
    log_perror("newlog2datefile()");
    return(1);
  }

  return(0);
}


/* rotate logs */
int log_newlog()
{
  char log[4096], oldlog[4096], olderlog[4096], oldestlog[4096], buf[4096], *cp;
  extern char procName[];

  log_msg("%s: --NewLog--", procName);

  cp = getenv("LOGDIR");

  if(cp)
  {
    sprintf(log,       "%s/%s.log",       cp, logname );
    sprintf(oldlog,    "%s/%s.oldlog",    cp, logname );
    sprintf(olderlog,  "%s/%s.olderlog",  cp, logname );
    sprintf(oldestlog, "%s/%s.oldestlog", cp, logname );
  }
  else
  {
    sprintf(log,       "%s.log",       logname );
    sprintf(oldlog,    "%s.oldlog",    logname );
    sprintf(olderlog,  "%s.olderlog",  logname );
    sprintf(oldestlog, "%s.oldestlog", logname );
  }

  if( logfile)
  {
    fclose(logfile);
  }
  else
  {
    log_msg("newlog - no logfile");

    return(1);
  }

  sprintf(buf, "rm -f %s", oldestlog );           system(buf);
  sprintf(buf, "touch %s", olderlog );            system(buf);
  sprintf(buf, "mv %s %s", olderlog, oldestlog ); system(buf);
  sprintf(buf, "touch %s", oldlog );              system(buf);
  sprintf(buf, "mv %s %s", oldlog, olderlog );    system(buf);
  sprintf(buf, "touch %s", log );                 system(buf);
  sprintf(buf, "mv %s %s", log, oldlog);          system(buf);

  if( (logfile = fopen( log, "a+" )) == NULL )
  {
    log_perror("log_newlog");
  }

  return(0);
}


char *getNewCtime(offset)
int offset;
{
  long usec;
  static char timestr[256];
  struct timeval tvv,*tp;
  static struct tm *tmnow;
  time_t sec, toff;	//AGY changed from long

  tp = &tvv;
  gettimeofday(tp,0);
  tp->tv_usec += 500;	/* Add .0005 sec for rounding to millisec */

  if (tp->tv_usec >= 1000000)
  {
    tp->tv_usec -= 1000000;
    tp->tv_sec++;
  }

  toff = offset * 3600;
  sec = tp->tv_sec + toff;                                      /* add offset */
  tmnow = localtime(&sec);
  usec = ((double)tp->tv_usec / 1000000.0) * 1000.0;            /* Rounded ms */

  sprintf(timestr,"%02d %3.3s %02d %02d:%02d:%02d.%03ld ",
		tmnow->tm_mday, 
		months[tmnow->tm_mon], 
		tmnow->tm_year-100,
		tmnow->tm_hour,
		tmnow->tm_min,
		tmnow->tm_sec,
		usec);

  return(timestr);
}
