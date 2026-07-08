#include "../coreServer/Server.h"
#include "../coreServer/CmdArr.h"
#include "../coreServer/Dispatch.h"
#include "../coreServer/hardware/can.h"
#include "Commands.h"
#include "rxServer.h"

#define BUF_SIZE 1024
extern unsigned long long reverse_payload(unsigned long long);
extern int tellUser(int, const char*, ...);
extern int tellSerial(int, char *, char *);

#define VYIGCAL_8 7.25
#define VYIGOFF_8 -7261
#define VYIGCAL_9 8.08166
#define VYIGOFF_9 -7182

// command callbacks go here

int server_initialize()
{
  return 0;
}

int server_shutdown()
{
  return 0;
}

int nop( int argc, char **argv, int fdout, int fderr )
{
  return 0;
}

typedef struct {
	int degree;
	double *coeffs;
} Polynomial;

Polynomial* init_polynomial(int degree, double *initial_coeffs) {
    Polynomial *p = (Polynomial*) malloc(sizeof(Polynomial));
    if (p == NULL) {
        perror("Failed to allocate memory for polynomial");
        exit(EXIT_FAILURE);
    }
    p->degree = degree;
    p->coeffs = (double*) malloc((degree + 1) * sizeof(double));
    if (p->coeffs == NULL) {
        perror("Failed to allocate memory for coefficients");
        free(p);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i <= degree; i++) {
        p->coeffs[i] = initial_coeffs[i];
    }
    return p;
}

double evaluate_polynomial(Polynomial *p, double x) {
    double result = 0.0;
    double x_power = 1.0;
    for (int i = 0; i <= p->degree; i++) {
        result += p->coeffs[i] * x_power;
        x_power *= x;
    }
    return result;
}

int sendStatus(int argc, char **argv, int fdout, int fderr)
{

  tellUser(fdout, "SERVER %d %d %d %.3f %.3f %.3f %s\n", server.BandSelect, server.YIGHarmonicN, server.GunnHarmonicM, server.GunnFreq/1000., server.LOFreq/1000., server.L_Band, (server.calState ==0) ? "SKY" : "CAL");

  return 0;
}


int sendCan(int argc, char **argv, int fdout, int fderr)
{
  uint32_t canid = (uint32_t)strtoul(argv[2], NULL, 16);
  uint64_t candata = (uint64_t)strtoull(argv[4], NULL, 16);

  writeCan(canid, candata);

  return 0;
}

int doCal(int argc, char **argv, int fdout, int fderr)
{
  int state = (int)strtod(argv[2], NULL);

  int device = 2; //GPIB DEVICE
  char cmd_buffer[24];
  char reply[BUF_SIZE];

  // SWITCH GPIB DEVICE 
  // SET GPIB ADDRESS OF USB DEVICE
  snprintf(cmd_buffer, sizeof(cmd_buffer), "++addr 8\n");
  tellSerial(device, cmd_buffer, reply);
  // SET AUTO READ FROM USB DEVICE
  snprintf(cmd_buffer, sizeof(cmd_buffer), "++auto 1\n");
  tellSerial(device, cmd_buffer, reply);
  // SET CR+LF USB DEVICE LINE ENDING
  snprintf(cmd_buffer, sizeof(cmd_buffer), "++eos 0\n");
  tellSerial(device, cmd_buffer, reply);

  /* DEBUG
  // READ Identification Query
  snprintf(cmd_buffer, sizeof(cmd_buffer), "*IDN?\n");
  printf("%s", cmd_buffer);
  tellSerial(device, cmd_buffer, reply);
  tellUser(fdout, "GPIB talker to %s", reply);
  */

  // SEND STATE to CAL pin
  snprintf(cmd_buffer, sizeof(cmd_buffer), "OUTP %d\n", state);
  tellSerial(device, cmd_buffer, reply);

  server.calState = state;

  return 0;

}


// Sets Lband synth frequency directly 
// Also sets the YIG Tune voltage for X-band lock based on required nth harmonic
// 
// Useful mostly for making Gunn tuning sheets in the lab
// i.e., 1) set the tuner to a micrometer position
//       2) set the backshort for peak power output on power meter
//       3) adjust the L-band synth until lock watching Gunn phase lock monitor and error voltage
//       4) determine mth harmonic of Gunn frequency (There are 3 possibilities for 3MM, 2 for 1MM)
//
// Jun 10, 2026
// Converted setLband() to setLband() which is called by Commands.h setLband and a seperate setLband_worker()
// which may be used by either the setLband command, or the setfreq command through setFreq() calling the worker
//
int setLband(int argc, char **argv, int fdout, int fderr)
{
  int band          = (int)strtod(argv[2], NULL);
  int YIGHarmonicN  = (int)strtod(argv[4], NULL);
  int GunnHarmonicM = (int)strtod(argv[6], NULL);
  float f_synth     = (float)strtof(argv[8], NULL);

  setLband_worker(band, YIGHarmonicN, GunnHarmonicM, f_synth);

  return 0;
}

