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

#define PI		(M_PI)
#define DEG_TO_RAD	PI/180.0
#define RAD_TO_DEG	180.0/PI

double  deg_sin(double);
double  deg_cos(double);
double  deg_tan(double);
double  deg_asin(double);
double  deg_acos(double);
double  deg_atan(double);

/* Calculates trigonometric fn's with 'angle' in degrees, not radians.  */

double deg_sin(angle)
double angle;
{
  return( sin(DEG_TO_RAD * angle) );
}

double deg_cos(angle)
double angle;
{
  return( cos(DEG_TO_RAD * angle) );
}

double deg_tan(angle)
double  angle;
{
  return( tan(DEG_TO_RAD * angle) );
}

double deg_asin(value)
double  value;
{
  return( RAD_TO_DEG*asin(value) );
}

double deg_acos(value)
double  value;
{
  return( RAD_TO_DEG*acos(value) );
}

double deg_atan(value)
double  value;
{
  return( RAD_TO_DEG*atan(value) );
}

double deg_atan2(top, bot )
double  top, bot;
{
  return( RAD_TO_DEG*atan2(top, bot) );
}


