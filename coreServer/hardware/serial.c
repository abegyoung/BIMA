#include "serial.h"
#include "hardware.h"

/* Open serial port */
int openSerial(char *devname) 
{
	struct termios t;
	int fd;
        if ((fd = open (devname, O_RDWR | O_NONBLOCK )) < 0) 
	{
	  sprintf (serial_errorstr, "serial_open :: Failed to open the serial device %s for cryocooler.", devname);
	  return _ERROR;
	}

	fcntl(fd, F_SETOWN, getpid());
	fcntl(fd, F_SETFL, FNDELAY);  // don't block in case cooler is off
	
	bzero (&t, sizeof (t));
	t.c_cflag &= ~PARENB;
	t.c_cflag &= ~CSTOPB;
	t.c_cflag &= ~CRTSCTS;
	t.c_cflag &= ~CSIZE;
	t.c_cflag |= (CS8 | CLOCAL | CREAD);
	t.c_iflag &= ~(IXON | IXOFF);
	t.c_iflag |= IGNPAR;
	t.c_oflag=0; t.c_lflag=0;
	
	// Copy input and output speeds to struct termios t. //
	if (cfsetispeed (&t, B460800) < 0)
		SERIAL_ERROR ("serial_open :: Error setting serial comm input speed.")
	if (cfsetospeed (&t, B460800) < 0)
		SERIAL_ERROR ("serial_open :: Error setting serial comm output speed.")
	// Throw away any input data (noise). //
	if (tcflush (fd, TCIFLUSH) < 0)
		SERIAL_ERROR ("serial_open :: Error executing serial serial flush.")
	// Now set the terminal port attributes. //
	if (tcsetattr (fd, TCSANOW, &t) < 0)
		SERIAL_ERROR ("serial_open :: Error setting serial terminal attributes.")

	Timeout.tv_usec = 5000000;
	Timeout.tv_sec  = 0;

	FD_SET(fd, &readfs);
	return fd;
}


int closeSerial (int fd) 
{
  // If there's an error while closing. //
  if (close (fd) < 0)
    SERIAL_ERROR ("serial_close :: Error closing the serial file descriptor.")
  return _NO_ERROR;
}


int tellSerial(int device, char *command, char *retbuffer)
{
  retbuffer[0]='\0';
  int fd=openSerial(devName[device]);
  if(write(fd, command, strlen(command)) > 0)
    {
      usleep(waitval[device] * serial_pause);
      read (fd, retbuffer, BUF_SIZE);  // nonblocking read
    }
  else
     SERIAL_ERROR("tellSerial :: Error writing to serial file descriptor.\n")
  closeSerial(fd);
  return(_NO_ERROR);
}