int setLband_worker(int band, int YIGHarmonicN, int GunnHarmonicM, float f_synth)
{
  float y_int=0.;
  float slope=0.;
  float f_yig;

  int device = 2; //GPIB DEVICE
  char cmd_buffer[24];
  const char *prefix = "FR ";
  char reply[BUF_SIZE];

  // SET GPIB ADDRESS OF USB DEVICE
  snprintf(cmd_buffer, sizeof(cmd_buffer), "++addr 7\n");
  tellSerial(device, cmd_buffer, reply);

  // SET NO AUTO READ FROM USB DEVICE
  snprintf(cmd_buffer, sizeof(cmd_buffer), "++auto 0\n");
  tellSerial(device, cmd_buffer, reply);

  // SET CR+LF USB DEVICE LINE ENDING
  snprintf(cmd_buffer, sizeof(cmd_buffer), "++eos 3\n");
  tellSerial(device, cmd_buffer, reply);

  // SEND FR 1200.000000 MZ to synth
  snprintf(cmd_buffer, sizeof(cmd_buffer), "%s%.6f MZ\n", prefix, f_synth);
  tellSerial(device, cmd_buffer, reply);
  //printf("%s", cmd_buffer);

  //Set harmonic number
  if (YIGHarmonicN==8){
    y_int = -7261;	//Fit for YIG coarse tune (bits) to L band synth (MHz)
    slope = 7.25;	//Offset
    y_int = VYIGOFF_8;  //Fit for YIG coarse tune (bits) to L band synth (MHz)
    slope = VYIGCAL_8;  //Offset
  }
  else if (YIGHarmonicN==9){
    y_int = -7182;
    slope = 8.08166;
    y_int = VYIGOFF_9;  //Fit for YIG coarse tune (bits) to L band synth (MHz)
    slope = VYIGCAL_9;  //Offset
  }

  // Compute YIG coarse tuning
  int dac = f_synth * slope + y_int - 10;

  uint32_t canid;
  uint64_t candata;

  canid = 0x080;
  candata = ((uint64_t)0<<56)|((uint64_t)dac<<40);

  writeCan(canid, candata);

  // Once we're done setting the LO frequency, update the server struct before leaving
  server.BandSelect    = band;
  server.YIGHarmonicN  = YIGHarmonicN;
  server.GunnHarmonicM = GunnHarmonicM;
  server.L_Band        = f_synth;

  //update the rest of these based on the n,m harmonic numbers ala setFreq()
  if (server.BandSelect==3)
  {
    float f_yig = YIGHarmonicN * f_synth - 10.0; //MHz
    server.GunnFreq = (server.GunnHarmonicM * f_yig + 50.0); //GHz
    server.LOFreq = 1. * server.GunnFreq;
  }
  else if (server.BandSelect==1)
  {
    float f_yig = YIGHarmonicN * f_synth - 10.0; //MHz
    server.GunnFreq = (server.GunnHarmonicM * f_yig + 50.0); //GHz
    server.LOFreq = 3. * server.GunnFreq;
  }

  return 0;
}

