#!/usr/bin/perl -w

package main;

##

use strict;
use Client;
use Data::Dumper;
use Curses;

##

my $name = ($ENV{NAME} or "line$$");

my $synth_ctrl = "synth_$name";

my %ctrls = ($synth_ctrl => ($ENV{SYNTH} or 'slub'));

my $client;
my $tick = 0;
my $tpb = ($ENV{TPB}   or 2);
my $vol = ($ENV{VOL}   or 40);
my $decay = ($ENV{DECAY} or 70);
my $mod = ($ENV{MOD} or 1);
my $rand = ($ENV{RAND} or 0.9);
my $oct = 0;

my $place = 0;

my @notes = @ARGV;

die "no notes" if not @notes;

##

init();

##

sub init {
    $client = Client->new(port      => 6010,
			  on_clock  => \&on_clock,
			  on_ctrl   => \&on_ctrl,
			  ctrls     => [$synth_ctrl],
			  timer     => 'ext',
			  tpb       => $tpb
			 );
    curses_init();

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

sub on_ctrl {
    my ($ctrl_name, $data) = @_;
    $ctrls{$ctrl_name} = $data;
}

##

sub on_clock {

    return if $tick++ % $mod;

    my $note = $notes[$place];
    my $msg = "play $synth_ctrl pitch $note decay $decay vol $vol";
    $client->send('MSG', $msg);

    if ($place == $#notes or rand() <= $rand) {
	$place = abs($place - 1);
    }
    else {
	$place = $place + 1;
    }

    if (exists $ctrls{$synth_ctrl}) {
	$client->send('MSG', "make $synth_ctrl from $ctrls{$synth_ctrl}");
	    delete $ctrls{$synth_ctrl};
    }
}

##

sub key__p {
    ++$oct;
}

##

sub key__o {
    --$oct if $oct > 0;
}

