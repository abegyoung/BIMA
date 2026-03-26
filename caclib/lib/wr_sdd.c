/*
   linux version wr_sdd, does specific byte swapping
   swapping removed by twf 04/02/02.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <alloca.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>

#include "sdd.h"
#include "header.h"
#include "caclib_proto.h"

#define BLOCK 512
#define OFFSET(x,f)  ((short)(((int)&x->f - ((int)x))/8 + 1))

#define MAX_SCANS	16384  /* number of directory entrys, for new file */

static struct BOOTSTRAP boot;

int write_sdd(name, h,d)
char *name;
struct HEADER *h;
char *d;
{
  long fd, headlen, datalen, dirl, start;
  long datab, maxb, bytesinindex, bytesused;
  struct DIRECTORY dir;
  char *z;
  time_t itime;

  if( (fd = pdopen(name))<0)
    return(-1);

  bzero(&boot, sizeof(struct BOOTSTRAP));
  if( read( fd, &boot, sizeof(struct BOOTSTRAP))<0 ) {
    log_perror("read boot");
    close(fd);
    return(-2);
  }
  
  /* update bootstrap block */

  if( sizeof(struct DIRECTORY) != boot.bytperent ) {
    log_msg("directory sizes dont match\n");
    close(fd);
    return(-4);
  }

/* There could be holes but I'm not going to check for that */

  bytesinindex = boot.num_index_rec * boot.bytperrec;
  bytesused = boot.num_entries_used * boot.bytperent;
  
  if( bytesinindex < bytesused+sizeof(struct DIRECTORY) ) {
    log_msg("Directory structure of %s is full.\n", name );
    close(fd);
    return(-5);
  }

  if( boot.typesdd || !boot.version  ) {
    log_msg("SDD version or type of %s is invalid.\n", name );
    close(fd);
    return(-5);
  }

  headlen = sizeof(struct HEADER);
  h->headlen = (double)headlen;
  h->headlen = h->headlen;
  datalen = (long)h->datalen;
  
  datab =  blocks( headlen + datalen );
  boot.num_data_rec += datab;

  maxb = boot.num_data_rec + boot.num_index_rec;

  if( maxb < 0 ) {
    log_msg("Data file %s is full.\n", name );
    close(fd);
    return(-5);
  }

  if( maxb >= 1717986918 )
    log_msg("Data file is getting full. %d blocks left\n", 2147483647-maxb);

  boot.counter++;
  boot.num_entries_used++;

  if( lseek( fd, 0l, SEEK_SET ) <0 ) {
    log_perror("lseek boot");
    return(-7);
  }

  if( write_boot( fd, &boot, sizeof(struct BOOTSTRAP)) <0 ) {
    log_perror("write boot");
    return(-3);
  }

  /* update new directory */

  dirl = (boot.num_entries_used-1) * boot.bytperent + boot.bytperrec;

  if( lseek( fd, dirl, SEEK_SET ) < 0) {
    log_perror("lseek dir");
    return(-7);
  }

  bzero( &dir, sizeof(struct DIRECTORY) );

  dir.start_rec = boot.num_index_rec + boot.num_data_rec - datab + 1;
  dir.end_rec = boot.num_index_rec + boot.num_data_rec;
  dir.h_coord = h->xsource;
  dir.v_coord = h->ysource;
  strncpy( dir.source, h->object, 16 ); 
  padd(dir.source, 16);
  dir.scan = h->scan;
  dir.freq_res = index_freqres(h);
  dir.rest_freq = index_restfreq(h);
  dir.lst = h->lst;
  dir.ut = h->utdate;
  dir.obsmode = obsmode(h->obsmode);
  dir.phase_rec = -1;
  dir.pos_code = pos_code(h->coordcd);

  if( write_dir( fd, &dir, boot.bytperent ) <0 ) {
    log_perror("write dir");
    return(-3);
  }
  
  /* set header offsets */

  h->headcls = (short)13;
  h->oneptr =  OFFSET(h,headlen);
  h->twoptr =  OFFSET(h,xpoint);
  h->thrptr =  OFFSET(h,utdate);
  h->fourptr = OFFSET(h,epoch);
  h->fiveptr = OFFSET(h,tamb);
  h->sixptr =  OFFSET(h,scanang);
  h->sevptr =  OFFSET(h,bfwhm);
  h->eigptr =  OFFSET(h,appeff);
  h->nineptr = OFFSET(h,synfreq);
  h->tenptr =  OFFSET(h,openpar[0]);
  h->elvptr =  OFFSET(h,current_disk);
  h->twlptr =  OFFSET(h,obsfreq);
  h->trnptr =  OFFSET(h,nostac);

/* padd strings with spaces */

  padd(h->obsid, 8);
  padd(h->observer, 16);
  padd(h->telescop, 8);
  padd(h->projid, 8);
  padd(h->object, 16);
  padd(h->obsmode, 8);
  padd(h->frontend, 8);
  padd(h->backend, 8);
  padd(h->precis, 8);
  padd(h->pt_model,8);
  padd(h->cl11type,8);
/*
  padd(h->phastb01,32);
  padd(h->phastb02,32);
  padd(h->phastb03,32);
  padd(h->phastb04,32);
  padd(h->phastb05,32);
 */
  padd(h->rx_info,16);

  padd(h->coordcd,8);
  padd(h->frame,8);
  padd(h->veldef,8);
  padd(h->typecal,8);

  /* fill the header stop "burn_time" with the time it was written. */

  itime = time(NULL);
  h->burn_time = (double)itime;

  /* write data */

  start = (dir.start_rec-1) * boot.bytperrec;

  if( lseek(fd, start, SEEK_SET ) <0 ) {
    log_perror("lseek data");
    return(-7);
  }

  if( write( fd, h, headlen) <0 ) {
    log_perror("write head");
    return(-3);
  }

  if( write( fd, d, datalen ) <0 ) {
    log_perror("write data");
    return(-3);
  }

  /* complete block */
  dirl = (datalen + headlen) % boot.bytperrec;
  if( dirl > 0 ) {
    dirl = boot.bytperrec - dirl;
    z = (char *)alloca(dirl); 
    bzero(z,dirl);
    if( write( fd, z, dirl ) <0 ) {
      log_perror("write block_fill");
      return(-3);
    }
  }
  close(fd);
  return((boot.num_index_rec-1) * boot.bytperrec / boot.bytperent -
          boot.num_entries_used);
}