int setFreq(int argc, char **argv, int fdout, int fderr)
{
  int band, m, n;
  float y_int, slope;
  float f_synth;
  float freq = (float)strtof(argv[2], NULL);

  char reply[BUF_SIZE];

  //Set harmonic number
  if (freq>=79.160 && freq<90.){
    band = 3;
    n = 8;		// X band YIG  output is nth harmonic of L band synth
    m = 9;		//MM band Gunn output is mth harmonic of X band synth
  }
  else if (freq>=90. && freq<100.){
    band = 3;
    n = 8;
    m = 10;
  }
  else if (freq>=100. && freq<113.35){
    band = 3;
    n = 9;
    m = 10;
  }
  else if (freq>=211.11 && freq<240.){
    band = 1;
    n = 8;
    m = 8;
  }
  else if (freq>=240. && freq<=273.04){
    band = 1;
    n = 8;
    m = 9;
  }
  else
    return -1;

  // Compute Synthesizer from millimeter LO freq using m, n harmonics
  freq = freq*1000.;
  if (band==3)
    f_synth = (((freq-50.)/m)+10.)/n;
  else if (band==1)
    f_synth = ((((freq/3.)-50.)/m)+10.)/n;


  setLband_worker(band, n, m, f_synth);

  // Compute Tuner and Backshort for H106 Carlstrom Gunn
  double Tuner[] = {1496.164355f, -35.604355f, 0.28455f, -0.000765f,};
  double Backshort[] = {-181688.5531468f, 7709.2387941f, -121.8067542f, 0.8503164f, -0.002214525f};

  Polynomial *tuner_poly = init_polynomial(3, Tuner);
  double result = evaluate_polynomial(tuner_poly, freq/1000.);
  int tuner_counts = -105.651487*result + 18203.8798;
  tellUser(fdout, "tuner = %.1f (%d)\n", result, tuner_counts, reply);
  tellUser(fdout, "motor motorA 32 %d\n", tuner_counts, reply);

  Polynomial *backshort_poly = init_polynomial(4, Backshort);
  result = evaluate_polynomial(backshort_poly, freq/1000.);
  int backshort_counts = -105.798227*result + 24846.692483;
  tellUser(fdout, "backshort = %.1f (%d)\n", result, backshort_counts, reply);
  tellUser(fdout, "motor motorB 32 %d\n", backshort_counts, reply);

  uint8_t motor;
  uint8_t speed;
  uint16_t position;

/*
  // Send CAN message to set Tuner Micrometer position
  motor=(uint8_t)0;
  speed=(uint8_t)24;
  position=(uint16_t)tuner_counts;
  canid = 0x084; //setMotor
  candata = ((uint64_t)motor<<56)|((uint64_t)speed<<48)|((uint64_t)position<<32);
  writeCan(canid, candata);

  sleep(10);

  // Send CAN message to set Backshort Micrometer position
  motor=(uint8_t)1;
  speed=(uint8_t)40;
  position=(uint16_t)backshort_counts;
  canid = 0x084; //setMotor
  candata = ((uint64_t)motor<<56)|((uint64_t)speed<<48)|((uint64_t)position<<32);
  writeCan(canid, candata);
*/

  return 0;
    
}

int getIFtotalpower(int argc, char **argv, int fdout, int fderr)
{
  unsigned int avg = (unsigned short)strtod(argv[2], NULL);

  tellUser(fdout, "IFTOTPOW %.6f\n", server.IFTOTPOW);

  return 0;
}

int doBump(int argc, char **argv, int fdout, int fderr)
{

  unsigned short chan = (unsigned short)strtol(argv[2], NULL, 10);
  unsigned short direction = (unsigned short)strtol(argv[4], NULL, 10);

  uint32_t canid;
  uint64_t candata;

  uint8_t motor;
  uint8_t speed;
  uint16_t position;
  uint8_t bump;

  motor=(uint8_t)chan;
  speed=(uint8_t)0;
  position=(uint16_t)0;
  bump = (uint8_t)direction+(uint8_t)1;

  canid = 0x084; //setMotor
  candata = ((uint64_t)motor<<56)|((uint64_t)speed<<48)|((uint64_t)position<<32)|(uint64_t)bump<<24;
  writeCan(canid, candata);

  return 0;

}

int setDAC(int argc, char **argv, int fdout, int fderr)
{
  unsigned short chan = (unsigned short)strtol(argv[2], NULL, 10);
  unsigned short dac = (unsigned short)strtol(argv[4], NULL, 10);

  uint32_t canid;
  uint64_t candata;

  //canid = (uint32_t) (0<<28)|(1<<27)|(0x080<<17)|(144<<9)|(21); // EXTID not working with TEENSY36 CAN2.0
  canid = 0x080;
  candata = ((uint64_t)chan<<56)|((uint64_t)dac<<40);

  writeCan(canid, candata);

  return 0;
}

int setTTL(int argc, char **argv, int fdout, int fderr)
{
  unsigned short mask = (unsigned short)strtol(argv[2], NULL, 10);

  //uint32_t canid = (uint32_t) (0<<28)|(1<<27)|(0x081<<17)|(144<<9)|(21); // EXTID not working with TEENSY36 CAN2.0
  uint32_t canid = 0x081;
  uint64_t candata = (uint64_t) mask<<56;

  writeCan(canid, candata);

  return 0;

}


