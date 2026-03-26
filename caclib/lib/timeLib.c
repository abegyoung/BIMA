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
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include "caclib_sex.h"
#include "caclib_proto.h"

static char *months[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                          "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};


#define SUNFUDGE      0.000022222 			  /* i.e. 00:00:00.08 */
#define JAN1989		599601743

/* SEXAGESIMAL -- converts double to xx:xx[:xx.xx]
 */
int sexagesimal(dang, str, prec)
double dang;
char *str;
char prec;
{
  int bigi,degi,mini,
      seci,sec100,neg=0,i;
 
  if(dang < 0.0) neg = 1;
  dang = fabs(dang);
  if(dang > 360.0) dang = 360.0;
 
  switch(prec)
  { 
    case MMMM_SS:
    case IGNORE_EXP:
    case MMM_SS_OVER:
    case DDDD_MM_SS:
//        dang += 0.0001388;                                /* add 1/2 a second */
        break;
    case DDDD_MM:
    case HH_MM:
//        dang += 0.0083334;                                /* add 1/2 a minute */
        break;
    case SSS_S100:
    case MMM_SS_S100:
    case DDDD_MM_SS_S100:
    case DDD_MM_SS_S100:
    case DD_MM_SS:
    case DDDD_MM_SS_SS100:
//        dang += 0.00001388;                 /* add 1/2 of 1 tenth of a second */
        break;
  }
 
  bigi = (int)(dang * 360000);                 /* convert to integer math now */
  degi = bigi / 360000;
  bigi %= 360000;
  mini = bigi / 6000;
  bigi %= 6000;
  seci = bigi / 100;
  sec100 = bigi % 100;
    
  switch(prec)
  {
    case SSS_S100 :                                         /* seconds.sec100 */
      sprintf(str,"%3d.%1.0f",seci,(double)sec100 / 10);
      break;
     
    case MMMM_SS :                                                 /* MMMM:SS */
     if(degi>0)
       mini += degi * 60;
     sprintf(str,"%4d:%02d",mini,seci);
     break;
 
    case MMM_SS_OVER :               /* MM:SS or 60:00 if greater thern 60:00 */
     sprintf(str,"%3d:%02d",mini,seci);
     if(degi>0)
       strcpy(str," 60:00");
     break;
 
    case IGNORE_EXP :              /* MM:SS or 'IGNORE' if greater then 60:00 */
     sprintf(str,"%3d:%02d",mini,seci);
       if(degi>0) strcpy(str,"IGNORE");
     break;
 
    case DDDD_MM :                                                 /* +DDD:MM */
      sprintf(str,"%4d:%02d",degi,mini);
      break;
     
    case HH_MM :                                                     /* DD:MM */
      sprintf(str,"%2d:%02d",degi,mini);
      break;
     
    case MMM_SS_S100 :                                            /* MMM:SS.S */
      sprintf(str,"%3d:%02d.%1.1d",mini,seci,sec100);
      break;
 
    case DDDD_MM_SS :                                           /* +DDD:MM:SS */
      sprintf(str,"%4d:%02d:%02d",degi,mini,seci);
      break;
 
    case DD_MM_SS :                                   /* HH:MM:SS or DD:MM:SS */
      sprintf(str,"%2d:%02d:%02d",degi,mini,seci);
      break;
 
    case DDDD_MM_SS_S100 :                                    /* +DDD:MM:SS.s */
      sprintf(str,"%4d:%02d:%02d.%d",degi,mini,seci,sec100 / 10);
      break;
 
    case DDD_MM_SS_S100 :                                      /* +DD:MM:SS.s */
      sprintf(str,"%3d:%02d:%02d.%d",degi,mini,seci,sec100 / 10);
      break;
 
    case DDDD_MM_SS_SS100 :                                  /* +DDD:MM:SS.SS */
      sprintf(str,"%4d:%02d:%02d.%02d",degi,mini,seci,sec100);
      break;
  }
  if(neg)                                           /* add negitive sign here */
  {
    for(i=0;i<4;i++)
    {
      if(isdigit(str[i]))
      {
        str[--i] = '-';
        break;
      }
    }
  }

  return(0);
}   


/* DELAY() -- Custom delay routine that uses select to time out. delays in
 *		units of 22 - 44 usec can be had. returns 0 for a good delay.
 *		any key board hit will stop delay and return true.
 */
int delay(dc)
double dc;
{
  
  struct timeval del;
  fd_set a;
  
  del.tv_sec = (int)dc;
  del.tv_usec = (dc - (int)dc) * 1e6;
  FD_ZERO(&a);
  FD_SET(0, &a);
  if(select(1, &a, NULL, NULL, &del))
  {
    /* ioctl(0,TCFLSH,0); 			 clear out keyborad buffer */
    return(1);
  }
  return(0); 
}



/* getLongYear() returns the year.  */
int getLongYear(offset)
int offset;
{
  struct timeval tvv,*tp;                         /* holds time in SUN format */
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff;                        /* add offset for u.t. time */

  tn = localtime(&sec);

  return(tn->tm_year+1900);
}


int getWDay(offset)
int offset;
{
  struct timeval tvv,*tp;                         /* holds time in SUN format */
  struct tm *tn;
  long sec, toff;
  
  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff;                                      /* add offset */
  
  tn = localtime(&sec);

  return(tn->tm_wday);
}


/* getDoy() returns the day of year.  */
int getDoy(offset)
int offset;
{
  struct timeval tvv,*tp;    			  /* holds time in SUN format */
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff; 			  /* add offset for u.t. time */

  tn = localtime(&sec);

  return(tn->tm_yday + 1);
}


/* getYear() returns the year.  */
int getYear(offset)
int offset;
{
  struct timeval tvv,*tp;    			  /* holds time in SUN format */
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff; 			  /* add offset for u.t. time */

  tn = localtime(&sec);

  return(tn->tm_year+1900);
}



/* getMonth() returns the Month. ((1-12) */
int getMonth(offset)
int offset;
{
  struct timeval tvv,*tp;    			  /* holds time in SUN format */
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff; 			  /* add offset for u.t. time */

  tn = localtime(&sec);

  return(tn->tm_mon+1);
}



/* getDom() returns the day of Month.  */
int getDom(offset)
int offset;
{
  struct timeval tvv,*tp;    			  /* holds time in SUN format */
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff; 			  /* add offset for u.t. time */

  tn = localtime(&sec);

  return(tn->tm_mday);
}



/* getDate() -- returns date like '032293'.  */
char *getDate(offset)
int offset;
{
  static char str[256];
  struct timeval tvv,*tp;
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;					     /* secs per hour */
  sec = tp->tv_sec + toff;					/* add offset */

  tn = localtime(&sec);

  sprintf(str,"%02d%02d%02d", tn->tm_mon+1, tn->tm_mday, tn->tm_year-100);

  return(str);
}




/* getLogDate() -- returns date like '03.22.234500'.  i=`date +%m.%d."08/25/97M%S"` */
char *getLogDate(offset)
int offset;
{
  static char str[256];
  struct timeval tvv,*tp;
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;                                      /* secs per hour */
  sec = tp->tv_sec + toff;                                      /* add offset */

  tn = localtime(&sec);

  sprintf(str,"%02d.%02d.%02d%02d%02d", tn->tm_mon+1, tn->tm_mday,
				        tn->tm_hour, tn->tm_min,
					tn->tm_sec);
  return(str);
}


/* getIncFileName() -- returns date like '23JAN03.0923'.  */
char *getIncFileName(offset)
int offset;
{
  static char str[256];
  struct timeval tvv,*tp;
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;                                      /* secs per hour */
  sec = tp->tv_sec + toff;                                      /* add offset */

  tn = localtime(&sec);

  sprintf(str,"%02d%3.3s%02d.%02d%02d", tn->tm_mday, months[tn->tm_mon],
                                        tn->tm_year-100,
                                        tn->tm_hour, tn->tm_min);
  return(str);
}



/* getSlashDate() -- returns date like '03/22/93'.  */
char *getSlashDate(offset)
int offset;
{
  static char str[256];
  struct timeval tvv,*tp;
  struct tm *tn;
  long sec, toff;
 
  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;                                      /* secs per hour */
  sec = tp->tv_sec + toff;                                      /* add offset */

  tn = localtime(&sec);

  sprintf(str,"%02d/%02d/%02d", tn->tm_mon+1, tn->tm_mday, tn->tm_year-100);

  return(str);
}


/* getIncDate() -- returns date like '23-JAN-03'.  */
char *getIncDate(offset)
int offset;
{
  static char str[256];
  struct timeval tvv,*tp;
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;                                      /* secs per hour */
  sec = tp->tv_sec + toff;                                      /* add offset */

  tn  = localtime(&sec);

  sprintf(str,"%02d-%3.3s-%02d", tn->tm_mday, months[tn->tm_mon], tn->tm_year-100);

  return(str);
}



/* getSDate() -- returns date like '0322'.  */
char *getSDate(offset)
int offset;
{
  static char str[256];
  struct timeval tvv,*tp;
  struct tm *tn;
  long sec, toff; 

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;                                      /* secs per hour */
  sec = tp->tv_sec + toff;                                      /* add offset */

  tn = localtime(&sec);

  sprintf(str,"%02d%02d", tn->tm_mon+1, tn->tm_mday);

  return(str);
}



/* getRDate() -- returns date like '930322'.  */
char *getRDate(offset)
int offset;
{
  static char str[256];
  struct timeval tvv,*tp;
  struct tm *tn;
  long sec, toff;
 
  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;                                      /* secs per hour */
  sec = tp->tv_sec + toff;                                      /* add offset */

  tn = localtime(&sec);

  sprintf(str,"%02d%02d%02d",   tn->tm_year-100, tn->tm_mon+1, tn->tm_mday);

  return(str);
}


/* getNraoDate() -- returns date like '1570.000'.  */
double getNraoDate()
{
  struct timeval tvv,*tp;
  long   sec;
  double diff;

  tp = &tvv;

  gettimeofday(tp,0);

  sec   = tp->tv_sec + 25200;
  diff  = sec - JAN1989;
  diff /= 86400.0;

  return(diff);
}



/* getTimeD() -- Returns a double time.  */
double getTimeD(offset)
int offset;
{
  struct timeval tvv,*tp;    			  /* holds time in SUN format */
  struct tm *tn;
  double timenow;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff;					/* add offset */

  tn = localtime(&sec);

  timenow = (double)tn->tm_hour 
	  + (double)tn->tm_min  / 60.0 
	  + (double)tn->tm_sec  / 3600.0
          + (double)tp->tv_usec / (3600.0 * 1e6);

  return(timenow);
}


/* Returns the current minute, or current +/- offset */
int getMinute(offset)
int offset;
{
  struct timeval tvv,*tp;    			  /* holds time in SUN format */
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff;					/* add offset */

  tn = localtime(&sec);

  return(tn->tm_min);
}



/* Returns the current hour, or current +/- offset in seconds */
int getHour(offset)
int offset;
{
  struct timeval tvv,*tp;    			  /* holds time in SUN format */
  struct tm *tn;
  long sec, toff;

  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff;					/* add offset */

  tn = localtime(&sec);

  return(tn->tm_hour);
}


/* getTimeS --  Returns a char string like '12:34:56'.  */
char *getTimeS(offset)
int offset;
{
  static char timestr[128];
    
    
  struct timeval tvv,*tp;
  struct tm *tn;
  long sec, toff;
 
  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff;					/* add offset */

  tn = localtime(&sec);

  sprintf(timestr,"%02d:%02d:%02d",tn->tm_hour,tn->tm_min,tn->tm_sec);

  return(timestr);
}



/* getcTime --  Returns a char string like 'Fri Feb 26 13:30:12 1993'.  */
char *getcTime(offset)
int offset;
{    
  struct timeval tvv,*tp;
  long sec, toff;
  static char buf[80];
 
  tp = &tvv;
  gettimeofday(tp,0);
  toff = offset * 3600;
  sec = tp->tv_sec + toff;					/* add offset */
  strcpy(buf, ctime(&sec));
  buf[strlen(buf)-1] = ' ';
  return(buf);
}



#define GREG 588829

/* Returns a double of Modified Julian Time +/- offset(secs) */
double getmJulDate(offset)
int offset;
{
  struct timeval tvv,*tp;    			  /* holds time in SUN format */
  struct tm *tn;
  long sec, toff;
  int jm, jy, ja, jd;
  int im, iy, id;
  double rjult, juld;

  tp = &tvv;

  gettimeofday(tp,0);

  toff = offset * 3600;
  sec = tp->tv_sec + toff; 			  /* add offset for u.t. time */

  tn = localtime(&sec);

  im = tn->tm_mon+1;
  id = tn->tm_mday;
  iy = tn->tm_year+1900;

  if(im > 2)
  {
    jy = iy;
    jm = im + 1;
  }
  else
  {
    jy = iy - 1;
    jm = im + 13;
  }

  jd = (int)(365.25 * (double)jy) + (int)(30.6001 * (double)jm) + id + 1720995;

  if(( id + 31 * (im + 12 * iy)) > GREG)
  {
    ja = (int)(0.01 * (double)jy);
    jd = jd + 2 - ja + (int)(0.25 * ja);
  }

  rjult = getTimeD(offset) + 12.0;

  if(rjult > 24.0)
  {
    jd++;
    rjult -= 24.0;
  }

  juld = (double)jd + (rjult/24.0) - 2400000.5;

  return(juld-1.0);
}


/* break a MJD into its componets parts */
int breakmJulian(jd, yr, mo, day, hr, min, sec)
double jd;
int *yr, *mo, *day, *hr, *min, *sec;
{
  int   id, im, iy, ihour, imin, isec;
  double j, y, d, m, t;
  int bigi;
  double jj;

  jj = jd + 2400000.5;

  t = (jj - (double)((int)(jj))) * 24.0 + 12.0;

  if(t > 24.0)
  {
    t -= 24.0;
  }

  j  = (double)((int)(jj + 0.50 - 1721119.0));
  y  = (double)((int)((4.0*j - 1.00) / 146097.0));
  j  = 4.0*j - 1.00 - 146097.0*y;
  d  = (double)((int)(j * 0.250));
  j  = (double)((int)((4.00*d + 3.00) / 1461.00));
  d  = 4.00*d + 3.00 - 1461.00*j;
  d  = (double)((int)((d+4.00) * 0.250));
  m  = (double)((int)((5.00*d - 3.00) / 153.00));
  id = 5.00*d - 3.00 - 153.00*m;
  id = (id + 5) / 5;
  iy = j;
  im = m;

  if(im <= 10)
  {
    im += 3;
  }
  else
  {
    im -= 9;
    iy += 1;
  }

  if(im>12)
  {
    im -= 12;
    iy += 1;
  }

  t += 0.00001388;
  bigi = (int)(t * 360000);
  ihour = bigi / 360000;
  bigi %= 360000;
  imin = bigi / 6000;
  bigi %= 6000;
  isec = bigi / 100;

  *yr  = iy;
  *mo  = im;
  *day = id;
  *hr  = ihour;
  *min = imin;
  *sec = isec;

  return(1);
}


/* Given a month, day, year and time in 24.0 hour format, make a MJD */
double makeJulDate(im, id, iy, ut)
int im, id, iy;
float ut;
{
  int jm, jy, ja, jd;
  double rjult, juld;

  if(iy < 93)
  {
    iy += 2000;
  }
  else
  {
    iy += 1900;
  }

  if(im > 2)
  {
    jy = iy;
    jm = im + 1;
  }
  else
  {
    jy = iy - 1;
    jm = im + 13;
  }

  jd = (int)(365.25 * (double)jy) + (int)(30.6001 * (double)jm) + id + 1720995;

  if((id + 31 * (im + 12 * iy)) > GREG)
  {
    ja = (int)(0.01 * (double)jy);
    jd = jd + 2 - ja + (int)(0.25 * ja);
  }

  rjult = ut + 12.0;

  if(rjult > 24.0)
  {
    jd++;
    rjult -= 24.0;
  }

  juld = (double)jd + (rjult/24.0) - 2400000.5;

  return(juld-1.0);
}


/* Given a month, day, year, hour, min; make a MJD */
double getmjd(int iy, int im, int id, int hr, int min)
{
  int jm, jy, ja, jd;
  double rjult, juld, ut;

  ut = (double)hr + ((double)min / 60.0);

  if(im > 2)
  {
    jy = iy;
    jm = im + 1;
  }
  else
  {
    jy = iy - 1;
    jm = im + 13;
  }

  jd = (int)(365.25 * (double)jy) + (int)(30.6001 * (double)jm) + id + 1720995;

  if((id + 31 * (im + 12 * iy)) > GREG)
  {
    ja = (int)(0.01 * (double)jy);
    jd = jd + 2 - ja + (int)(0.25 * ja);
  }

  rjult = ut + 12.0;

  if(rjult > 24.0)
  {
    jd++;
    rjult -= 24.0;
  }

  juld = (double)jd + (rjult/24.0) - 2400000.5;

  return(juld-1.0);
}
