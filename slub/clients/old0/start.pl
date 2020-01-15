#!/usr/bin/perl -w

use strict;

use old::Old;
use Client;

my $loopsize = 64;

##

my $c = Client->new(port     => 6010,
		    #ctrls    => \@ctrls,
		    on_ctrl  => \&on_ctrl,
		    on_clock => \&on_clock,
		    timer    => 'ext',
		    tpb      => 4,
		   );

my @samples = map {$c->init_samples($_)} @ARGV;

my @aged;
foreach my $sample (@samples) {
    push @aged, old::Old->new(sample   => $sample,
			      loopsize => $loopsize,
			      pitch    => 60
			     );
}


$c->event_loop;

sub on_ctrl {
}

my $tick;
sub on_clock {
    foreach my $old (@aged) {
	if (my $next = $old->next_beat) {
	    my @p;
	    foreach my $sp (qw(pitch cutoff vol pan res)) {
		push @p, $next->{$sp};
	    }
	    warn("playing: $old->{sample}, " . join(':', @p) . "\n");
	    $c->play_sample($old->sample, @p);
	}
    }
}
##

1;