int setMonitor(int argc, char **argv, int fdout, int fderr)
{
  unsigned short mask = (unsigned short)strtol(argv[2], NULL, 10);
  uint32_t canid;
  uint64_t candata;

  canid = (uint32_t) (0<<28)|(1<<27)|(0x3FB<<17)|(208<<9)|(29);
  candata = ((uint64_t)0xE11EA55AC300<<16)|((uint64_t)mask<<8)|((uint64_t)0);;
  writeCan(canid, candata);

  canid = (uint32_t) (0<<28)|(1<<27)|(0x3FB<<17)|(224<9)|(0);
  candata = ((uint64_t)0xE11EA55AC300<<16)|((uint64_t)mask<<8)|((uint64_t)0);;
  writeCan(canid, candata);

  return 0;
}


int runSweep(int argc, char **argv, int fdout, int fderr)
{

  unsigned short start = (unsigned short)strtol(argv[2], NULL, 10);
  unsigned short stop  = (unsigned short)strtol(argv[4], NULL, 10);
  unsigned short step  = (unsigned short)strtol(argv[6], NULL, 10);
  unsigned short power = (unsigned short)strtol(argv[8], NULL, 10);

  uint32_t canid;
  uint64_t candata;

  canid = (uint32_t) (0<<28)|(1<<27)|(0x086<<17)|(208<<9)|(29);
  candata = ((uint64_t)start<<48)|((uint64_t)stop<<32)|((uint64_t)step<<16)|((uint64_t)1<<8)|((uint64_t)power);
  writeCan(canid, candata);

  return 0;

}

int setFeedback(int argc, char **argv, int fdout, int fderr)
{
  unsigned short mask = (unsigned short)strtol(argv[2], NULL, 10);

  uint32_t canid = (uint32_t) (0<<28)|(1<<27)|(0x089<<17)|(208<<9)|(29);
  uint64_t candata = (uint64_t) mask<<56;

  writeCan(canid, candata);

  return 0;

}

int doVgap (int argc, char **argv, int fdout, int fderr)
{
  uint32_t canid = (uint32_t) (0<<28)|(1<<27)|(0x087<<17)|(208<<9)|(29);
  uint64_t candata = (uint64_t) 2<<56;

  writeCan(canid, candata);

  return 0;

}

int setBias(int argc, char **argv, int fdout, int fderr)
{

  unsigned short value = (unsigned short)strtol(argv[2], NULL, 10);

  uint32_t canid = (uint32_t) (0<<28)|(1<<27)|(0x080<<17)|(208<<9)|(29);
  uint64_t candata = (uint64_t) value<<48;

  writeCan(canid, candata);

  return 0;

}

int setIF(int argc, char **argv, int fdout, int fderr)
{

  union {
	  float f;
	  uint32_t i;
  } u;
  u.f = (float)strtof(argv[2], NULL);

  uint32_t canid = (uint32_t) (0<<28)|(1<<27)|(0x082<<17)|(224<<9)|(0);
  uint64_t candata = (uint64_t) u.i<<32;

  writeCan(canid, candata);

  return 0;

}

int setLNAdrain(int argc, char **argv, int fdout, int fderr)
{

  unsigned short value = (unsigned short)strtol(argv[2], NULL, 10);

  uint32_t canid = (uint32_t) (0<<28)|(1<<27)|(0x083<<17)|(208<<9)|(29);
  uint64_t candata = ((uint64_t)1<<56)|((uint64_t)value<<40);

  writeCan(canid, candata);

  return 0;

}

int setLNAgate(int argc, char **argv, int fdout, int fderr)
{

  unsigned short value = (unsigned short)strtol(argv[2], NULL, 10);

  uint32_t canid = (uint32_t) (0<<28)|(1<<27)|(0x085<<17)|(208<<9)|(29);
  uint64_t candata;

  candata = ((uint64_t)1<<56)|((uint64_t)value<<40);  //CAN DATA for Vg1
  writeCan(canid, candata);

  candata = ((uint64_t)2<<56)|((uint64_t)value<<40);  //CAN DATA for Vg2
  writeCan(canid, candata);

  return 0;

}

int setCanOut(int argc, char **argv, int fdout, int fderr)
{

  //unsigned short set = (unsigned short)strtol(argv[2], NULL, 10);

  pthread_mutex_lock(&destination_lock);
  destination = fdout;
  pthread_mutex_unlock(&destination_lock);

  return 0;
}