/* create new sdd file with (MAX_SCANS) 16384 possible scans (formerly 4096) */

int new_sdd(name)
char *name;
{
  int fd;
  struct BOOTSTRAP *boot = (struct BOOTSTRAP *)alloca(sizeof(struct BOOTSTRAP));

  if( (fd = open(name, O_CREAT | O_RDWR | O_EXCL, 0644 ))<0)
  {
    log_perror("open");

    return(1);
  }

//  fchown( fd, 2022, 2004 );

  bzero(boot, sizeof(struct BOOTSTRAP) );

  boot->num_index_rec = MAX_SCANS * sizeof(struct DIRECTORY)/BLOCK+1;
  boot->num_data_rec = 0; 
  boot->bytperrec = BLOCK;
  boot->bytperent = sizeof(struct DIRECTORY);
  boot->num_entries_used = 0;
  boot->counter = 0;
  boot->typesdd = 0;
  boot->version = 1;

  if( write_boot( fd, boot, BLOCK)<0) {
    log_perror("write");
    close(fd);
    return(1);
  }

  close(fd);
  return(0);
}

/* 
    returns last scan # out of last directory entry in pscan
    returns nxte ( which is # of scans in this file )
    returns  0 if empty
    returns -1 if failure
*/

int verify_sdd(name, pscan)
char *name;
float *pscan;
{
  int fd, dirl, bytesinindex, bytesused;
  struct BOOTSTRAP boot;
  struct DIRECTORY dir;

  *pscan = 0;

  if( (fd = pdopen(name))<0)
    return(-1);

  if( read( fd, &boot, sizeof(struct BOOTSTRAP))<0 ) {
    log_perror("read boot");
    close(fd);
    return(-1);
  }
  
  /* update bootstrap block */

  if( sizeof(struct DIRECTORY) != boot.bytperent ) {
    log_msg("directory sizes dont match\n");
    close(fd);
    return(-1);
  }

/* There could be holes but I'm not going to check for that */

  bytesinindex = boot.num_index_rec * boot.bytperrec;
  bytesused = boot.num_entries_used * boot.bytperent;
  
  if( bytesinindex < bytesused+sizeof(struct DIRECTORY) ) {
    log_msg("Directory structure of %s is full.\n", name );
    close(fd);
    return(-1);
  }

  if( boot.typesdd || !boot.version  ) {
    log_msg("SDD version or type of %s is invalid.\n", name );
    close(fd);
    return(-1);
  }

  if( sizeof(struct DIRECTORY) != boot.bytperent ) {
    log_msg( "File %s has faulty direcory size, can't verify", name );
    close(fd);
    return(-1);
  }

  if( boot.num_entries_used <= 0 ) {
    log_msg( "File %s is empty", name );
    close(fd);
    return(0);
  }
 
  dirl = (boot.num_entries_used-1) * boot.bytperent + boot.bytperrec;
  lseek( fd, dirl, SEEK_SET );

  if(read( fd, &dir, sizeof(struct DIRECTORY))!= sizeof(struct DIRECTORY)){
    log_perror("readdir");
    close(fd);
    return(-1);
  }

  *pscan = dir.scan;
  close(fd);
  return( boot.num_entries_used );
}

