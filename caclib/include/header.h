/*
 *  Cactus file %W%
 *        Date %G%
 *
 */

#ifndef HEADER_H

/**
 * @file header.h
 * @brief Main Header structure definition for POPS I/O.
 */

/* sdd.h -- Defined HEADER structures for POPS I/O */

#define getheads(a) swaps(a) /**< Macro to swap header bytes */

/**
 * @struct HEADER
 * @brief The main data header structure.
 * Contains metadata organized by classes.
 */
struct HEADER {
  short headcls;
  short oneptr;
  short twoptr;
  short thrptr;
  short fourptr;
  short fiveptr;
  short sixptr;
  short sevptr;
  short eigptr;
  short nineptr;
  short tenptr;
  short elvptr;
  short twlptr;
  short trnptr;
  short align1; /* makes class 1 align */
  short align2; /* makes class 1 align */

/* Class 1 */

  double headlen;
  double datalen;
  double scan;
  char obsid[8];
  char observer[16];
  char telescop[8];
  char projid[8];
  char object[16];
  char obsmode[8];
  char frontend[8];
  char backend[8];
  char precis[8];

/* Class 2 */

  double xpoint;
  double ypoint;
  double uxpnt;
  double uypnt;
  double ptcon[4];
  double orient;
  double focusr;
  double focusv;
  double focush;
  char pt_model[8];

/* Class 3 */

  double utdate;
  double ut;
  double lst;
  double norchan;
  double noswvar;
  double nophase;
  double cycllen;
  double samprat;
  char cl11type[8];

/* Class 4 */

  double epoch;
  double xsource;
  double ysource;
  double xref;
  double yref;
  double epocra;
  double epocdec;
  double gallong;
  double gallat;
  double az;
  double el;
  double indx;
  double indy;
  double desorg[3];
  char coordcd[8];

/* Class 5 */

  double tamb;
  double pressure;
  double humidity;
  double refrac;
  double dewpt;
  double mmh2o;

/* Class 6 */

  double scanang;
  double xzero;
  double yzero;
  double deltaxr;
  double deltayr;
  double nopts;
  double noxpts;
  double noypts;
  double xcell0;
  double ycell0;
  char frame[8];

/* Class 7 */

  double bfwhm;
  double offscan;
  double badchv;
  double rvsys;
  double velocity;
  char veldef[8];
  char typecal[8];

/* Class 8 */

  double appeff;
  double beameff;
  double antgain;
  double etal;
  double etafss;

/* Class 9 - Mt. Graham */

  double synfreq;
  double lofact;
  double harmonic;
  double loif;
  double firstif;
  double razoff;
  double reloff;
  double bmthrow;
  double bmorent;
  double baseoff;
  double obstol;
  double sideband;
  double wl;
  double gains;
  double pbeam[2];
  double mbeam[2];
  double sroff[4];
  double foffsig;
  double foffref1;
  double foffref2;

 /* Class 10 */

  double openpar[10]; 

 /* Class 11 */

/* removed 288 bytes of unused variables, or 36 doubles */

  double current_disk;
  double bologain;
  double sptip_start;
  double sptip_stop;
  double ramp_up;
  double tatms;
  double taus;
  double taui;
  double tatmi;
  double tchop;
  double tcold;
  double gaini;
  double count[3];
  char   linename[16];
  double refpt_vel;
  double tip_humid;
  double tip_ref_flag;
  double refract_45;
  double ref_correct;
  double beam_num;			  /* which beam of array rcvr this is */
  double burn_time;       /* the time at which the scan was acutally recorded */
  double parallactic;                           /* angle between north and up */
  double az_offset;                       /* az offset during spec five point */
  double el_offset;                       /* el offset during spec five point */
  double yfactor;                                         /* receiver yfactor */
  double rejection;                                     /* receiver rejection */
  double nutator_temp;    /* Temp inside nutator; used for fine pointing corr */
  double nutate_rate;                                          /* nutate freq */

/* if you add a variable above remove the appropriate number of doubles below */

  double spares05[5];


 /* Class 12 */

  double obsfreq;
  double restfreq;
  double freqres;
  double bw;
  double trx;
  double tcal;
  double stsys;
  double rtsys;
  double tsource;
  double trms;
  double refpt;
  double x0;
  double deltax;
  double inttime;
  double noint;
  double spn;
  double tauh2o;
  double th2o;
  double tauo2;
  double to2;
  char polariz[8];
  double effint;
  char rx_info[16];

/* Class 13 */

  double nostac;
  double fscan;
  double lscan;
  double lamp;
  double lwid;
  double ili;
  double rms;
  double align3[4];
};

#define HEADER_H

#endif
