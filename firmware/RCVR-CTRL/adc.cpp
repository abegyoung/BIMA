/*
* File:     dac.cpp
*
* Author:   A. Young (young@physics.arizona.edu)
* Reviewer: I. W. A.
* Date:     08/21/25
* Project:  BIMA (Berkeley Illinois Millimeter Array) 6-meter at UofA
* 
* Function: 
*
* Description: Code to control the TI DAC80004 16 bit Quad DACs
*              Imon expanded scale voltage offsets, and voice coil drive.
*
* Changelog:
*       08/21/2025 AGY Initial for prototype DAC
*
* Last Version: 0.9
*
*/

#include <Arduino.h>
#include <Wire.h>
#include "adc.h"        //ADS1115_ADDRESS =0x48
#include "pins.h"


// read from ADC
int16_t readADC(int ch, uint8_t addr){
  const int OS   = 0b1;     // start conversion
	int MUX;
  if( (addr==1) && (ch==0) )
        MUX  = (0b0<<2) | (ch&0x3); // MUX for differential channel 0, chip #1
  else
        MUX  = (0b1<<2) | (ch&0x3); // MUX
  const int PGA  = 0b001;   // +/- 4.096V gain
  const int MODE = 0b1;     // single shot
  const int DR   = 0b010;   // 32 samples/sec
  const int COMP = 0b0;     // comparator
  const int POL  = 0b0;     // polarity of RDY
  const int LAT  = 0b0;;    // no latching
  const int QUE  = 0b11;    // disable comparator & RDY

  uint16_t config = (OS<<15) | (MUX<<12) | (PGA<<9) | (MODE<<8) | (DR<<5) | (COMP<<4) | (POL<<3) | (LAT<<2) | QUE;

  int ADDRESS;

  if(addr == 1)
    ADDRESS = ADC_ADDRESS;
  else if(addr == 2)
    ADDRESS = MOTOR_ADS1115;

  // Write config register (0x01)
  Wire.beginTransmission(ADDRESS);
  Wire.write(0x01);                // Point to config register
  Wire.write(config >> 8);         // MSB
  Wire.write(config & 0xFF);       // LSB
  Wire.endTransmission();

  // Wait for conversion (~32ms at 64SPS)
  delay(64);

  // Set pointer to conversion register (0x00)
  Wire.beginTransmission(ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();

  // Read 2 bytes
  Wire.requestFrom(ADDRESS, 2);
  while (Wire.available() < 2);
  uint16_t result = (Wire.read() << 8) | Wire.read();

  return (int16_t)result;  // ADS1115 outputs signed 16-bit
}

