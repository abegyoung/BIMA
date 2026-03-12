#include "../Server.h"
#include <math.h>
#include "hardware.h"

extern int transferSPI(uint8_t *, uint8_t *, size_t, int);
extern uint8_t TX[16], RX[16];
extern void setGPIO(int, int);

float * readADC(int channel, int average)
{
  unsigned short val;
  unsigned int jmax=1, i, j, max=average;
#ifdef LINUX
  unsigned int convtime=1000;
#else
  unsigned int convtime=0;
#endif
  float stdev=0.0, sum=0.0, volt[256];
  static float ret[2];
  short scan = 0x00;

  if(!(average % 8)) // special adc scan mode
  {
     max = average/8;
     jmax=8;
     scan = 0x18;
  }
  
  for(i=0; i < max; i++)
    {
      setGPIO(CS,LOW);
      TX[0]=0x01 + scan + channel*0x20;
      transferSPI(TX, RX, 1, HOLDCS);
      usleep(convtime);
      for(j=0;j<jmax*2;j++)
         TX[j]=0x00;
      transferSPI(TX, RX, jmax*2, HOLDCS);
      setGPIO(CS,HIGH);
      for(j=0;j<jmax;j++)
      {
        val = (RX[2*j] << 8) | RX[2*j+1];
        volt[jmax*i+j] = (float)val/65535.0*4.07;
        sum += volt[jmax*i+j];
      }
    }
  sum /= average;
  for(i=0; i<average; i++)
    stdev += pow((double)volt[i] - (double)sum, 2.0);

  stdev = (float)sqrt(stdev/(double)average);
  ret[0] = sum;
  ret[1] = stdev;
  
  return ret;
}  
