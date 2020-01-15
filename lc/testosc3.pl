#!/usr/bin/perl -w

use strict;
use Time::HiRes;
use lib '/yaxu/lc/lib';

use Audio::OSC::Client;
use Time::HiRes qw/time/;

my $osc =
  Audio::OSC::Client->new(Host => 'localhost',
                          Port => 57121
                         );

my $time = Time::HiRes::gettimeofday;

printf "The time is %.6f\n", $time; 

$osc->send(['#bundle', $time, ['/play', 's', 'hello']]
          );