int pdopen(name)
char *name;
{
  int fd;

  if( (fd = open(name, O_RDWR, 0666 ))<0) {
/*
    extern int errno, sys_nerr;
    extern char *sys_errlist[];

    if( errno > sys_nerr )
      log_msg("%s open error number %d", name, errno );
    else
 */

    log_msg("Error opening: %s", name );

    return(-1);
  }

  return(fd);
}

int scan_cmp( f, d )
float f, d;
{
  int subf, subd;

  subf = (int)((f - (int)f) * 100.0 +0.5);
  subd = (int)((d - (int)d) * 100.0 +0.5);

  return( (int)f == (int)d && subf == subd );
}

/* return number of stupid fortran blocks given bytes  */
int blocks(b)
int b;
{
  if( b % BLOCK )
    return(b/BLOCK + 1);
  else
    return(b/BLOCK);
}

int obsmode(o)
char *o;
{
  int mask, mode;

  if( strncasecmp(o, "LINE", 4 )== 0 )
    mask = 512;
  else if( strncasecmp(o, "CONT", 4 )== 0 )
    mask = 256;
  else 
    mask = 0;

       if( strncasecmp(&o[4], "PS  ", 4)==0) mode = 1;
  else if( strncasecmp(&o[4], "APS ", 4)==0) mode = 2;
  else if( strncasecmp(&o[4], "FS  ", 4)==0) mode = 3;
  else if( strncasecmp(&o[4], "BSP ", 4)==0) mode = 4;
  else if( strncasecmp(&o[4], "TPON", 4)==0) mode = 5;
  else if( strncasecmp(&o[4], "TPOF", 4)==0) mode = 6;
  else if( strncasecmp(&o[4], "ATP ", 4)==0) mode = 7;
  else if( strncasecmp(&o[4], "PSM ", 4)==0) mode = 8;
  else if( strncasecmp(&o[4], "APM ", 4)==0) mode = 9;
  else if( strncasecmp(&o[4], "FSM ", 4)==0) mode = 10;
  else if( strncasecmp(&o[4], "TPMO", 4)==0) mode = 11;
  else if( strncasecmp(&o[4], "TPMF", 4)==0) mode = 12;
  else if( strncasecmp(&o[4], "DRFT", 4)==0) mode = 13;
  else if( strncasecmp(&o[4], "PCAL", 4)==0) mode = 14;
  else if( strncasecmp(&o[4], "BCAL", 4)==0) mode = 15;
  else if( strncasecmp(&o[4], "BLNK", 4)==0) mode = 16;
  else if( strncasecmp(&o[4], "SEQ ", 4)==0) mode = 17;
  else if( strncasecmp(&o[4], "FIVE", 4)==0) mode = 18;
  else if( strncasecmp(&o[4], "MAP ", 4)==0) mode = 19;
  else if( strncasecmp(&o[4], "FOC ", 4)==0) mode = 20;
  else if( strncasecmp(&o[4], "NSFC", 4)==0) mode = 21;
  else if( strncasecmp(&o[4], "TTIP", 4)==0) mode = 22;
  else if( strncasecmp(&o[4], "STIP", 4)==0) mode = 23;
  else if( strncasecmp(&o[4], "D_ON", 4)==0) mode = 24;
  else if( strncasecmp(&o[4], "CAL ", 4)==0) mode = 25;
  else if( strncasecmp(&o[4], "FSPS", 4)==0) mode = 26;
  else if( strncasecmp(&o[4], "EWFC", 4)==0) mode = 27;
  else if( strncasecmp(&o[4], "ZERO", 4)==0) mode = 28;

  else if( strncasecmp(&o[4], "TLPW", 4)==0) mode = 29;
  else if( strncasecmp(&o[4], "FQSW", 4)==0) mode = 30;
  else if( strncasecmp(&o[4], "NOCL", 4)==0) mode = 31;
  else if( strncasecmp(&o[4], "PLCL", 4)==0) mode = 32;
  else if( strncasecmp(&o[4], "ONOF", 4)==0) mode = 33;
  else if( strncasecmp(&o[4], "BMSW", 4)==0) mode = 34;
  else if( strncasecmp(&o[4], "PSSW", 4)==0) mode = 35;
  else if( strncasecmp(&o[4], "DRFT", 4)==0) mode = 36;
  else if( strncasecmp(&o[4], "OTF ", 4)==0) mode = 37;
  else if( strncasecmp(&o[4], "S_ON", 4)==0) mode = 38;
  else if( strncasecmp(&o[4], "S_OF", 4)==0) mode = 39;
  else if( strncasecmp(&o[4], "QK5 ", 4)==0) mode = 40;
  else if( strncasecmp(&o[4], "QK5A", 4)==0) mode = 41;
  else if( strncasecmp(&o[4], "PS-1", 4)==0) mode = 42;
  else if( strncasecmp(&o[4], "VLBI", 4)==0) mode = 43;
  else if( strncasecmp(&o[4], "PZC ", 4)==0) mode = 44;
  else if( strncasecmp(&o[4], "CPZM", 4)==0) mode = 45;
  else if( strncasecmp(&o[4], "PSPZ", 4)==0) mode = 46;
  else if( strncasecmp(&o[4], "CPZ1", 4)==0) mode = 47;
  else if( strncasecmp(&o[4], "CPZ2", 4)==0) mode = 48;
  else if( strncasecmp(&o[4], "CCPZ", 4)==0) mode = 49;
  else if( strncasecmp(&o[4], "CCPM", 4)==0) mode = 50;
  else if( strncasecmp(&o[4], "TPPZ", 4)==0) mode = 51;
  else if( strncasecmp(&o[4], "OPT ", 4)==0) mode = 52;
  else mode = 0;

  return(mode+mask);
}

