#!/usr/bin/perl

##

use strict;
use Client;
use Data::Dumper;

##

use vars qw| $client @samples $inc $max_inc $gap $no_fingers |;

$no_fingers = 4;
$max_inc    = 120;

##

init();

##

sub init {
    $inc = 0;
    $gap = 0;
    $client = Client->new(port      => 6010,
			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => 1
			 );
    @samples = map {$client->init_samples($_)} @ARGV;
    if (@samples < 2) {
	die "more samples required\n";
    }
    $client->event_loop;
}

##

sub on_clock {
  fingers();
  slap();
}

##
#
#  0    10   20   30   40   50   60   70   80   90   100  110  120  130  140
#  |----|----|----|----|----|----|----|----|----|----|----|----|----|----|
#  
##

{
    my $finger;
    my $fingers;
    sub fingers {
	$fingers ||= init_fingers();
	$finger  ||= 0;
	
	my $thresh;
	if ($finger == 0) {
	    $thresh = 17;
	}
	else {
	    $thresh = 10;
	}
	
	#sin(1.57 / ($inc / $max_inc));
	if ($thresh < rand(20)) {
	    hit_finger($fingers->[($finger++) % scalar(@$fingers)]);
	}
	else {
	    ++$gap;
	}
    }
}

##

sub init_fingers {
    [map {
	[60  + (2 - rand(4)) ,#pitch 
	 44000               ,#cutoff
	 50  -  rand(20)     ,#vol
	 0.5 + (rand(0.1))   ,#pan
	 0.1 + (rand(0.3))   ,#res
	]
     } (1 .. $no_fingers)
    ];
}

##

sub slap {
    if ($gap > (2 + rand(5))) {
	hit_slap()
    }
}

##

sub hit_finger {
    my $finger = shift;
    if ($inc < $max_inc) {
	$inc += int(rand(3));
	if ($inc > $max_inc) {
	    $inc = $max_inc;
	}
    }
    $gap = 0;
    $client->play_sample($samples[0], @$finger);
}

##

sub hit_slap {
    my $vol = $inc;
    if ($vol < ($max_inc / 2)) {
	$vol = ($max_inc / 2);
    }
    $client->play_sample($samples[1], 60, 44000, $vol);
    if ($inc >= $max_inc) {
	$inc = 0;
    }
}

##

1;

