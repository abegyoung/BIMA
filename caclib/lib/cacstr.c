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


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define	NO_EXTERNS

#include "sdd.h"
#include "header.h"
#include "caclib_sex.h"
#include "caclib_proto.h"

#define SZ_LINE         161     /* Length of typical text line. */
#define YES             1       /* Boolean true. */
#define NO              0       /* Boolean false. */


struct SOCK *super = 0;
int  interactiveSession = 0;
char procName[256];

char *cactus_home        = "/home/user/cactus";
char *cactus_global      = "/home/user/cactus/CACTUS_GLOBAL";
char *cactus_mailboxdefs = "/home/user/cactus/mailboxdefs";
char *cactus_wind_memory = "/home/user/cactus/monitor/WIND_MEMORY";
char *cactus_isc_memory  = "/home/user/cactus/monitor/ISC_MEMORY";


/* CACSTR.C -- Assorted CACTUS string handling routines. */

/*              
 * Convert hexadecimal string <s> to an integer
 * (If <s> is not a hexadecimal number, 0 is returned)
 */             

/* setCactusEnvironment()
 *
 * Set the standard cactus string variables for use
 * by programs who care to use them. 
 * i.e. all of them eventually. ;-)
 *
 * Must be called before any thing else.
 *
 */
int setCactusEnvironment()
{
  char *cp;

  if((cp = getenv("CACTUSHOME")))
  {
    cactus_home = cp;
  }

  if((cp = getenv("CACTUS_GLOBAL")))
  {
    cactus_global = cp;
  }

  if((cp = getenv("MAILBOXDEFS")))
  {
    cactus_mailboxdefs = cp;
  }

  return(0);
}

int atox(s)             
char *s;
{
  int d = 0;            
  sscanf(s, "%x", &d );
  return(d);    
}


/* Return 0 if interactive terminal session */
int getInteractive()
{
  int log_how;
  pid_t tgrp, pgrp;

  char *cp;

  cp = getenv("TERM");

  if(cp)
  {
    pgrp = getpgrp();
    tgrp = tcgetpgrp(STDOUT_FILENO);

    if(pgrp == tgrp)
    {
      log_how = 0;
      interactiveSession = 1;
    }
    else
    {
      log_how = 1;
      interactiveSession = 0;
    }
  }
  else
  {
    log_how = 1;
  }

  return(log_how);
}



int smart_log_open(name, offset)
char *name;
int offset;
{
  int log_how;

  log_how = getInteractive() + offset;

  log_open(name, log_how);

  strcpy(procName, name);
  strupr(procName);

  log_msg("%s: Smart Logging Set to: %d", procName, log_how);
  log_msg("%s: interactiveSession:   %d", procName, interactiveSession);

  return(log_how);
}


/* Determines if the instance of the process in owned by 'user' and
   is being displayed on 'display'. If so return True. 

   Used to determine if this program should be the one to send
   PROCESS_TIMES to the super-visor process. A lot of programs can
   be run multiply times and only one of them should report in. 
 */
int iamwho(user, display)
char *user, *display;
{
  char *cp1, *cp2;

  log_msg("iamwho(%s, %s)", user, display);

  cp1 = getenv("USER");
  cp2 = getenv("DISPLAY");

  log_msg("iamwho(): USER = %s", cp1);
  log_msg("iamwho(): DISPLAY = %s", cp2);

  if(cp1)
  {
    if(!strncmp(cp1, user, strlen(user)))
    {
      if(cp2)
      {
        if(strstr(cp2, display))
        {
          log_msg("iamwho(): Returns True");
          return(1);
        }
      }
    }
  }

  log_msg("iamwho(): Returns False");

  return(0);
}


int sendProcessTime(indx, t, pid)
int indx, t, pid;
{
  char tbuf[256];

  if(!indx)
  { 
    return(0);
  } 

  sprintf(tbuf, "PROCESS_TIME %d %d %d", indx, t, pid);
    
  if(!super)
  {
    super = sock_connect("SUPER");
  } 
    
  return(sock_send(super, tbuf));
}


