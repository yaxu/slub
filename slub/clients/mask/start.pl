#!/usr/bin/perl -w

package main;

##

use strict;
use Client2;
use Data::Dumper;
use Curses;

##

my $loopsize = 32;
my $max_inc = 32;
my $tick = 0;
my @samples;
my $client;
my $tpb = ($ENV{TPB} or 2);
my $vol = ($ENV{VOL} or 40);

my @masks;

my %algos = (prm => {},
	     rnd => {}
	    );

my @types = qw/pitch volume delay cutoff res/;

##

sub add_mask {
    my($algo, $types) = @_;
    
    my $func = "algo__$algo";
    if(not __PACKAGE__->can($func)) {
	warn("no func $func\n");
	return;
    }
    my $seq = __PACKAGE__->$func();

    my $mask = [map {my $value = $_;
		     {map { $_ => $value } @$types}
		    }
		@$seq
	       ];
    push @masks, $mask;
}

##

sub step {
    my $point = $tick % $loopsize;

    my %values = map {$_ => 1} @types;

    foreach my $mask (@masks) {
	while (my ($key, $value) = each %$mask) {
	    $values{$key} *= $value;
	}
    }
}

##

{
    my $perms;
    my @bar;
    my $foo;
    my $seq;
    sub algo__prm {
	$seq ||= [0, 0.25, 0.5, 0.75, 1];
	$perms ||= perms($seq, 16);
	$foo ||= 0;

	if (not @bar) {
	    addstr(($tick % 23) + 1, int($foo * 80), " ");
	    $foo = 0.5 + (sin(1.57 * (($tick % $max_inc) / ($max_inc / 4))) 
			  / 2
			 );
	    #$tick++;
	    addstr(($tick % 23) + 1, int($foo * 80), "x");
	    my $perm_now = $perms->[$foo * (scalar(@$perms) - 1)];
	
	    #refresh();
	    foreach my $c (0 .. (scalar(@$seq) - 1)) {
		push(@bar, (($seq->[$c]) x $perm_now->[$c]));
	    }
	}
    }
}

##

sub algo__rnd {
    int(rand(16));
}

##

init();

##

sub init {
    #curses_init();

    $client = Client2->new(port      => 6010,
			   on_clock  => \&on_clock,
			   on_ctrl   => \&on_ctrl,
			   ctrls     => ['reverse'],
			   timer     => 'ext',
			   tpb       => $tpb
			  );
    @samples = map {$client->init_samples($_)} @ARGV;
    warn("erp!\n");
    add_mask('prm', ['volume']);
    print Dumper \@masks;
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

    while((my $ch = getch()) ne ERR) {
	my $sub = "key__$ch";
	if (main->can($sub)) {
	    main->$sub();
	}
    }

    $tick++;
}

##

sub on_ctrl {
    my($ctrl, $value) = shift;
    my $func = "ctrl__$ctrl";
    if (main->can($func)) {
	main->ctrl($value);
    }
}

##

sub perms {
    my ($numbers, $bar_length) = @_;
    my $perms = gen($numbers, $bar_length);
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


