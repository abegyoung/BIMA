#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

void *can_receiver_thread(void *);
extern int writeCan(uint32_t, uint64_t);

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

#define SAMBUS_ON_SRC           0X000001        // 00
#define SAMBUS_TRK              0X000002        // 01
#define SAMBUS_NV               0X000004        // 02
#define SAMBUS_NUT              0X000008        // 03
#define SAMBUS_PTUA             0X000010        // 04
#define SAMBUS_UPS              0X000020        // 05
#define SAMBUS_GEN              0X000040        // 06
#define SAMBUS_RXL1             0X000080        // 07
#define SAMBUS_RXL2             0X000100        // 08
#define SAMBUS_100M             0X000200        // 09
#define SAMBUS_RUBIDIUM         0X000400        // 10
#define SAMBUS_SPARE0B          0X000800        // 11
#define SAMBUS_DOTF             0X001000        // 12
#define SAMBUS_SOTF             0X002000        // 13
#define SAMBUS_SR               0X004000        // 14
#define SAMBUS_NS               0X008000        // 15
#define SAMBUS_EW               0X010000        // 16
#define SAMBUS_AX               0X020000        // 17
#define SAMBUS_DV               0X040000        // 18
#define SAMBUS_VNP              0X080000        // 19
#define SAMBUS_LCK              0X100000        // 20
#define SAMBUS_SPARE15          0X200000        // 21
#define SAMBUS_SPARE16          0X400000        // 22
#define SAMBUS_SPARE17          0X800000        // 23

