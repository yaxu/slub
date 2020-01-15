#!/usr/bin/perl -w

use strict;

use lib '/yaxu/lc/lib';

use Net::OSC;
use Time::HiRes qw/time/;

my $osc = Net::OSC->new;
$osc->server(hostname => '212.158.255.157', port => 8000);

$osc->command('hello');
$osc->data([['i', 1], 
	   ]
	  );
$osc->send_message();
