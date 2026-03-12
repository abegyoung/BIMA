/*
* File:     pca8574.cpp
*
* Author:   A. Young (young@physics.arizona.edu)
* Reviewer: I. W. A.
* Date:     01/01/2020
* Project:  GUSTO (Galactic / Extragalactic ULDB Spectroscopic Terahertz Observatory)
* 
* Function: void writeRegister(reg, val)        write to CTRL BOARD I/O 
*           int readRegsiter(reg)
*           void writePSatRegister(reg, val)
*           int readPSatRegsiter(reg)
*           void writePowerRegister(reg, val)
*           int readPowerRegsiter(reg)
*
* Description: Code to read/write to NXP 8-bit I/O expanders with SPI bus.  Seperate control
*              of (8) CTRL board DCDC enables on SPI0, or POWER BOARD ADC MUX lines on SPI1
*
* Changelog:
*       01/01/2020 AGY Initial prototype B1LO control board
*
* Last Version: 1.0.0.1
*
*/

#include <Arduino.h>
#include <Wire.h>
#include "pca8574.h"
#include "pins.h"

// write to PCA8574 over I2C
void writeRegister(byte thisValue, int chip) {
  int PCA8574_ADDRESS;

  if (chip==1) {
    PCA8574_ADDRESS = PCA8574_ADDRESS1;
  } else if (chip==2) {
    PCA8574_ADDRESS = PCA8574_ADDRESS2;
  }
  Wire.beginTransmission(PCA8574_ADDRESS);   // I2C address of PCA9534
  Wire.write(thisValue);                     // Data to write
  Wire.endTransmission();
}

// read from PCA8574 over I2C
uint8_t readRegister(int chip) {
  int PCA8574_ADDRESS;
  uint8_t result = 0;

  if (chip==1) {
    PCA8574_ADDRESS = PCA8574_ADDRESS1;
  } else if (chip==2) {
    PCA8574_ADDRESS = PCA8574_ADDRESS2;
  }

  // Request one byte from that register
  Wire.requestFrom(PCA8574_ADDRESS, 1);
  if (Wire.available()) {
    result = Wire.read();
  }

  return result;
}

