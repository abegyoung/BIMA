#ifndef SHARED_H
#define SHARED_H

typedef struct {
    int fake; //0=real, 1=cal 2=ref 3=sig
} SHARED_DATA;

#define SHM_NAME "shared_mem_file"

#define FAKE_CAL "fake_spectra/fake_cal.txt"
#define FAKE_REF "fake_spectra/fake_ref.txt"
#define FAKE_SIGS (const char*[]){"fake_spectra/fake_sig0.txt", \
                                  "fake_spectra/fake_sig1.txt", \
                                  "fake_spectra/fake_sig2.txt", \
                                  "fake_spectra/fake_sig3.txt", \
                                  "fake_spectra/fake_sig4.txt"}

#endif
