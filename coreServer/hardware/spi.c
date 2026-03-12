#include "../Server.h"

#ifdef SIM
void transferSPI(uint8_t *tx, uint8_t *rx, size_t len, int holdCS)
{
}
#else

#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>  
#include <sys/ioctl.h>

#ifdef LINUX
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#else
#include <dev/spi/spi_io.h>
#endif

#include "hardware.h"

extern void setGPIO(int, int);

#ifdef LINUX
int claimSPI()
{
  int fd;
  spi.bits=8;
  spi.device = "/dev/spidev1.1";
  fd = open(spi.device, O_RDWR);
  if(fd < 0)
  {
     close(fd);
     return -1;
  }
  else
  {
     ioctl(fd, SPI_IOC_WR_MODE, &spi.mode);
     ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi.bits);
     ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi.speed);
  }
  return fd;
}

#else  // NetBSD
int claimSPI()
{
  int fd;
  spi.bits=8;
  spi_ioctl_configure_t sic;
  sic.sic_addr  = spi.cs;
  sic.sic_mode  = spi.mode;
  sic.sic_speed = spi.speed;
  spi.device = "/dev/spi1";
  fd = open(spi.device, O_RDWR);
  if (fd >= 0)
    {
      if (ioctl(fd, SPI_IOCTL_CONFIGURE, &sic) == -1)
	{
	  close(fd);
	  return -1;
	}
    }
  return fd;
}
#endif

int releaseSPI(int fd)
{
  return(close(fd));
}

#ifdef LINUX
void transferSPI(uint8_t *tx, uint8_t *rx, size_t len, int holdCS)
{
  int fd;
  struct spi_ioc_transfer tr = {};
  memset(&tr,0,sizeof(tr));

  if(!holdCS)  // normal case
    setGPIO(CS, LOW);
  fd=claimSPI();
  tr.tx_buf = (unsigned long)tx;
  tr.rx_buf = (unsigned long)rx;
  tr.len = len;
  if(ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1)
    printf("can't send spi message\n");  
  releaseSPI(fd);
  if(!holdCS)
    setGPIO(CS, HIGH);
}

#else
void transferSPI(uint8_t *tx, uint8_t *rx, size_t len, int holdCS)
{
  int fd;
  spi_ioctl_transfer_t sit;
  sit.sit_addr = spi.cs;
  sit.sit_send = tx;
  sit.sit_recv = rx;
  sit.sit_sendlen = sit.sit_recvlen = len;
  if(!holdCS)  // normal case
    setGPIO(CS, LOW);
  fd=claimSPI();
  if(ioctl(fd, SPI_IOCTL_TRANSFER, &sit))
    printf("can't send spi message\n");  
  releaseSPI(fd);
  if(!holdCS)
    setGPIO(CS, HIGH);
}
#endif

#endif