int pos_code(s)
char *s;
{
       if( strncmp( s, "GALACTIC", 8 )==0) return 1;
  else if( strncmp( s, "1950RADC", 8 )==0) return 2;
  else if( strncmp( s, "EPOCRADC", 8 )==0) return 3;
  else if( strncmp( s, "MEANRADC", 8 )==0) return 4;
  else if( strncmp( s, "APPRADC",  7 )==0) return 5;
  else if( strncmp( s, "APPHADC",  7 )==0) return 6;
  else if( strncmp( s, "1950ECL",  7 )==0) return 7;
  else if( strncmp( s, "EPOCECL",  7 )==0) return 8;
  else if( strncmp( s, "MEANECL",  7 )==0) return 9;
  else if( strncmp( s, "APPECL",   6 )==0) return 10;
  else if( strncmp( s, "AZEL",     4 )==0) return 11;
  else if( strncmp( s, "USERDEF",  7 )==0) return 12;
  else if( strncmp( s, "2000RADC", 8 )==0) return 13;
  else if( strncmp( s, "INDRADC",  7 )==0) return 14;
  else return 0;
}

/* padd a string with spaces, fortran style */

int padd(s,n)
char *s;
int n;
{
  int i, flag = 0;

  for( i=0; i<n; i++ ) {
    if( !flag && s[i] == '\0' ) {
      flag = 1;
    }
    if( flag )
      s[i] = ' ';
  }
}

double index_freqres(h)
struct HEADER *h;
{
  double ret, sq;

  if( strncmp(h->obsmode, "CONT", 4 ) == 0 ) {
    sq =h->deltaxr * cos(h->ysource * M_PI / 180.0);
    ret = sqrt( sq * sq + h->deltayr * h->deltayr );
  } else
    ret = h->freqres;

  return(ret);
}

double index_restfreq(h)
struct HEADER *h;
{
  double ret;

  if( strncmp(h->obsmode, "CONT", 4 ) == 0 )
    ret = h->samprat;
  else
    ret = h->restfreq;

  return(ret);
}

int write_boot( fd, b, len )
int fd;
struct BOOTSTRAP *b;
int len;
{
  struct BOOTSTRAP bb;

  bcopy( b, &bb, sizeof(struct BOOTSTRAP));
  return write( fd, &bb, len );
}


int write_dir( fd, d, len )
int fd;
struct DIRECTORY *d;
int len;
{
  struct DIRECTORY dd;

  bcopy( d, &dd, sizeof(struct DIRECTORY));
  return write( fd, &dd, len );
}

struct BOOTSTRAP *last_bootb()
{
  return(&boot);
}

