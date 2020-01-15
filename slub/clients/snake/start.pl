#!/usr/bin/perl

##

use strict;
use Client;
use Data::Dumper;

##

my $sdirection = 1;
my $smax = 6;
my $size = 6;
my $direction = 1;
my $position = 0;
my @snake;
my $tick = 0;
my @samples;
my $client;
my $tpb = ($ENV{TPB} or 4);
my $vol = ($ENV{VOL} or 40);

##

init();

##

sub init {
    $client = Client->new(port      => 6011,
			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => $tpb
			 );
    @samples = map {$client->init_samples($_)} @ARGV;

    $client->event_loop;
}

##
my $foo;
sub on_clock {
    if (not ($tick++ % 32)) {
	push @snake, [$samples[rand @samples], (0.5 - 0.15 + rand(0.3))];
	if (@snake > $size) {
	    shift @snake;
	}
    }
    $position += $direction;
    if ($position < 0 or $position >= @snake) {
	$direction = ($direction > 0
		      ? -1
		      : 1
		     );
	$position += $direction;
	$position += $direction;
    }
    
    $client->play_sample($snake[$position]->[0], ($ENV{PITCH} || 60), 44000, $vol, $snake[$position]->[1]);
}

##

1;

