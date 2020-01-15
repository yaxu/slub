#!/usr/bin/perl -w

package main;

##

use strict;
use Client2;
use Data::Dumper;
use Curses;

##

my $tick = 0;
my @samples;
my $sample;
my $client;
my $tpb = ($ENV{TPB} or 2);
my $vol = ($ENV{VOL} or 40);

my $reverse = 0;

##

init();

##

sub init {
    curses_init();
    
    $client = Client2->new(port      => 6010,
			   on_clock  => \&on_clock,
			   timer     => 'ext',
			   tpb       => $tpb
			  );
    @samples = $client->init_samples($ARGV[0] || 'glitch');
    $sample = $samples[$ARGV[1] || 0];
    $client->event_loop;
}

##

sub curses_init {
    initscr;
    cbreak;
    noecho;
    keypad 1;
    nonl;
    nodelay 1;
}

##

sub on_clock {
    
    if ($tick % 16 < 12) {
	$client->play_sample($sample, (not ($tick % 3) ? 60 : -60), 20000,
			     ($ENV{VOL} || 20)
			    );
    }
    
    while((my $ch = getch()) ne ERR) {
	my $sub = "key__$ch";
	if (main->can($sub)) {
	    main->$sub();
	}
    }
    
    ++$tick;
}

##

1;

