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

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>


#include "sdd.h"
#include "header.h"
#include "caclib_mbdef.h"
#include "caclib_proto.h"

static int init_dest();

static int destflag = 0;
static struct DEST mailboxes[MAXDEST];

static struct DEST def_mailboxes[MAXDEST] = {
	{ 1710, 	0,	"localhost", "MONITOR", 	0, { 0, 0, 0 } }, 
	{ 1712, 	0,	"localhost", "RAMBO", 		0, { 0, 0, 0 } },
	{ 1714, 	0,	"localhost", "EXEC", 		0, { 0, 0, 0 } },
	{ 1716, 	0,	"localhost", "RTDATA", 		0, { 0, 0, 0 } },
	{ 1717, 	0,	"localhost", "STATUS", 		0, { 0, 0, 0 } },
	{ 1726, 	0,	"localhost", "COMPOSE", 	0, { 0, 0, 0 } },
	{ 1727, 	0,	"localhost", "EXECTEST", 	0, { 0, 0, 0 } },
	{ 1733, 	0,	"localhost", "SOUND", 		0, { 0, 0, 0 } },
	{ 1743, 	0,	"localhost", "OBSERVER", 	0, { 0, 0, 0 } },
	{ 1746, 	0,	"localhost", "GLOB", 		0, { 0, 0, 0 } },
	{ 1747, 	0,	"localhost", "OBSTOOL",		0, { 0, 0, 0 } },
	{ 1759, 	0,	"localhost", "DATASERV", 	0, { 0, 0, 0 } },

	{ 0,    	0,	"",          "",	 	0, { 0, 0, 0 } }
};


/* Return a pointer to the struct DEST for mailbox `name' (or NULL if none) */

struct DEST *find_dest(name)
char *name;
{
  int i;

  if(destflag == 0)
  {
    init_dest();
  }

  for(i=0;i<MAXDEST;i++)
  {
    if(mailboxes[i].dname[0] == '\0')
    {
      break;
    }
    else
    if(strcmp( mailboxes[i].dname, name) == 0)
    {
      return( &mailboxes[i] );
    }
  }

  for(i=0;i<MAXDEST;i++)
  {
    if(def_mailboxes[i].dname[0] == '\0')
    {
      break;
    }
    else
    if(strcmp(def_mailboxes[i].dname, name) == 0)
    {
      return(&def_mailboxes[i]);
    }
  }

  return(NULL);
}

struct DEST *get_dest(i)
int i;
{
  if( destflag == 0 )
  {
    init_dest();
  }

  if(i < MAXDEST)
  {
    if(mailboxes[i].dname[0] != '\0')
    {
      return( &mailboxes[i] );
    }
  }

  return(NULL);
}


/*
 * Return a pointer to the first struct DEST for a mailbox with a port number 
 * `p' (or NULL if none).  If p=0, continue the search for the previous port 
 * number beginning after the last one found (or NULL if no more found)
 */

struct DEST *find_port(p)
int p;
{
  static int port, i_standard, i_default;

  if (p) 
  {
    i_standard = 0;					/* Start from the beginning */
    i_default  = 0;
    port       = p;					/* And remember the port number */
  }

  if(destflag == 0)
  {
    init_dest();
  }

  for(;i_standard<MAXDEST;i_standard++)
  {
    if(mailboxes[i_standard].dname[0] == '\0')
    {
      break;
    }
    else
    if(mailboxes[i_standard].dport == port)
    {
      return(&mailboxes[i_standard++]);
    }
  }

  for(;i_default<MAXDEST;i_default++)
  {
    if(def_mailboxes[i_default].dname[0] == '\0')
    {
      break;
    }
    else
    if(def_mailboxes[i_default].dport == port)
    {
      return(&def_mailboxes[i_default++]);
    }
  }

  return(NULL);
}


/* Read the mailboxdefs file and store in mailboxes[] */
static int init_dest()
{
  static char delim[] = ", \t\n";
  FILE *fd;
  int i, count;
  char buf[256], *p;

  p = getenv("MAILBOXDEFS");

  if(!p)
  {
    p = "/home/user/cactus/mailboxdefs";
  }

  bzero( &mailboxes[0], sizeof(struct DEST)*MAXDEST);

  if((fd = fopen( p, "r" ) )== NULL) 
  {
    destflag = 1;

    return(0);
  }

  count = 0;
  while(fgets(buf, 256, fd ) != NULL) 
  {
    if(buf[0] == '#' || buf[0] == '\n')
    {
      continue;
    }

    if((i = atoi(strtok( buf, delim ))) == 0)
    {
      continue;
    }

    mailboxes[count].dport = abs(i);			/* Ports are always positive */
    mailboxes[count].raw   = (i < 0);			/* Negative port means a `raw' socket */

    strncpy(mailboxes[count].host,  strtok(NULL, delim), MAXNAME );
    strncpy(mailboxes[count].dname, strtok(NULL, delim), MAXNAME );

    if( ++count >= MAXDEST ) 
    {
      printf("too many entries in mailboxdefs\n");
      break;

    }
  }

  fclose(fd);

  destflag = 1;

  return(0);
}



/* bind before connect to avoid known OS hang bug */
int bind_any(fd)
int fd;
{

  return(0);
}
