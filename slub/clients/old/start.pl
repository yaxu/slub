#!/usr/bin/perl -w

use strict;

use old::Old;
use Client;
use Curses;

my $loopsize = 62;

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

curses_init();

$c->event_loop;

sub on_ctrl {
}

my $tick;
sub on_clock {

    my $ch = getch();
    if ($ch ne ERR) {
	if ($ch =~ /[A-Z]/) {
	    foreach my $old (@aged) {
		$old->save(lc $ch);
	    }
	}
	elsif ($ch =~ /[a-z]/) {
	    foreach my $old (@aged) {
		$old->restore($ch);
	    }
	}
    }

    foreach my $old (@aged) {
	if (my $next = $old->next_beat) {
	    my @p;
	    foreach my $sp (qw(pitch cutoff vol pan res)) {
		push @p, $next->{$sp};
	    }
	    $c->play_sample($old->sample, @p);
	}
    }
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

sub curses_deinit {
  endwin;
}

##


1;
