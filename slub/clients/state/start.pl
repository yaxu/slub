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
my @samples;
my $tpb = ($ENV{TPB}   or 2);
my $vol = ($ENV{VOL}   or 40);
my $decay = ($ENV{DECAY} or 70);
my $mod = ($ENV{MOD} or 1);
my $rand = ($ENV{RAND} or 0.9);
my $oct = 0;

my $place = 0;

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
    @samples = $client->init_samples($ARGV[0] || 'house');

    init_idle();
    #curses_init();

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

    mode();
}

{
    my $mode;
    my $state;
    my $mystate;

    sub mode {
	&$mode();
    }

    sub init_idle {
	$mode = \&idle;
	$mystate = $state->{idle} ||= {};
	$mystate->{period}   = (3 + int(rand(2))) * 2;
	$mystate->{ticks}    = 0;
	$mystate->{maxticks} = 12 + (int(rand(3)) * 4);
	$mystate->{sample}   = $samples[rand @samples];
    }

    sub idle {
	#idle
	#  pick period x
	#  pick sound x
	#  play sound x every period x
	
	if (not $mystate->{ticks} % $mystate->{period}) {
	    play($mystate->{sample});
	}
	
	if (++$mystate->{ticks} >= $mystate->{maxticks}) {
	    init_fill();
	}
    }

    sub init_fill {
	$mode = \&fill;
	$mystate = $state->{fill} ||= {};
	$mystate->{sample}   = $samples[rand @samples];
	$mystate->{hits}     = 1 + int(rand(6));
	$mystate->{ticks}    = 0;
	$mystate->{maxticks} = 12 + (int(rand(4)) * 4);

	my $period = $state->{idle}->{period};
	my $seq = [map {[]} (1 .. $period)];

	push(@{$seq->[0]}, $state->{idle}->{sample});

	my $hits = $mystate->{hits} + 1;
	my $mod = $period % $hits;
	$hits -= $mod;
	my $myperiod = $period / $hits;
	foreach my $point (1 .. ($hits - 1)) {
	    push(@{$seq->[$myperiod * $point]}, $mystate->{sample});
	}

	foreach my $point (0 .. ($mod - 1)) {
	    push(@{$seq->[int(($period / $mod) * $point)]},
		 $mystate->{sample}
		);
	}
	
	print Dumper $seq;
	$mystate->{seq} = $seq;
	$mystate->{currseq} = [@{$seq}];
    }

    sub fill {
	#fill
	#  pick sound y
	#  pick number of times to play y during every period x
	#  play sound x every period x
	#  before period x tends, play y the alloted number of times, in a
	#  fairly equidistant manner
	
	if (not @{$mystate->{currseq}}) {
	    $mystate->{currseq} = [@{$mystate->{seq}}];
	}
	
	foreach my $sample (@{shift @{$mystate->{currseq}}}) {
	    play($sample);
	}
	
	if (++$mystate->{ticks} >= $mystate->{maxticks}) {
	    init_recap();
	}
    }

    sub init_recap {
	$mode = \&recap;
	$mystate = $state->{recap} ||= {};
	$mystate->{maxticks} = 12 + (int(rand(6)) * 4);
	$mystate->{ticks} = 0;
	my $buf = $mystate->{buf} ||= [];
	
	if (not @$buf) {
	    print "empty\n";
	    init_idle();
	}
	else {
	    print "recap !\n";
	    $mystate->{seq} = $mystate->{buf}->[rand @{$mystate->{buf}}];
	    $mystate->{currseq} = [@{$mystate->{seq}}];
	}
	
	push(@$buf, $state->{fill}->{seq});
	if (@$buf > 3) {
	    warn("shifting\n");
	    shift(@$buf);
	}

    }

    sub recap {
	warn("recap.\n");
	#recap
	#  pick parameters for a fill we have already played
	#  play it again
	#
	
	if (not @{$mystate->{currseq}}) {
	    $mystate->{currseq} = [@{$mystate->{seq}}];
	}
	
	foreach my $sample (@{shift @{$mystate->{currseq}}}) {
	    play($sample);
	}
	
	if (++$mystate->{ticks} >= $mystate->{maxticks}) {
	    if (@{$mystate->{buf}} > 2) {
		if (rand(1) < 0.90) {
		    init_recap();
		}
		else {
		    init_idle();
		}
	    }
	    else {
		init_idle();
	    }
	}
    }
}

sub play {
    my $sample = shift;
    $client->play_sample($sample, ($ENV{PITCH} || 60), 20000,
			 ($ENV{VOL} || 20)
			);
}
