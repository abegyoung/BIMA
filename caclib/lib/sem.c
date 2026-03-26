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
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/time.h>

#include "caclib_sem.h"
#include "caclib_proto.h"

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
	int val;                    /* value for SETVAL */
        struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
        unsigned short int *array;  /* array for GETALL, SETALL */
        struct seminfo *__buf;      /* buffer for IPC_INFO */
};
#endif

extern char procName[256];

int   semKeys[]  = {  SEM_SERIALOUT_KEY, SEM_SERVO_KEY, SEM_RECORD_KEY, SEM_WAVETEST_KEY };
char *semNames[] = { "SEM_SERIALOUT",   "SEM_SERVO",   "SEM_RECORD",   "SEM_WAVETEST" };

int sems[MAX_SEMS_KEYS];


/* Basic semaphore library:

   semInit()   -- Initialize the defined semaphores

   semTake()   -- Wait on the passed sem

   semClear(); -- Clear out any pending semaphores for the passed one.

   semGive()   -- Give the passed semaphore

 */

int semInit()
{
  int i;

  for(i=0;i<MAX_SEMS_KEYS;i++)
  {
    if( (sems[i] = semget( semKeys[i], 2, IPC_CREAT | IPC_EXCL | 0x1ff ))<0) 
    {
      if( (sems[i] = semget( semKeys[i], 2, 0x1ff ))<0)
      {
        log_msg("%s: semget2 %s", procName, semNames[i]);
      }
    }
  }

  return(0);
}


int semTake(sem, wait)
int sem, wait;
{
  struct sembuf thesem;
  int i, n, val=0;
  union semun arg;

  if(wait == NO_WAIT)
  {
    while(1)                                       /* loop until sem is clear */
    {
      val = semctl(sem, 0, GETVAL, &arg);
      if(val)                           /* there is one to be had, go take it */
      {
        thesem.sem_num = 0;
        thesem.sem_op = -1;
        thesem.sem_flg = 0;

        if( semop( sem, &thesem, 1) < 0)
        {
          perror("semop");

          if( errno == EINTR )
          {
            log_msg("%s: take sem error", procName);
          }

          return(1);
        }
      }
      else
      {
        return(0);                                   /* it's clear, so return */
      }
    }
  }
  else
  if(wait == WAIT_FOREVER)
  {
    n = 86400 * 1000; /* wait a whole day */
  }
  else
  {
    n = wait * 10;
  }

  for(i=0;i<n;i++)                           /* loop until sem given, timeout */
  {
    val = semctl(sem, 0, GETVAL, &arg);

    if(val)                             /* there is one to be had, go take it */
    {
      thesem.sem_num = 0;
      thesem.sem_op = -1;
      thesem.sem_flg = 0;

      if(semop( sem, &thesem, 1 ) < 0)
      {
        perror("semop");

        if(errno == EINTR)
        {
          log_msg("%s: take sem error", procName);
        }

        return(1);
      }

      return(0);
    }

    usleep(1000);
  }

  return(TIMEOUT);
}



int semClear(sem, verbose)
int sem, verbose;
{
  struct sembuf thesem;
  int val=0, cnt=0;
  union semun arg;

  while(1)                                       /* loop until sem is clear */
  {
    val = semctl(sem, 0, GETVAL, &arg);

    if(val)                           /* there is one to be had, go take it */
    {
      thesem.sem_num = 0;
      thesem.sem_op = -1;
      thesem.sem_flg = 0;

      if( semop( sem, &thesem, 1) < 0)
      {
        perror("semop");

        if( errno == EINTR )
        {
          log_msg("%s: take sem error", procName);
        }
      }
    }
    else
    {
      if(verbose)
      {
        log_msg("%s: Removed %d sem's", procName, cnt);
      }

      return(0);                                   /* it's clear, so return */
    }

    cnt++;
  }

  return(0);
}


int semGive(sem)
int sem;
{
  struct sembuf thesem;

  thesem.sem_num = 0;
  thesem.sem_op =  1;
  thesem.sem_flg = 0;

  if(semop(sem, &thesem, 1) < 0)
  {
    perror("semop");

    if( errno == EINTR )
    {
      log_msg("%s: give sem error", procName);
    }

    return(1);
  }

  return(0);
}
