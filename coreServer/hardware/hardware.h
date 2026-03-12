#define MUXONLY 0
#define FLIPFLOP 1
#ifdef LINUX
#define CS  157
#else
#define CS  29
#endif
#define NORM 0
#define HOLDCS 1
#define DAC_SET 0x02000000
#define LOW 0
#define HIGH 1

struct SPI {
  char *device;
  int cs;
  uint8_t bits;
  uint32_t mode;
  uint32_t speed;
};

extern struct SPI spi;
