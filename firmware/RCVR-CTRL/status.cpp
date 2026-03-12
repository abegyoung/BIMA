/*
* File:     status.cpp
*
* Author:   A. Young (young@physics.arizona.edu)
* Reviewer: I. W. A.
* Date:     04/18/2023
* Project:  GUSTO (Galactic / Extragalactic ULDB Spectroscopic Terahertz Observatory)
* 
* Function: showStatus()
*
* Description: Displays status
*
* Changelog:
*	05/30/2023 AGY Initial functionality copy from B3LO status.cpp
*
* Last Version: 1.0.0.19
*
*/

#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include "status.h"
#include "pca8574.h"            //for readRegister()
#include "adc.h"
#include "RCVR-CTRL.h"
#include "can.h"

uint64_t val;

void showStatus(char *arg){


  if(!strncmp(arg, "slow", 4)) // TODO: this will be blanking frame (fast) or system frame (slow)
  {
    DACStatus();
    ADCStatus();
    TTLStatus();
  }
  else
  {
    DACStatus();
    ADCStatus();
    TTLStatus();
  }

}

void DACStatus(){
  if(currentValues->blanking>0) {
    val = ((uint64_t)currentValues->dacs[0]<<48) | \
	  ((uint64_t)currentValues->dacs[1]<<32) | \
	  ((uint64_t)currentValues->dacs[2]<<16) | \
	  ((uint64_t)currentValues->dacs[3]);
    sendTelemetry((uint32_t) 0x11C12000, &val);
  }

  if(currentValues->blanking==2) {
    for(int ch=0; ch<4; ch++)
      Serial.printf("DAC%d %d\n", ch, currentValues->dacs[ch]);
  }

}

void ADCStatus(){
  int16_t adc[8];

  // Get first 4 ADCs from chip 1, the second 4 from chip 2
  for(int ch=0; ch<8; ch++)
    adc[ch] = readADC(ch, (ch<4) ? 1 : 2);

  if(currentValues->blanking>0) {
    val = ((uint64_t)(adc[0] & 0xFFFF) << 48) |
          ((uint64_t)(adc[1] & 0xFFFF) << 32) |
          ((uint64_t)(adc[2] & 0xFFFF) << 16) |
          ((uint64_t)(adc[3] & 0xFFFF));
    sendTelemetry((uint32_t) 0x11C32000, &val);

    val = ((uint64_t)adc[4]<<48) | ((uint64_t)adc[5]<<32) | ((uint64_t)adc[6]<<16) | ((uint64_t)adc[7]);
    sendTelemetry((uint32_t) 0x11C52000, &val);
  }

  if(currentValues->blanking==2) {
    for(int ch=0; ch<4; ch++)
      Serial.printf("ADC%d %.3f\n", ch, 4.096*adc[ch]/32768.); // phaselock err as signed floats
    for(int ch=4; ch<8; ch++)
      Serial.printf("ADC%d %d\n",   ch, adc[ch]);		   // motor position as ints
  }

}

void TTLStatus(){
  uint8_t mask[2];

  mask[0] = readRegister(1);
  mask[1] = readRegister(2);

  if(currentValues->blanking>0) {
    val = ((uint64_t)0x000000000000<<16) | ((uint64_t)mask[1]<<8) | ((uint64_t)mask[0]);
    sendTelemetry((uint32_t) 0x11C72000, &val);
  }

  if(currentValues->blanking==2) {
    for(int i=0; i<2; i++)
      Serial.printf("TTL%d " BYTE_TO_BINARY_PATTERN"\n", i+1, BYTE_TO_BINARY(mask[i]));
  }
}
