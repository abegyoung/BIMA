#include <Wire.h>
#include <SPI.h>
#include <Bounce.h>
#include "adc.h"
#include "dac.h"
#include "status.h"
#include "pca8574.h"
#include "drv8833.h"
#include "RCVR-CTRL.h"
#include "pins.h"
#include "canAPI.h"
#include <FlexCAN_T4.h>

#define TRUE 1
#define FALSE 0
#define WHITESPACE(ch) ( (ch)==' ' || (ch)=='\t' || (ch)=='\n' || (ch)=='\r' )

#define NUM_MAILBOXES 9

// Define a global database variable for current states of type struct with elements:
// uint16_T *dacs[i]
// uint8_t  blanking
// uint8_t  system
database *currentValues;

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;


extern "C"
{
    struct usb_string_descriptor_struct
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint16_t wString[10];
    };

    extern struct usb_string_descriptor_struct usb_string_serial_number =
    {
         22,  // 2 + 2*length of the sn string
         3,
         {'B','I', 'M','A', '_', 'C', 'T','R', 'L', 0},
    };
}

//Handler for commanding interface
char *one_argument(char  *line, char *arg){
  char *end;
  while(WHITESPACE (*line)) line++;

  while(!WHITESPACE (*line) && *line != '\0') {
    *arg = *line;
    line++;
    arg++;
  }
  *arg = '\0';
  while(WHITESPACE (*line)) line++;
  for (end = line + strlen(line) - 1; WHITESPACE (*end); end--);
  *(end + 1) = '\0';

  return line;
}


void setup() {

  ///////////// COMM SETUP /////////////
  //SERIAL
  Serial.begin(115200);
  Serial.setTimeout(10000);

  //CAN
  Can1.begin();
  Can1.setMaxMB(NUM_MAILBOXES);
  Can1.setBaudRate(1000000);

  Can1.setMB( (FLEXCAN_MAILBOX)0, RX, EXT);	// 0x080 setDAC
  Can1.setMB( (FLEXCAN_MAILBOX)1, RX, EXT);	// 0x081 setTTL
  Can1.setMB( (FLEXCAN_MAILBOX)2, RX, EXT);	// 0x082 setMONITOR
  Can1.setMB( (FLEXCAN_MAILBOX)3, RX, EXT);	// 0x083 ECHO
  Can1.setMB( (FLEXCAN_MAILBOX)4, RX, EXT);	// 0x084 setMotor
  Can1.setMB( (FLEXCAN_MAILBOX)5, TX, EXT);	// 0x0E0 Blanking Frame DAC
  Can1.setMB( (FLEXCAN_MAILBOX)6, TX, EXT);	// 0x0E1 Blanking Frame ADC
  Can1.setMB( (FLEXCAN_MAILBOX)7, TX, EXT);	// 0x0E2 Blanking Frame ADC
  Can1.setMB( (FLEXCAN_MAILBOX)8, TX, EXT);	// 0x0E3 Blanking Frame TTL
  Can1.setMBFilter(REJECT_ALL);
  Can1.enableMBInterrupts();
  Can1.onReceive(MB0, setDAC);
  Can1.onReceive(MB1, setTTL);
  Can1.onReceive(MB2, setMONITOR);
  Can1.onReceive(MB3, canECHO);
  Can1.onReceive(MB4, setMotor);
  Can1.setMBFilter(MB0, 0x080); //msgType 0x080 API 144 SN 21 setDAC
  Can1.setMBFilter(MB1, 0x081); //msgType 0x081 API 144 SN 21 setTTL
  Can1.setMBFilter(MB2, 0x082); //msgType 0x082 API 144 SN 21 setMONITOR
  Can1.setMBFilter(MB3, 0x083); //msgType 0x083 API 144 SN 21 ECHO
  Can1.setMBFilter(MB4, 0x084); //msgType 0x084 API 144 SN 21 setMotor
  Can1.mailboxStatus();

  //SPI
  pinMode(SPI_CS, OUTPUT);
  pinMode(LDAC, OUTPUT);
  digitalWrite(SPI_CS, HIGH);
  digitalWrite(LDAC, LOW);

  ///////////// MOTOR SETUP /////////////
  // configure pins
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(CIN1, OUTPUT);
  pinMode(CIN2, OUTPUT);
  pinMode(SLP, OUTPUT);

  // disable DRV8833
  digitalWrite(SLP, LOW);

  // Start Communications
  SPI.begin();          //SPI
  Wire.begin();         //I2c

  // Interrupt setup
  pinMode(INT, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(INT), newState, FALLING);

  ///////////// REGISTER SETUP /////////////
  // Set all outputs low initially
  writeRegister(0x00, 1);

  // Set PCA8574 inputs high (Input mode)
  writeRegister(0x03, 2);

  // dynamically allocate database structure
  currentValues = malloc(sizeof(*currentValues));
  currentValues->dacs = (uint16_t *)malloc(4 * sizeof(uint16_t));
  currentValues->blanking= (int *)malloc(1 * sizeof(int));
  currentValues->blanking=1;

  // set default DACs
  delay(100);
  writeDAC(1621, 0);  //Xband YIG coarse tune 97.967 GHz
  writeDAC(0,    1);  //Gunn Vop 9.5V
  writeDAC(1990, 2);  //Gunn sweep gain 0V
  writeDAC(0,    3);  

  currentValues->dacs[0] = 1621;  
  currentValues->dacs[1] =    0;
  currentValues->dacs[2] = 1990;  
  currentValues->dacs[3] =    0;  

}

