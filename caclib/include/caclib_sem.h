#ifndef SEM_H
#define SEM_H

/* Sem lib */
#define SEM_SERIALOUT_KEY      0xcafe18
#define SEM_SERVO_KEY          0xcafe19
#define SEM_RECORD_KEY         0xcafe20
#define SEM_WAVETEST_KEY       0xcafe21

#define MAX_SEMS_KEYS                 4

extern int sems[MAX_SEMS_KEYS];

#define SEM_SERIALOUT_INDEX           0
#define SEM_SERVO_INDEX               1
#define SEM_RECORD_INDEX              2
#define SEM_WAVETEST_INDEX            3

#define WAIT_FOREVER       (-1)
#define NO_WAIT            ( 0)

#ifndef ERROR
#define ERROR              (-1)
#endif

#define TIMEOUT            ERROR

#endif
