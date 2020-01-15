#!/usr/bin/perl -w

use strict;

use Inline (C => 'DATA',
            LIBS => '-llo',
            DIRECTORY => '/music/new/inline/cache'
           );



send_osc("gabba/0.wav");
send_osc("gabba/1.wav");
send_osc("gabba/2.wav");
send_osc("gabba/3.wav");

__DATA__
__C__

#include "lo/lo.h"

int send_osc(char *sample)
{
    lo_address t = lo_address_new(NULL, "7770");
    if (lo_send(t, "/trigger", "sf", sample, 1.0) == -1) {
      printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
    }
}