void newState() {
  uint8_t mask;
  mask = readRegister(2);
  Serial.printf("TTL1 " BYTE_TO_BINARY_PATTERN"\n", BYTE_TO_BINARY(mask));
  delay(500);
}

int16_t adc;
uint16_t dac;
elapsedMillis since;

void loop() {
  char *args;
  uint8_t buffer[30];

  // Whack in status poll.  TODO: move to secure thread
  if (since >= 1000) {
    since = since - 1000;
    if(currentValues->blanking!=0) {
      showStatus("fast");
    }
  }

  // Run the CAN events function in loop()
  // *Should be* unneccessary since all mailboxes have interrupt functions to call
  Can1.events();

  // Check for 
  if(Serial.available()){
    String string = Serial.readStringUntil('\r');
    uint8_t *buffer = (uint8_t*)string.c_str();

    const char *listOfDCDC[] = {"RF Select 0", "RF Select 1", "MMOsc Select 0", "MMOsc Select 1", \
                                "MMOsc Power", "MM Sweep", "CAL POS", "MM/CM POS", \
                                "MM PhaseLock Status", "MM Ref Status", "XBand Lock Status", "CAL POS", \
                                "MM/CM POS", "unused", "unused", "unused"};

    if(!strncmp((char *)buffer, "dcdc",4)){
      char cmd[25], arg1[25], arg2[25];
      int newvalue = 0;
      args = (char *)buffer;
      args = one_argument(args, cmd);
      args = one_argument(args, arg1);
      args = one_argument(args, arg2);

      if(!*arg1){
        //Read DCDC lines
        byte current = readRegister(1);
        for(int i=0; i<8; i++)
          Serial.printf("dcdc %d = %d\t%s\n", i, (current>>i) & 0x1, listOfDCDC[i]);

        //Read DCDC lines
        current = readRegister(2);
        for(int i=8; i<16; i++)
          Serial.printf("dcdc %d = %d\t%s\n", i, (current>>(i-8)) & 0x1, listOfDCDC[i]);
        Serial.printf("END\n");
      
      }

      if(*arg1){
        int channel  = atoi(arg1);        //channel to change (0-15)
        int value    = atoi(arg2);        //off or on (0 1)
        int chip;
        if(channel<8){
          chip = 1;
        } else if(channel>7 && channel<16){
          chip = 2;
          channel = channel-8;
        }

        byte current = readRegister(chip); // Read pin states

        if(channel<0 || channel >7)
          Serial.printf("RESPONSE channel # out of range\n");
        else
        {
          if(!value) newvalue = (current & ~(1<<channel));   //Set a 0 to turn OFF DCDC channel
          if(value)  newvalue = (current |  (1<<channel));   //Set a 1 to turn ON DCDC channel

          writeRegister(newvalue, chip); 
          Serial.printf("RESPONSE DCDC %d changed to %d was 0x%X now 0x%X\n", channel+8*(chip-1), value, current, newvalue);
        }
      }

    }

    if(!strncmp((char *)buffer, "dac",3)){
      char cmd[25], arg1[25], arg2[25];
      args = (char *)buffer;
      args = one_argument(args, cmd);
      args = one_argument(args, arg1);
      args = one_argument(args, arg2);
      int8_t ch = atoi(arg1);
      uint16_t value = atoi(arg2);
          
      writeDAC(value, ch);
      Serial.printf("RESPONSE DAC %d set to %d\n", ch, value);

      currentValues->dacs[ch] = value;  

    }

    if(!strncmp((char *)buffer, "adc",3)){
      char cmd[25], arg1[25];
      args = (char *)buffer;
      args = one_argument(args, cmd);
      args = one_argument(args, arg1);
      uint8_t ch = atoi(arg1);
          
      if ( ch>=0 && ch<4 )
        adc = readADC(ch, 1);
      else if ( ch>3 && ch<8 )
        adc = readADC(ch, 2);
      else{
        Serial.printf("out of range\n");
      }

      Serial.printf("RESPONSE ADC %d\n", adc);

    }

    if(!strncmp((char *)buffer, "motor",5)){
      char cmd[25], arg1[25],arg2[25],arg3[25];
      args = (char *)buffer;
      args = one_argument(args, cmd);
      args = one_argument(args, arg1);	// motorName = motorA, motorB, motorC
      args = one_argument(args, arg2);	// speed 
      args = one_argument(args, arg3);	// position
      char *motor = arg1;
      int8_t speed = atoi(arg2);
      int16_t position = atoi(arg3);

      // Multiple levels of limit checking
      if(!strncmp((char *)motor, "motorA", 6)) {
        if ( (position>Tuner_UL) || (position<Tuner_LL) ) {
           Serial.printf("out of range\n");
           return;
        }
      }
      if(!strncmp((char *)motor, "motorB", 6)) {
        if ( (position>Backshort_UL) || (position<Backshort_LL) ) {
           Serial.printf("out of range\n");
           return;
        }
      }
          
      Serial.printf("RESPONSE moving motor %s to position %d at speed %d\n", motor, position, speed);

      if( !moveMotor(motor, speed, position) )
        Serial.printf("RESPONSE move succesful\n");
      else
        Serial.printf("RESPONSE move failed\n");

    }

    if(!strncmp((char *)buffer, "bump",4)){
      char cmd[25], arg1[25],arg2[25];
      args = (char *)buffer;
      args = one_argument(args, cmd);
      args = one_argument(args, arg1);
      args = one_argument(args, arg2);
      char *motor = arg1;
      int8_t dir = atoi(arg2); //0=FWD, 1=REV
          
      Serial.printf("RESPONSE bumping motor %s %s... ", motor, (dir == 0) ? "FWD" : "REV" );

      if( !bumpMotor(motor, dir) )
        Serial.printf("RESPONSE move succesful\n");
      else
        Serial.printf("RESPONSE move failed\n");

    }

    if(!strncmp((char *)buffer, "id",2)){
      Serial.printf("ID BIMA RCVR CTRL\n"); 
    }

    if(!strncmp((char *)buffer, "status",4)){
      char cmd[25], arg1[25];
      args = (char *)buffer;
      args = one_argument(args, cmd);
      args = one_argument(args, arg1);

      if(!*arg1)                          //if no arg, call fast
      {
        showStatus("fast");
        Serial.printf("END\n");
      }
      else
      {
        showStatus((char *)arg1);       //if arg, pass to function (slow or fast)
        Serial.printf("END\n");         //"slow" will call all status, anything else will call fast
      }

    } // END status COMMAND

    if(!strncmp((char *)buffer, "scan",4)){
      byte error, address;

      for (address=1; address<127; address++){
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error==0){
          Serial.printf("I2C device found at 0x%X", address);
          if (address == PCA8574_ADDRESS1) Serial.printf("\tTTL output");
          if (address == PCA8574_ADDRESS2) Serial.printf("\tTTL input");
          if (address == ADC_ADDRESS) Serial.printf("\terror ADC");
          if (address == MOTOR_ADS1115) Serial.printf("\tmotor ADC");
          Serial.printf("\n");
        }
      }

    }

    if(!strncmp((char *)buffer, "help",4)){
      Serial.printf("\
dcdc <1-8> <0|1>            : Set DCDC <n> ON (1) or OFF (0)\n\
adc <0-3> <1|2>             : Read ADC <n> from unit 1 or 2\n\
dac <0-3>                   : \n\
motor <#> <dir> <spd> <pos> : <motor A,B,C> <0|1> <0-255> <0-32767>\n\
status                      : Display all parameters\n\
scan                        : Scan I2C bus for addresses\n\
id                          : Responds with board id\n\
help                        : Display this help\n");
      Serial.printf("END\n");
    } // END help COMMAND

  } // END Serial Available

} // END LOOP
