#include "../Server.h"
#include "hardware.h"

extern uint8_t TX[16], RX[16];
extern void setSelect(int);
extern int transferSPI(uint8_t *, uint8_t *, size_t, int);


int setDAC(unsigned short channel, unsigned short value)
{
  unsigned int cmd;
  cmd=DAC_SET | ((channel << 20) & 0x00F00000) | ((value << 4) & 0x000FFFF0);
  spi.mode=1;
  TX[0]=(cmd>>24) & 0xFF;
  TX[1]=(cmd>>16) & 0xFF;
  TX[2]=(cmd>>8) & 0xFF;
  TX[3]=(cmd>>0) & 0xFF;  
  transferSPI( TX, RX, 4, NORM);
  spi.mode=0;
  return 0;
}


unsigned short getDAC(unsigned short channel)
{
  unsigned short result;
  spi.mode=1;
  TX[0]=0x08; TX[1]=0x00; TX[2]=0x00; TX[3]=0x02; // set SDO
  transferSPI( TX, RX, 4, NORM);
  TX[0]=0x10; TX[1]=(channel << 4); TX[2]=0x00; TX[3]=0x00; // read channel
  transferSPI( TX, RX, 4, NORM);
  TX[0]=0x0E; TX[1]=0x00; TX[2]=0x00; TX[3]=0x00;  // nop, read sdo
  transferSPI( TX, RX, 4, NORM);
  result = ((0xF0 & RX[3]) >> 4) + ((0x0F & RX[2]) << 4) + ((0xF0 & RX[2]) << 4) + ((0x0F & RX[1]) << 12);
  TX[0]=0x08; TX[1]=0x00; TX[2]=0x00; TX[3]=0x00;  // reset SDO
  transferSPI( TX, RX, 4, NORM);  
  spi.mode=0;
  return result; 
}


void initDAC(void)
{
  TX[0]=0x08; TX[1]=0x00; TX[2]=0x00; TX[3]=0x00;
  for(int i=0; i < 3; i++)
    {
      setSelect(i);      // select each DAC
      transferSPI( TX, RX, 4, NORM);
    }
}
