#include <Arduino.h>
#include "adc.h"
#include "pins.h"
#include "drv8833.h"
#include "RCVR-CTRL.h"

// put these into adc.h when done
#define FWD 0
#define REV 1

// PWM DC motor via DRV8833 with a fixed length of time
int bumpMotor(char *motor, int8_t direction){
  int in1, in2;
  int bump_time, bump_pwm;

  if ( direction == FWD ){
    if(!strncmp((char *)motor, "motorA", 6)) {
      in1 = AIN1;
      in2 = AIN2;
      bump_time = 200;
      bump_pwm = 32;
    }
    if(!strncmp((char *)motor, "motorB", 6)) {
      in1 = BIN1;
      in2 = BIN2;
      bump_time = 500;
      bump_pwm = 40;
    }
    if(!strncmp((char *)motor, "motorC", 6)) {
      in1 = CIN1;
      in2 = CIN2;
      bump_time = 500;
      bump_pwm = 100;
    }
  }
  else if ( direction == REV ){
    if(!strncmp((char *)motor, "motorA", 6)) {
      in1 = AIN2;
      in2 = AIN1;
      bump_time = 200;
      bump_pwm = 32;
    }
    if(!strncmp((char *)motor, "motorB", 6)) {
      in1 = BIN2;
      in2 = BIN1;
      bump_time = 500;
      bump_pwm = 40;
    }
    if(!strncmp((char *)motor, "motorC", 6)) {
      in1 = CIN2;
      in2 = CIN1;
      bump_time = 500;
      bump_pwm = 100;
    }
  }
  else
    return -1;

  // enable DRV8833
  digitalWrite(SLP, HIGH);

  // start move
  digitalWrite(in1, LOW);    
  analogWrite(in2, bump_pwm);

  // wait
  delay(bump_time);

  // disable DRV8833
  digitalWrite(SLP, LOW);

  // reset PWM output to ZERO
  analogWrite(in2, 0);

  return 0;

}


// PWM speed control of DC motor via DRV8833 to a fixed position
int moveMotor(char *motor, int8_t speed, int16_t gotoPosition){
  int16_t currentPosition, lastPosition;
  int16_t currentDelta, lastDelta;
  int in1, in2;
  int adcCH;

  int db = 0;
  lastDelta = 100000; // a big number
  currentDelta = 0;   // a small number

  // Get current Position
  currentPosition = readADC(adcCH, 2);

  // Determine direction
  int direction = -1;
  if (currentPosition < gotoPosition) direction = FWD;
  else if (currentPosition > gotoPosition) direction = REV;
  else
    return -1;

  // Select driver pins via motorName and direction
  if ( direction == FWD ){
    if(!strncmp((char *)motor, "motorA", 6)) {
      in1 = AIN2;
      in2 = AIN1;
      adcCH=0;
    }
    if(!strncmp((char *)motor, "motorB", 6)) {
      in1 = BIN2;
      in2 = BIN1;
      adcCH=1;
    }
    if(!strncmp((char *)motor, "motorC", 6)) {
      in1 = CIN2;
      in2 = CIN1;
      adcCH=2;
    }
  }
  else if ( direction == REV ){
    if(!strncmp((char *)motor, "motorA", 6)) {
      in1 = AIN1;
      in2 = AIN2;
      adcCH=0;
    }
    if(!strncmp((char *)motor, "motorB", 6)) {
      in1 = BIN1;
      in2 = BIN2;
      adcCH=1;
    }
    if(!strncmp((char *)motor, "motorC", 6)) {
      in1 = CIN1;
      in2 = CIN2;
      adcCH=2;
    }
  }
  else
    return -1;

  // enable DRV8833
  digitalWrite(SLP, HIGH);

  // start move
  digitalWrite(in1, LOW);    
  analogWrite(in2, speed);

  // delay() in while loop until position reached
  while ( abs( gotoPosition-currentPosition ) > 10 ){

    currentPosition = readADC(adcCH, 2);
    currentDelta = abs( gotoPosition-currentPosition );

    if ( currentDelta < 20 ) {
      Serial.printf("Success!\n");
      break;}

    // Multiple levels of limit checking continue
    if(!strncmp((char *)motor, "motorA", 6)) {
      if ( (currentPosition>Tuner_UL) || (currentPosition<Tuner_LL) ) {
         Serial.printf("out of range\n");
         break;
      }
    }
    if(!strncmp((char *)motor, "motorB", 6)) {
      if ( (currentPosition>Backshort_UL) || (currentPosition<Backshort_LL) ) {
         Serial.printf("out of range\n");
         break;
      }
    }

    // Timeout limit (DEBUG)
    if ( db>20 ) { 
      Serial.printf("timed out\n");
      break;
    }
    db++;

    Serial.printf("%d %d %d %d\n", db, gotoPosition, currentPosition, currentDelta); // DEBUG
    delay(250);
  }

  // disable DRV8833
  digitalWrite(SLP, LOW);

  // reset PWM output to ZERO
  analogWrite(in2, 0);

  return 0;

}
