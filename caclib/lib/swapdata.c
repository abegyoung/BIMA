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

#include <math.h>
#include "caclib_proto.h"

/* Returned swapped double; leave passed double alone */
double swapd( d )
double *d;
{
  union {
    double d;
    unsigned char c[sizeof(double)];
  }u;
  unsigned char t;

  u.d = *d;

  t = u.c[0];
  u.c[0] = u.c[7];
  u.c[7] = t;

  t = u.c[1];
  u.c[1] = u.c[6];
  u.c[6] = t;

  t = u.c[2];
  u.c[2] = u.c[5];
  u.c[5] = t;

  t = u.c[3];
  u.c[3] = u.c[4];
  u.c[4] = t;

  return(u.d);
}


/* Returned swapped long; leave passed long alone */
long swapl(l)
long *l;
{
  union {
    long l;
    unsigned char c[4];
  }u;
  unsigned char temp;

  u.l = *l;
  temp = u.c[0];
  u.c[0] = u.c[3];
  u.c[3] = temp;

  temp = u.c[1];
  u.c[1] = u.c[2];
  u.c[2] = temp;

  return(u.l);
}


/* Returned swapped int; leave passed int alone */
int swapi(l)
int *l;
{
  union {
    int l;
    unsigned char c[4];
  }u;
  unsigned char temp;

  u.l = *l;
  temp = u.c[0];
  u.c[0] = u.c[3];
  u.c[3] = temp;

  temp = u.c[1];
  u.c[1] = u.c[2];
  u.c[2] = temp;

  return(u.l);
}


/* Returned swapped short; leave passed short alone */
unsigned short swaps(s)
unsigned short *s;
{
  union {
    unsigned s;
    unsigned char c[2];
  }u;
  unsigned char temp;

  u.s = *s;
  temp = u.c[0];
  u.c[0] = u.c[1];
  u.c[1] = temp;

  return(u.s);
}


/* Swap passwd unsigned short in place; return 0 */
int in_swapus(s)
unsigned short *s;
{
  union SWAPS {
    unsigned short *s;
    unsigned char c[2];
  }*u;
  unsigned char t;

  u = (union SWAPS *)s;
  
  t = u->c[0];
  u->c[0] = u->c[1];
  u->c[1] = t;

  return(0);
}


/* Swap passwd float in place; return 0 */
int in_swapf(f)
float *f;
{
  union SWAPF {
    float f;
    unsigned char c[4];
  }*u;
  unsigned char t;

  u = (union SWAPF *)f;
  
  t = u->c[0];
  u->c[0] = u->c[3];
  u->c[3] = t;

  t = u->c[1];
  u->c[1] = u->c[2];
  u->c[2] = t;

  return(0);
}


/* Swap passwd signed short in place; return 0 */
int in_swaps(s)
short *s;
{
  union SWAPS {
    short s;
    unsigned char c[2];
  }*u;
  unsigned char t;

  u = (union SWAPS *)s;
  
  t = u->c[0];
  u->c[0] = u->c[1];
  u->c[1] = t;


  return(0);
}


/* Swap passwd double in place; return 0 */
int in_swapd(d)
double *d;
{
  union SWAPD {
    double d;
    unsigned char c[4];
  }*u;
  unsigned char t;

  u = (union SWAPD *)d;
  
  t = u->c[0];
  u->c[0] = u->c[7];
  u->c[7] = t;

  t = u->c[1];
  u->c[1] = u->c[6];
  u->c[6] = t;

  t = u->c[2];
  u->c[2] = u->c[5];
  u->c[5] = t;

  t = u->c[3];
  u->c[3] = u->c[4];
  u->c[4] = t;

  return(0);
}


/* Swap passwd long in place; return 0 */
int in_swapl(ll)
long *ll;
{
  union SWAPLL {
    long l;
    unsigned char c[4];
  }*u;
  unsigned char t;

  u = (union SWAPLL *)ll;
  
  t = u->c[0];
  u->c[0] = u->c[3];
  u->c[3] = t;

  t = u->c[1];
  u->c[1] = u->c[2];
  u->c[2] = t;

  return(0);
}


/* Swap passwd int in place; return 0 */
int in_swapi(ii)
int *ii;
{
  union SWAPII {
    int i;
    unsigned char c[4];
  }*u;
  unsigned char t;

  u = (union SWAPII *)ii;
  
  t = u->c[0];
  u->c[0] = u->c[3];
  u->c[3] = t;

  t = u->c[1];
  u->c[1] = u->c[2];
  u->c[2] = t;

  return(0);
}