/* Convience functions for timed reporting */
int sendTimeStamp(id)
int id; 
{ 
  static time_t oldTime=0;
  time_t t;
            
  t = time(NULL);
    
  if(id && abs(t-oldTime) > 8)
  {
    sendProcessTime(id, t, getpid());
    oldTime = t;
  }

  return(0);
}



int checkSendProc(id, pid)
int id, pid;
{
  static time_t old_time = 0;
  time_t t;

  t = time(NULL);

  if(abs(t - old_time) > 10)
  {
    sendProcessTime(id, t, pid);

    old_time = t;

    return(1);
  }

  return(0);
}

int send_process_info_internal(indx, iNum, iBuf)
int indx, iNum;
char *iBuf;
{
  char tbuf[4096];

  if(!indx)
  {
    return(0);
  }

  snprintf(tbuf, sizeof(tbuf), "PROCESS_INFO %3d %5d %s", indx, iNum, iBuf);

  if(!super)
  {
    super = sock_connect("SUPER");
  }

  return(sock_write(super, tbuf, strlen(tbuf)));
}

#define PROCESS_INFO_STATUS 0

int sendProcessInfo(indx, pid, type)
int indx, pid, type;
{
  FILE *fp;
  char bigAssBuf[4096], tbuf[4096];
  char filename[256];

  if(type == PROCESS_INFO_STATUS)
  {
    sprintf(filename, "cat /proc/%d/status", pid);

    if((fp = popen(filename, "r") ) <= (FILE *)0 )
    {
      log_msg("Unable to %s", filename);
      return(1);
    }

    sprintf(bigAssBuf, "/proc/%d/status\n", pid);

    while(fgets(tbuf, sizeof(tbuf), fp) != NULL)
    {
      strcat(bigAssBuf, tbuf);
    }

    pclose(fp);
  }

/* Big Ass Buffer size arounf 750 bytes */

  if(type == PROCESS_INFO_STATUS)
  {
    send_process_info_internal(indx, type, bigAssBuf);
  }

  return(0);
}



/* STRCMPRS -- Compress all white spaces in a string to single spaces.
 * Leading and trailing white space is removed from the string.  The
 * function returns the resulting compressed string.
 *
 * This is the C emulation of the DCL f$edit(,"compress") lexical function.
 *
 *		char *
 *		strcmprs(str)
 *
 *		char	*str	input string
 */

char *
strcmprs(str)
char    *str;           /* input string */
{
    register char   *ip;
    register char   *op;
    int     whiteout;

    for(ip=str; isspace(*ip); ip++)    /* Remove leading white */
        ;
    whiteout=NO;

    for(op=str;(*op = *ip); ip++, op++)
        if(isspace(*ip))
        if(whiteout)
            op--;
        else {
            *op = ' ';
            whiteout = YES;
        }
        else
        whiteout = NO;

    if(whiteout)               /* Remove trailing white */
        op--;

    *op = 0;

    return(str);
}


/* deNewLineIt() Replace all newlines and returns with spaces */
int deNewLineIt(buf)
char *buf;
{
  char *cp;

  for(cp=buf;*cp;cp++)
  {
    if( *cp == '\n' || *cp == '\r')
    {
      *cp = ' ';
    }
  }

  return(0);
}


/* CTOD -- Convert ASCII character string to double, including sexagesimal.
 *
 *		double
 *		ctod(str)
 *
 *		char    *str	input string
 *
 * Sexagesimal numbers are identified as strings containing colons, and 0, 1
 * or 2 "significant" colons are permissible.  A "significant" colon is one
 * that has a placeholder string in front of or behind it.  A sexigesimal
 * number has from 1 to 3 fields, and they are presumed to accumulate from
 * the least to most significant.
 */

