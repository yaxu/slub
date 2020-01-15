#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "INLINE.h"

int test(char *str)
{
    char *sample;
    lo_address t = lo_address_new(NULL, "7770");
    sample = "gabba/0.wav";

    if (lo_send(t, "/trigger", "sf", sample, 0.5) == -1) {
      printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
    }
}

MODULE = test_pl_615f	PACKAGE = main	

PROTOTYPES: DISABLE


int
test (str)
	char *	str

