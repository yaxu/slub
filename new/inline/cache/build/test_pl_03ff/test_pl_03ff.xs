#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "INLINE.h"

int test(char *str)
{
    char *sample;
    sample = "gabba/0.wav";
    if (lo_send(t, "/trigger", "sf", sample, 0.5) == -1) {
      printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
    }
}

MODULE = test_pl_03ff	PACKAGE = main	

PROTOTYPES: DISABLE


int
test (str)
	char *	str

