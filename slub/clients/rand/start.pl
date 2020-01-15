#!/usr/bin/perl

##

use strict;
use Client;
use Data::Dumper;

##

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
    $client->play_sample($samples[rand @samples], rand(40) + 20, 20000, 10);
}

##

1;

