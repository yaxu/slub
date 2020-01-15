#!/usr/bin/perl

##

use strict;
use Client;
use Data::Dumper;

##

use vars qw| $client @samples $no_fingers $fingers $inc $max_inc |;

$no_fingers = 4;
$max_inc = 64;

##

init();

##

sub init {
    $inc = 0;
    $client = Client->new(port      => 6010,
			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => 8
			 );
    @samples = map {$client->init_samples($_)} @ARGV;
    if (@samples < 2) {
	die "more samples required\n";
    }

    $fingers = fingers();
    $client->event_loop;
}

##

sub fingers {
    [map {
	[$samples[$_ % @samples],
	 60                  ,#pitch
	 44000               ,#cutoff
	 50                  ,#vol
	 0.3 + (rand(0.4))   ,#pan
	]
    } (1 .. $no_fingers)
    ];
}

##

sub on_clock {
     my $foo = sin(1.57 * (($inc++ % $max_inc) / ($max_inc / 2)));
     $foo *= 10;
     $foo -= int($foo);
     print "$foo\n";
     $client->play_sample(@{$fingers->[(@$fingers) * $foo]});
}

##

1;

