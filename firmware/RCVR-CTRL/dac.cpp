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
#include <SPI.h>
#include "dac.h"
#include "pins.h"


// write to MCP 4725 eval board 
/*
void writeDAC(uint16_t value){
  Wire.beginTransmission(0x60);
  Wire.write(0x40);
  Wire.write(value >> 4); //msb
  Wire.write((value & 0x0F) << 4); //lsb
  Wire.endTransmission();
}
*/

void writeDAC(uint16_t value, int8_t ch) {
  uint16_t dataOut = (ch<<14) | value;

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
  digitalWrite(SPI_CS,LOW);
  digitalWrite(LDAC, HIGH);

  SPI.transfer(dataOut >> 8);
  SPI.transfer(dataOut & 0xFF);

  digitalWrite(SPI_CS,HIGH);
  digitalWrite(LDAC, LOW);
  SPI.endTransaction();

}
