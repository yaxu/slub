#!/usr/bin/perl -w

use strict;

use Client;

my $c = Client->new(port     => 6010,
		    #ctrls    => \@ctrls,
		    on_ctrl  => \&on_ctrl,
		    on_clock => \&on_clock,
		    timer    => 'ext',
		    tpb      => 2,
		   );

my $offset = 60;
print "0: $ARGV[0]\n";
if ($ARGV[0] =~ /^\d+$/) {
  warn("aha\n");
  $offset = shift @ARGV;
}

my @samples = map {$c->init_samples($_)} @ARGV;

init_foo();

$c->event_loop;

my @foo;

sub init_foo {
    foreach my $c1 (0 .. 0) {
	my $fooette = [];
	foreach my $c2 (0 .. int(rand(4))) {
	    push(@$fooette, $samples[rand @samples]);
	}
	push @foo, $fooette;
    }
}
    
sub on_ctrl {
}

my $tick;
sub on_clock {
    morph_foo();
    play_foo()
}

sub morph_foo {
    foreach my $fooette (@foo) {
	my $p = rand();
	if ($p > 0.9) {
	    if ($p > 0.95) {
		if (@$fooette > 3) {
		    shift @$fooette;
		}
	    }
	    else {
		if (@$fooette < 9) {
		    unshift @$fooette, $samples[rand @samples];
		}	    
	    }
	}
    }
}

sub play_foo {
    my $c1;
    foreach my $fooette (@foo) {
	my $sample = shift(@$fooette);
	$c->play_sample($sample, $offset, 44000, 8);
	push @$fooette, $sample;
    }
}

##

#sub curses_init {
#  initscr;
#  cbreak;
#  noecho;
#  keypad 1;
#  nonl;
#  nodelay 1;
#}

##

#sub curses_deinit {
#  endwin;
#}

##


1;
