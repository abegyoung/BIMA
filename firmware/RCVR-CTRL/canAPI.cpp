#include <Arduino.h>
#include "canAPI.h"
#include "can.h"
#include "dac.h"
#include "pca8574.h"
#include "RCVR-CTRL.h"
#include "drv8833.h"

void setDAC(const CAN_message_t &msg) {	// msgType 0x080
  uint8_t ch; 
  uint16_t value;
  ch = msg.buf[0];
  value = msg.buf[1]<<8|msg.buf[2];

  if(ch<4) {
    writeDAC(value, ch);
    currentValues->dacs[ch] = value;
    Serial.printf("RESPONSE DAC %d set to %d\n", ch, value);
  }

}

void setTTL(const CAN_message_t &msg) {	// msgType = 0x081
   uint8_t newvalue = msg.buf[0];
   writeRegister(newvalue, 1);
}

int setMONITOR(const CAN_message_t &msg) {	// msgType = 0x082
   uint8_t key[5] = {0xE1, 0x1E, 0xA5, 0x5A, 0xC3};

   for (int i=0; i<5; i++) {
      if(key[i]==msg.buf[i])
         continue;
      else {
         return -1; // Failed security key
      }
   }
   currentValues->blanking = msg.buf[6]; // Set Blanking Frame
   currentValues->system   = msg.buf[7]; // Set System Monitor Frame

   return 0;
}

void canECHO(const CAN_message_t &msg) {// msgType 0x083
   uint64_t val;

   for (int i=0; i<8; i++)
     val |= ((uint64_t) msg.buf[i]<<((7-i)*8));

   sendTelemetry(0x11C92015, &val);
}

void setMotor(const CAN_message_t &msg) {// msgType 0x084

   char *motor;
   uint8_t speed;
   uint8_t dir;
   uint16_t position;

   if ( msg.buf[0]==0 )
      motor = "motorA";
   else if ( msg.buf[0]==1 )
      motor = "motorB";
   else if ( msg.buf[0]==2 )
      motor = "motorC";

   speed = msg.buf[1];

   position = msg.buf[2]<<8|msg.buf[3];

   dir= msg.buf[4] - 1;

   if( !msg.buf[4] ){
      moveMotor(motor, speed, position);
   }
   else{
      bumpMotor(motor, dir);
      Serial.printf("doing bump on %s %s\n", motor, (dir == 0) ? "FWD" : "REV" );

   }

}
