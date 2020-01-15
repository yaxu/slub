#!/usr/bin/perl -w

package main;

##

use strict;
use Client2;
use Data::Dumper;
use Curses;

##

my @chrs = qw(o x ~ = - + * % @ \ / ! ?);
my @samples;
my @distances = (2, 4, 6, 8);
my $client;
my $seq_place = 0;
my @seq;

my $place;

##

init();

##

sub init {
    curses_init();
    
    $client = Client2->new(port      => 6020,
			   on_clock  => \&on_clock,
			   timer     => 'ext',
			   tpb       => ($ENV{TPB} || 8),
			  );
    @samples = map {$client->init_samples($_)} @ARGV;

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

my $space;
my $wrap;

sub on_clock {
    if ($space) {
	$space--;
    }
    else {
	if ($wrap) {
	    $seq_place = 0;
	    $wrap = 0;
	}
	if ($seq_place >= @seq) {
	    $wrap = 1;
	    push(@seq, [$distances[rand @distances], rand(@samples)]);
	}
	
	my $sample;
	($space, $sample) = @{$seq[$seq_place]};
	
	$client->play_sample($samples[$sample]);
	$seq_place++;
    }
    
    while((my $ch = getch()) ne ERR) {
	my $sub = "key__$ch";
	if (main->can($sub)) {
	    main->$sub();
	}
    }
}

##

sub key__n {
    pop @seq
      if @seq;
}

##

sub key__N {
    shift @seq
      if @seq;
}

##

1;
