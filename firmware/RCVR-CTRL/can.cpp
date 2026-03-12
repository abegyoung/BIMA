#include "RCVR-CTRL.h"
#include <FlexCAN_T4.h>

// Command Frames
// CAN 8-byte payload is [addr0 val0 addr1 val1 addr2 val2 addr3 val3]
const uint16_t GUNNPOWER[2] = {0x6C, 0x40}; //val0 [0x40 = ON 0x00 = OFF]
const uint16_t MMOSC_SEL[2] = {0x6C, 0x0C}; // 
const uint16_t MM_SWEEP[2]  = {0x6C, 0x80};
const uint16_t GUNNVOLTS[2] = {0x64, 0x65};
const uint16_t YIGVOLTS[2]  = {0x52, 0x53};


// Telemetry Frames                  byte0 | byte1 | byte2 | byte3 | byte4 | byte5 | byte6 | byte7
#define FRM103 0x0A07F217 // 0x103                 |               |         XLOCK |
#define FRM105 0x0A0BF217 // 0x105                 |               | MMLOCK        |
#define FRM106 0x0A0DF217 // 0x106   OSC_A_TunePOS |               |     VopAD     |  X_IF_LVL
#define FRM107 0x0A0FF217 // 0x107                 | OSC_A_BS_POS  |               | MM_PHS_ERROR
#define FRM108 0x0A11F217 // 0x108                 |               |               | OSC_B_TunePOS
#define FRM109 0x0A13F217 // 0x109                 |  YIG_MON      |  MM_IF_LVL    |
#define FRM10A 0x0A15F217 // 0x10A                 | OSC_B_BS_POS  |               |
#define FRM10B 0x0A17F217 // 0x10B     Att_B_POS   |               |  Att_D_POS    |  X_PHS_ERROR
#define FRM124 0x0A49F217 // 0x124                 |               |     Vop_B     |

int sendTelemetry(uint32_t frame_id, uint64_t *data) {
   
   CAN_message_t msg;

   msg.id = frame_id;
   msg.len = 8;
   msg.flags.extended = 1;

   uint64_t val = *data;
   uint8_t *p = (uint8_t*)&val;

   for (size_t i=0; i<8; i++) {
     msg.buf[i] = p[7-i];
   }

   if (Can1.write(msg)) {
     //Serial.printf("Sent frame");
     return 0;
   } else {
     //Serial.printf("Failed to send Telemetry frame");
     return -1;
   }

}

int checkCommand(const CAN_message_t &msg) {

  return 0;

}

void printFrame(const CAN_message_t &msg) {
  // Timestamp (if supported by library build)
  // Not all builds populate msg.timestamp; guard its use loosely.
  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] ");

  // ID (hex), length
  Serial.print("ID:0x");
  Serial.print(msg.id, HEX);
  Serial.print(" DLC:");
  Serial.print(msg.len);

  // Flags (try to print if present)
  // Many versions expose msg.flags.extended (true for 29-bit), msg.flags.remote (RTR)
  // If not present in your version, you can comment these two lines.
  #ifdef FLEXCAN_T4_h_
    Serial.print(" EXT:");
    Serial.print(msg.flags.extended ? 1 : 0);
    Serial.print(" RTR:");
    Serial.print(msg.flags.remote ? 1 : 0);
  #endif

  // Data bytes
  Serial.print(" DATA:");
  for (uint8_t i = 0; i < msg.len && i < 8; i++) {
    if (msg.buf[i] < 0x10) Serial.print('0');
    Serial.print(msg.buf[i], HEX);
    if (i + 1 < msg.len) Serial.print(' ');
  }
  Serial.println();
}

