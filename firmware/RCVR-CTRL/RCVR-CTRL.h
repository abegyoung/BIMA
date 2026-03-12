#include <cstdint>
#include <FlexCAN_T4.h>

#define WHITESPACE(ch) ( (ch)==' ' || (ch)=='\t' || (ch)=='\n' || (ch)=='\r' )

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

#define Tuner_LL	10500
#define Tuner_UL	17700
#define Backshort_LL	8200
#define Backshort_UL	23000

typedef struct
{
    uint16_t *dacs;
    uint8_t blanking; //0=no msgs, 1=can only, 2=can+serial
    uint8_t system;
} database;
extern database *currentValues;

extern FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;

