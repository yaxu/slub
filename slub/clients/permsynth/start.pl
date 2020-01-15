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

my %ctrls = ($synth_ctrl => 'sbbb');

my $tick = 0;
my $foo  = 0;
my $client;
my $tpb = ($ENV{TPB} or 2);
my $vol = ($ENV{VOL} or 40);
my $max_inc = 32;

my $oct = 0;

my @seq = @{([qw/ 0 5 2 6 /],
             [qw/ 0 1 2 3 4 /],
             [qw/ 7 6 0 /],
	     [qw/ 16 19 21 24 26 28/],
	     [qw/ 0 1 2 3 4 5 6/],
            )[$ARGV[0] or 0]
          };

my @notes = qw( 1 3 7 9 11 13 15 17);
my $bar_length = 16;
my @bar;
my $seq_place = 0;
my $ticks_per_place = 3;
my $list;
my $perms;

my $reverse = 0;

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

    $perms = perms();
    $client->ctrl_send('synthreg', $synth_ctrl);
    $client->ctrl_send('synthreg', $synth_ctrl);
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
    if (not @bar) {
	addstr(($tick % 23) + 1, int($foo * 80), " ");
	$foo = 0.5 + (sin(1.57 * (($tick % $max_inc) / ($max_inc / 4))) / 2);
	$tick++;
	addstr(($tick % 23) + 1, int($foo * 80), "x");
	my $perm_now = $perms->[$foo * (scalar(@$perms) - 1)];

	refresh();
	foreach my $c (0 .. (scalar(@seq) - 1)) {
	    #push(@bar, (($seq[$c]) x $perm_now->[$c]));
	    push(@bar, $seq[$c]);
	    push(@bar, ((' ') x ($perm_now->[$c] - 1)));
	}
    }
    addstr(20,20, join(':', @bar) . '  ');
    refresh();
    my $note = shift @bar;
    if ($note ne ' ') {
	$note = $notes[$note % @notes] + 50;
	$note += (12 * $oct);
	
	#foreach my $chorus (0.1, -0.2, 0.2, -0.1) {
	
	foreach my $chorus (0) {
	    my $chorus_note = $chorus + $note;
	    my $msg = "play perm pitch $chorus_note decay 95 vol $vol";
	    $client->send('MSG', $msg);
	}

    }
    if (exists $ctrls{$synth_ctrl}) {
	$client->send('MSG', "make perm from $ctrls{$synth_ctrl}");
	    delete $ctrls{$synth_ctrl};
    }
	
    while((my $ch = getch()) ne ERR) {
	my $sub = "key__$ch";
	if (main->can($sub)) {
	    main->$sub();
	}
    }
}

##

sub perms {
    my $numbers = scalar(@seq);
    my $perms = gen($numbers, $bar_length);
    addstr(3,3, "numbers: $numbers");
    refresh();
    @$perms = map {pop @$_;$_} sort {$a->[$numbers] <=> $b->[$numbers]} @$perms;

    return $perms;
}

##

sub gen {
    my($numbers, $total, $gen, $array, $result) = @_;
    $array ||= [];
    $gen ||= 0;
    $result ||= [];

    my $subtotal = 0;
    if ($gen) {
	foreach my $c (0 .. $gen - 1) {
	    $subtotal += $array->[$c];
	}
    }

    my $top = ($total - $subtotal - ($numbers - $gen - 1));
    if ($gen == ($numbers - 1)) {
	$array->[$gen] = $top;
	my $deviance;
	foreach my $c (0 .. $numbers - 1) {
	    $deviance +=
	      abs(int($total / $numbers) - $array->[$c]);
	}
	$array->[$numbers] = $deviance;

	# push a copy of the array on to @$result
	push @$result, [@$array];
    }
    else {
	foreach my $c (1 .. $top) {
	    $array->[$gen] = $c;
	    gen($numbers, $total, $gen + 1, $array, $result);
	}
    }
    return $result;
}

##

sub key__p {
    ++$oct;
}

##

sub key__o {
    --$oct if $oct > 0;
}