double
ctod(str)
char    *str;           /* input string */
{
	char	strloc[SZ_LINE+1];
	
	strcpy(strloc, str);
	if(strchr(strloc, ':')) {	/* sexagesimal - parse here */
		char	*tok;
		double	fld[3], sexnum;
		int	i = -1;
		int	neg=NO;

		tok = strtok(strloc, ":");
		while(tok && ++i<3) {
			fld[i] = atof(tok);
			if(fld[i] <(double) 0)
				neg = YES;
			fld[i] = fabs(fld[i]);
			tok = strtok(NULL, ":");
		}

		switch(i) {
		case 0:
			sexnum = fld[0] /(double) 3600;
			break;
		case 1:
			sexnum = fld[0] /(double) 60 + fld[1] /(double) 3600;
			break;
		case 2:
			sexnum = fld[0] + fld[1] /(double) 60 + fld[2] /(double) 3600;
			break;
		default:
			fprintf(stderr, "ctod: invalid sexagesimal number %s\n", str);
			return((double) 0);
		}

		if(neg)
			sexnum = -sexnum;

		return((double) sexnum);

	} else				/* not sexagesimal - use atof() */

		return((double) atof(strloc));
}


#define DTOMA   3600000

/* CTOMA -- Convert ASCII character string to long integer milliarcsec.
 *
 *  long
 *  ctoma(str)
 *
 *  char    *str	input string
 */

long ctoma(str)
char *str;
{
    double  val;

    val = ctod(str) *(double) DTOMA;
    return((long) val);
}


/* STRLWR -- Convert string to lower case.
 *
 *		char *
 *		strlwr(str)
 *
 *		char *str	input string
 */

char *
strlwr(str)
char *str;
{
  char *ip;
	
  ip = str;
  while(*ip)
  {
    if(isupper(*ip))
    {
      *ip = tolower(*ip);
    }

    ip++;
  }

  return((char *) str);
}


/* STRUPR -- Convert string to upper case.
 *
 *		char *
 *		strupr(str)
 *
 *		char *str	input string
 */

char *
strupr(str)
char *str;
{
  char *ip;

  ip = str;	
  while(*ip) 
  {
    if(islower(*ip))
    {
      *ip = toupper(*ip);
    }

    ip++;
  }

  return((char *) str);
}



/* GET_TIME --  Returns a char string like '12:34:56'. 
 */
char *get_time()
{
  float timed;
  static char timestr[128];

  struct timeval tvv,*tp;   			  /* holds time in SUN format */
  struct tm *tmnow;
  long sec, usec;
  int hour, minutes, seconds;

  tp = &tvv;
  gettimeofday(tp,0);
  sec = tp->tv_sec + 25200; 			  /* add offset for u.t. time */
  usec = tp->tv_usec;
  tmnow = localtime(&sec);

  hour = tmnow->tm_hour;
  minutes = tmnow->tm_min;
  seconds = tmnow->tm_sec;

  timed = hour 
	+ minutes / 60.0 
	+ seconds / 3600.0 
	+ usec / 3600000000.0;
    
  sexagesimal((double)timed,timestr,DD_MM_SS);
  return(timestr);
}

#ifdef NOT_SURE_THIS_IS_NEEDED
/* Return non-zero if "string" only contains characters that could be a number:
 * digits, up to one decimal point, optional leading + or - sign.  
 * A single E is allowed if it is followed by an integer.  Returns 1 if 
 * string is an integer, 2 if string has a decimal point, 3 if string is an
 * E-format number.  Otherwise 0 is returned.
 * N.B. string should not contain leading or trailing blanks.
 */

int cactus_isnumber(string)
char *string;
{
  char *digits = "0123456789";
  int n = 0;

  if (string[0] == '+' || string[0] == '-')
  {
    string++;					/* Skip over the sign */
  }

  n = strspn(string, digits);
  if (string[n] == '\0')
  {
    return 1;					/* Only digits were found */
  }

  if (string[n] == '.')				/* Found a decimal point */
  {
    n += strspn(string+(++n), digits);		/* More digits can follow */
  }

  if (string[n] == '\0')
  {
    return 2;					/* A decimal number was found */
  }

  if (string[n] != 'e' && string[n] != 'E')
  {
    return 0;					/* Not a number */
  }
  
  n++;						/* letter E was found */
  if (string[n] == '+' || string[n] == '-')
  {
    n++;					/* Exponent has a sign */
  }

  if (string[n] == '\0')
  {
    return 0;					/* Bad: No exponent value */
  }

  n += strspn(string+n, digits);
  if (string[n] == '\0')
  {
    return 3;					/* E-format number was found */
  }

  return 0;					/* Not a number */
}
#endif
