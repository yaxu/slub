#!/usr/bin/perl

##

use strict;
use Client;
use Data::Dumper;

##

my @samples;
my $client;

my $partsize = 16;

init();

sub init {
    $client = Client->new(port      => 6011,
			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => ($ENV{TPB} || 4)
			 );
    @samples = map {$client->init_samples($_)} @ARGV;
    $client->event_loop;
}

##

sub on_clock {

    if (my $next = nextnote()) {
	$client->play_sample($next->[0], ($ENV{PITCH} || 80), 20000, (($ENV{VOL} || 60) + $next->[1]));
    }
}

##

{
    my $mode;
    my %parts;
    my $count;
    my $firstnote;
    my $lastnote;
    sub nextnote {
	my $result;
	$mode ||= 'exposition';
	$lastnote ||= [newnote(), -40];
	$count ||= 0;
	my $first = (not (($count) % $partsize));
	my $final = (not (($count+1) % $partsize));
	my $odd = $count % 2;
	if ($mode eq 'exposition') {
	    if (not $odd) {
		if ($first) {
		    $result = $firstnote = $lastnote;
		    $parts{exposition} = [];
		}
		else {
		    if (rand > 0.8) {
			$result = [newnote(), -40];
		    }
		    else {
			$result = $firstnote;
		    }
		}
		

		push @{$parts{exposition}}, $result;
	    }

	    if ($final) {
		$mode = 'development';
	    }
	}
	elsif ($mode eq 'development') {
	    if (not $odd) {
		my $part = $parts{development};
		my $pos = ($count % $partsize) / 2;
		
		if ((not $part->[$pos]) or (rand > 0.90)) {
		    $result = [newnote(), 0];
		    $part->[$pos] = $result;
		}
		else {
		    $result = $part->[$pos];
		}
	    }
	    if ($final) {
		$mode = 'recapitulation';
	    }
	}
	elsif ($mode eq 'recapitulation') {
	    $result = shift @{$parts{$odd ? 'development' : 'exposition'}};
	    # hack
	    push @{$parts{$odd ? 'development' : 'exposition'}}, $result;
	    if ($final) {
		$mode = 'exposition';
	    }
	}
	else {
	    die "can't happen";
	}
	$count++;
	$lastnote = $result;
	return $result;
    }
}

##

sub newnote {
    return($samples[rand @samples]);
}

##

1;

