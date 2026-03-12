#define VYIGCAL_8 7.25
#define VYIGOFF_8 -7261
#define VYIGCAL_9 8.08166
#define VYIGOFF_9 -7182

#include <inttypes.h>
#include <pthread.h>

struct SERVER {
  unsigned int BIAS_API_NODE;
  unsigned int PAM_API_NODE;
  unsigned int RCVR_API_NODE;
  unsigned int BandSelect;
  unsigned int YIGHarmonicN;
  unsigned int GunnHarmonicM;
  float L_Band;
  float GunnFreq;
  float LOFreq;
} server;

