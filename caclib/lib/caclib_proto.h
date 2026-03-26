 
#include "caclib_sock.h"
 
/* cacstr.c */
int setCactusEnvironment(void);
int atox(char *s);
int getInteractive(void);
int smart_log_open(char *name, int offset);
int iamwho(char *user, char *display);
int sendProcessTime(int indx, int t, int pid);
int sendTimeStamp(int id);
int checkSendProc(int id, int pid);
int send_process_info_internal(int indx, int iNum, char *iBuf);
int sendProcessInfo(int indx, int pid, int type);
char *strcmprs(char *str);
int deNewLineIt(char *buf);
double ctod(char *str);
long ctoma(char *str);
char *strlwr(char *str);
char *strupr(char *str);
char *get_time(void);
/* deg_trig.c */
double deg_sin(double angle);
double deg_cos(double angle);
double deg_tan(double angle);
double deg_asin(double value);
double deg_acos(double value);
double deg_atan(double value);
double deg_atan2(double top, double bot);
/* socklib.c */
struct BOUND *sock_bind(char *mbname);
struct SOCK *sock_connect(char *mbname);
int read_sock(struct SOCK *s, char *msg, int *l);
int sock_print_fds(void);
int findSocketsButThese(char *name, struct SOCK *rss[], int *rssn);
int sock_close(char *name);
struct SOCK *last_msg(void);
char *sock_name(struct SOCK *handle);
int sock_fd(struct SOCK *handle);
int sock_bufct(int bufct);
struct SOCK *sock_find(char *mbname);
int sock_intr(int flag);
int sock_sel(char *message, int *l, int *p, int n, int tim, int rd_in);
int sock_ssel(struct SOCK *ss[], int ns, char *message, int *l, int *p, int n, int tim, int rd_in);
int sock_fastsel(char *message, int *l, int *p, int n, struct timeval *tim, int rd_in);
int sock_only(struct SOCK *handle, char *message, int tim);
int sock_only_f(struct SOCK *handle, char *message, double ftime);
int sock_poll(struct SOCK *handle, char *message, int *plen);
int accept_sock(void);
int readtm(int fd, char *buf, int len);
/* mbdef.c */
struct DEST *find_dest(char *name);
struct DEST *get_dest(int i);
struct DEST *find_port(int p);
int bind_any(int fd);
/* socknoxvlib.c */
int sock_send(struct SOCK *s, char *message);
int sock_write(struct SOCK *s, char *message, int l);
/* timeLib.c */
int sexagesimal(double dang, char *str, int prec);
int delay(double dc);
int getLongYear(int offset);
int getWDay(int offset);
int getDoy(int offset);
int getYear(int offset);
int getMonth(int offset);
int getDom(int offset);
char *getDate(int offset);
char *getLogDate(int offset);
char *getIncFileName(int offset);
char *getSlashDate(int offset);
char *getIncDate(int offset);
char *getSDate(int offset);
char *getRDate(int offset);
double getNraoDate(void);
double getTimeD(int offset);
int getMinute(int offset);
int getHour(int offset);
char *getTimeS(int offset);
char *getcTime(int offset);
double getmJulDate(int offset);
int breakmJulian(double jd, int *yr, int *mo, int *day, int *hr, int *min, int *sec);
double makeJulDate(int im, int id, int iy, double ut);
double getmjd(int iy, int im, int id, int hr, int min);
/* loglib.c */
int log_msg(const char *fmt, ...);
int log_perror(char *msg);
int log_eventdEnable(int enable);
int log_open(char *name, int how);
int newlog2datefile(void);
int log_newlog(void);
char *getNewCtime(int offset);
/* sem.c */
int semInit(void);
int semTake(int sem, int wait);
int semClear(int sem, int verbose);
int semGive(int sem);
/* swapdata.c */
double swapd(double *d);
long swapl(long *l);
int swapi(int *l);
unsigned short swaps(unsigned short *s);
int in_swapus(unsigned short *s);
int in_swapf(float *f);
int in_swaps(short *s);
int in_swapd(double *d);
int in_swapl(long *ll);
int in_swapi(int *ii);
/* findversion.c */
int findDataVersion(char *ini, char *dir);
/* wr_sdd.c */
int write_sdd(char *name, struct HEADER *h, char *d);
int new_sdd(char *name);
int verify_sdd(char *name, float *pscan);
int pdopen(char *name);
int scan_cmp(double f, double d);
int blocks(int b);
int obsmode(char *o);
int pos_code(char *s);
int padd(char *s, int n);
double index_freqres(struct HEADER *h);
double index_restfreq(struct HEADER *h);
int write_boot(int fd, struct BOOTSTRAP *b, int len);
int write_dir(int fd, struct DIRECTORY *d, int len);
struct BOOTSTRAP *last_bootb(void);
