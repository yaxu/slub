#!/usr/bin/perl -w

use strict;

use lib '/yaxu/lc/lib';

use Audio::OSC::Client;
use Time::HiRes qw/time/;

my $osc = 
  Audio::OSC::Client->new(Host => 'localhost',
			  Port => 57121
			 );

$osc->send('/play', 
	  );
