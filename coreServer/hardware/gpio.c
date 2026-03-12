#include <semaphore.h> 
#ifdef SIM
int getGPIO(int pin) { return(0); }
int setGPIO(int pin, int val) { return(0); }
int getLHe() { return(0); }
int setLHe(int val) { return(0); }
void setSelect(int val) { }
int trylockCard(int card, sem_t *sem) { return(0); }
int releaseCard(int card, sem_t *sem) { return(0); }

#else
#include "../Server.h"
#include <fcntl.h>
#ifdef LINUX
#include <sys/stat.h>
#else
#include <sys/gpio.h>
#include <sys/ioctl.h>
#endif

#define LEDPIN 2
#define ON 0
#define OFF 1

#ifdef LINUX
#define LHE1 125
#define LHE2 126

// gpios must be setup in advance or this won't work
int readGPIO(char *device, int pin)
{
  #define VALUE_MAX 30
  char path[VALUE_MAX];
  char value_str[3];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_RDONLY);
  if (-1 == fd)
    {
      fprintf(stderr, "Failed to open gpio value for reading!\n");
      return(-1);
    }
  if (-1 == read(fd, value_str, 3))
    {
      fprintf(stderr, "Failed to read value!\n");
      return(-1);
    }
  close(fd);
  return(atoi(value_str));
}


int writeGPIO(char *device, int pin, int value)
{
  static const char s_values_str[] = "01";
  char path[VALUE_MAX];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);
  if (-1 == fd)
    {
      fprintf(stderr, "Failed to open gpio value for writing!\n");
      return(-1);
    }
  if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
    {
      fprintf(stderr, "Failed to write value!\n");
      return(-1);
    }
  close(fd);
  return(0);
}

#else  // NetBSD
#define LHE1 29
#define LHE2 30

int readGPIO(char *device, int pin)
{
  int fd, val = -1;
  struct gpio_req gp;
  fd = open(device, O_RDONLY);
  if (fd != -1)
    {
      gp.gp_name[0] = '\0';
      gp.gp_pin = pin;
      ioctl(fd, GPIOREAD, &gp);
      val = gp.gp_value;
      close(fd);
    }
  return val;
}

int writeGPIO(char *device, int pin, int val)
{
  int fd;
  struct gpio_req gp;
  fd = open(device, O_RDWR);
  if (fd != -1) {
    gp.gp_name[0] = '\0';
    gp.gp_pin = pin;
    gp.gp_value = val;
    ioctl(fd, GPIOWRITE, &gp);
    close(fd);
    return 0;
  }
  return -1;
}

#endif


// the rest of these are wrappers around the functions above
int getGPIO(int pin)
{
  char dev[16]="/dev/gpio3";
  return(readGPIO(dev,pin));
}

int setGPIO(int pin, int val)
{
  char dev[16]="/dev/gpio3";  
  return(writeGPIO(dev, pin, val));
}

int getLHe()
{
  int val1=-1, val2=-1;
  char dev[16]="/dev/gpio5";
  val1 = readGPIO(dev,LHE1);
  val2 = readGPIO(dev,LHE2);  
  if((val1 != val2) || val1==-1 || val2==-1)
    return(-1);
  return (val1 & val2);
}

int setLHe(int val)
{
  char dev[16]="/dev/gpio5";
  return(writeGPIO(dev, LHE1, val) || writeGPIO(dev, LHE2, val));
}

int setLED(int val)
{
#ifdef LINUX
  return 0;
#else
  char dev[16]="/dev/gpio4";
  return(writeGPIO(dev, LEDPIN, val));
#endif
}


void setSelect(int val)
{
#ifdef LINUX
  int ioList[4] = {145, 144, 143, 142}; // CS0 thru CS3
#else
  int ioList[4] = {17, 16, 15, 14}; // CS0 thru CS3
#endif
  for(int i=0; i<4; i++)
    {
      setGPIO(ioList[i], (val >> i) & 0x1);
    }
}

int trylockCard(int card, sem_t *sem)
{
  setLED(ON);
  int ret=sem_wait(sem);
  setGPIO(card, ACTIVE);
  return(ret);
}

int releaseCard(int card, sem_t *sem)
{
  setLED(OFF);
  setGPIO(card, INACTIVE);
  return(sem_post(sem));
}
#endif
