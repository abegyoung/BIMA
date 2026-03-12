#define SERIAL_ERRORSTR_SIZE 128
#define SERIAL_COMMSTR_SIZE  512

#define SERIAL_ERROR(str) {               \
              printf (str); \
              return  _ERROR;                \
            }

#define _ERROR    -1
#define _NO_ERROR  0
#define BUF_SIZE 1024


    // -----------------  INCLUDES  ----------------- //
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

char *devName[3] = {"/dev/ttyBIMA", "/dev/ttyCRYO", "/dev/ttyGPIB"};
speed_t serSpeed[3] = { B115200, B300, B460800 };
int waitval[3] = { 1, 1, 1 };
fd_set readfs;
char serial_errorstr [SERIAL_ERRORSTR_SIZE];
unsigned char databuf [SERIAL_COMMSTR_SIZE*100];
int serial_pause = 100000;

struct timeval Timeout;
