#!/usr/bin/perl

##

use strict;
use Client;
use Data::Dumper;

##

#my @rests = (5, 5, 5, 9, 13, 13);
my @rests = (3);
my $tree;

##

my @samples;
my $client;

init();

sub init {
    $client = Client->new(port      => 6011,
			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => ($ENV{TPB} || 4)
			 );
    @samples = map {$client->init_samples($_)} @ARGV;

    $tree = make_tree(5);

    $client->event_loop;
}

##
my $foo;
{
    my $genseq;
    my $wait;
    my $pan;
    sub on_clock {
	if ($wait > 0) {
	    $wait--;
	}
	else {
	    print join(':', @_) . "\n";
	    if (not @{$genseq || []}) {
		$genseq = genseq();
	    }
	    my $this = shift @$genseq;
	    while (not ref $this) {
		$pan = $this;
		$this = shift @$genseq;
	    }

	    my $pitch = ($ENV{OFFSET} || 60);
	
	    if ($ENV{STRAY}) {
		$pitch += (10 - (20 * $pan * 2));
	    }

	    $client->play_sample($this->[1], $pitch, 
				 44000, ($ENV{VOL} || 20), $pan
				);
	    $wait = $this->[0];
	}
    }
}

##

sub genseq {
    my $branch = $tree;
    my @seq;
    my $range = [0.30, 0.70];
    while ($branch) {
	my $seq = $branch->{sequence};
	if ($branch->{next}) {
	    my $next = $branch->{next};
	    my $rand = int(rand(@$next));
	    print ("-> $rand ");
	    my $rangelength = (($range->[1] - $range->[0]) / @$next);

	    $range->[0] += ($rangelength * $rand);
	    $range->[1] = $range->[0] + $rangelength;
	    $branch = $next->[$rand];
	}
	else {
	    undef $branch;
	}
	push(@seq, $range->[0] + (($range->[1] - $range->[0]) / 2));
	push(@seq, @$seq);
    }
    print ("\n");
    return \@seq;
}

##

{
    my $sequences;
    sub make_tree {
	my $gen = shift;
	$sequences ||= [map {sequence()} (0 .. 3)];

	$gen = $gen - (1 + int(rand(2)));

	my $next = (($gen > 0)
		    ? [map {make_tree($gen)} (0 .. 2)]
		    : undef
		   );
	return({
		sequence => $sequences->[rand(@$sequences)],
		next     => $next
	       }
	      );
    }
}

sub sequence {    
    [map { point() }
      (0, 1)
    ];
}

##

sub point {
    return [$rests[rand @rests], $samples[rand @samples]];
}

##

1;

