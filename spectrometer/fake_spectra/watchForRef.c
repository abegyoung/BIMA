/////////////////////////////////////////////////////////////////////////////
//    This little program allows the IBOB_CONTROL program to send
//    fake spectra to SPEC_ARO_CATCHER and be displayed by DATASERV
//
//    Run it anywhere on corona while VANED and IBOB_CONTROL are running to send
//    fake data back to spectral line system.
//
//    Essentially, watches SPEC_GLOBAL and CACTUS_GLOBAL shared memory files.
//    Cases:
//    1) When OFFSET is PSREF, and VANEIN is sent to VANED and spec control
//       does a setMode(OBS_MODE_HOT) saved to SGM, then send fake_cal to VANED.
//    2) When OFFSET is PSREF, and VANEHOME is sent to VANED and spec control
//       does a setMode(OBS_MODE_SKY || OBS_MODE_REF) saved to SGM, then send fake_ref to VANED.
//    3) When OFFSET is ZERO, and VANE position is OUT and spec control
//       does a setMode(OBS_MODE_SIG) saved to SGM, then send fake_sig to VANED.
//
//    On the cabin end, since VANED now knows a few global memory states, he can save
//    these states to a shared memory file on inst.  IBOB_CONTROL can access those, and
//    hijack appropriately the ibob spectra udp stream and insert the fake cal,ref,sig
//
//    Originially started as VANEIN/VANEHOME signal, but that wasn't unique enough.  Added
//    OBS_MODE_??? from SGM.  I could get rid of VANE now. meh.
//
//    Works fine for spectral PS and spectral 5-point
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <math.h>

#include "cactus_global.h"
#include "caclib_proto.h"
#include "spec_map.h"

struct SOCK *vane;

int main(int argc, char *argv[])
{

    setCactusEnvironment();

    sock_bind("ANONYMOUS");
    vane = sock_connect("VANED");

//////////////////////////////////////////////////////////////
//                  CACTUS_GLOBAL                           //
//////////////////////////////////////////////////////////////
    // Pointing system global shared memory
    const char *filename1 = "/home/corona/cactus/CACTUS_GLOBAL";
    int fd1;
    size_t size1= sizeof(struct cactus_global);
    struct cactus_global *cpr;

    // Open existing shared memory file read-write
    fd1 = open(filename1, O_RDWR);
    if (fd1 < 0) {
        perror("open");
        return 1;
    }

    // Map into memory
    cpr = mmap(NULL, size1, PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0);
    if (cpr == MAP_FAILED) {
        perror("mmap");
        close(fd1);
        return 1;
    }

//////////////////////////////////////////////////////////////
//                   SPEC_GLOBAL                            //
//////////////////////////////////////////////////////////////
    // Pointing system global shared memory
    const char *filename2 = "/home/corona/cactus/SPEC_GLOBAL";
    int fd2;
    size_t size2 = sizeof(struct SPEC_GLOBAL);
    struct SPEC_GLOBAL *SGM;

    // Open existing shared memory file read-write
    fd2 = open(filename2, O_RDWR);
    if (fd2 < 0) {
        perror("open");
        return 1;
    }

    // Map into memory
    SGM = mmap(NULL, size2, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);
    if (SGM == MAP_FAILED) {
        perror("mmap");
        close(fd2);
        return 1;
    }

//////////////////////////////////////////////////////////////
//               WATCH FOR CAL, REF, SIG ROUTINE            //
//////////////////////////////////////////////////////////////
    float offset=0.0;
    int mode=0;
    int done=0;
    while(1){
        offset = cpr->ts.azoff; 
        mode = SGM->control.internal_mode;
        //printf("%d\n", mode);
        if(offset==1.0 && mode==1 && done!=1){
            sock_send(vane, "fake_cal");
            printf("VANE %s\n", (mode==1) ? "CAL" : "SKY");
            printf("sending fake_cal\n\n");
            done=1;
        }
        if(offset==1.0 && mode==2 && done!=2){
            sock_send(vane, "fake_ref");
            printf("VANE %s\n", (mode==1) ? "CAL" : "SKY");
            printf("sending fake_ref\n\n");
            done=2;
        }
        if(offset==1.0 && mode==6 && done!=2){
            sock_send(vane, "fake_ref");
            printf("VANE %s\n", (mode==1) ? "CAL" : "SKY");
            printf("sending fake_ref\n\n");
            done=2;
        }
        if(offset==0.0 && mode==5 && done!=3){
            sock_send(vane, "fake_sig");
            printf("VANE %s\n", (mode==1) ? "CAL" : "SKY");
            printf("sending fake_sig\n\n");
            done=3;
        }
        usleep(50000);
    }


    /* Cleanup */
    munmap(cpr, size1);
    close(fd1);

    /* Cleanup */
    munmap(SGM, size2);
    close(fd2);

    return 0;
}




