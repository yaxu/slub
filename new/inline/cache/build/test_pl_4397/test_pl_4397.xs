#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "INLINE.h"

#include "lo/lo.h"

int send_osc(char *sample)
{
    lo_address t = lo_address_new(NULL, "7770");
    if (speed == 0) {
        speed = 1;
    }
    if (lo_send(t, "/trigger", "sf", sample, 1) == -1) {
      printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
    }
}

MODULE = test_pl_4397	PACKAGE = main	

PROTOTYPES: DISABLE


int
send_osc (sample)
	char *	sample

