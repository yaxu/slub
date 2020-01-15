#!/usr/bin/perl

package circpl;

use strict;

use Thread;
use Thread::Queue;
use Client;
use Circ::Env;
use Circ::Feep::Etab;
use Circ::Feep::Etab1;
use Circ::Feep::Etab2;
use Circ::Feep::Etab3;
use Circ::Feep::Etab4;

my $q = Thread::Queue->new;

my @ctrls = qw/ feep_new /;


my %values;

sub value {
    $values{$_[0]}
}

my $c = Client->new(port     => 6010,
		    ctrls    => \@ctrls,
		    on_ctrl  => \&on_ctrl,
		    on_clock => \&on_clock,
		    timer    => 'ext',
		    tpb      => 16,
		   );

my $env = Circ::Env->new({x => 16,
			  y => 8 
			 }
			);

{
    my $set;
    sub samples {
	if(not defined $set) {
	    $set = [map {$c->init_samples($_)} @ARGV];
	}
	$set;
    }	
}



sub intro {
    my $p = shift;
    my $pkg = "Circ::Feep::Base";
    my $samples = samples();
    my $soundno;

    if (exists $p->{type}) {
	$soundno = $p->{type} % scalar(@$samples);
    }
    else {
	$soundno = rand @$samples;
    }
    my $sample = $samples->[$soundno];
    my @sample_p = ($sample);

    my %d = (pitch => 60,
	     cutoff => 44000,
	     vol => 100,
	     pan => 0.5,
	     res => 0,
	    );

    foreach my $sp (qw(pitch cutoff vol pan res)) {
	push @sample_p, (exists $p->{$sp}
			 ? $p->{$sp}
			 : $d{$sp}
			);
    }
    
    my @i_p;
    foreach my $foo (qw/friendly dispersal racism strength type/) {
	if(exists $p->{$foo}) {
	    push @i_p, $foo => $p->{$foo};
	}
    }

    for (1 .. ($p->{number} or 1)) {
	$env->introduce($pkg->new({env       => $env,
				   trigger   => 
				   sub { my @foo = @sample_p;
				       $c->play_sample(@foo), 
				     },
				   @i_p
				  }
				 )
		       );
    }
}

my $t = Thread->new(\&thread_loop);


$c->event_loop;

##

sub thread_loop {
    while(my $feep = $q->dequeue) {
	warn("aha: $feep\n");
	eval {
	    intro($feep);
	};
	if ($@) {
	    warn($@);
	}
	warn("done.\n");
    }
    print "bah!\n";
}

##

sub on_ctrl {
    my ($ctrl, %data) = @_;
    $q->enqueue(\%data);
}

##

{
    my $cell_id;
    
    sub on_clock {
	$cell_id ||= 0;

	my $feeps = $env->env->[$cell_id];
	
	foreach my $feep (@$feeps) {
	    $feep->trigger->();
	}
	
	$cell_id = ++$cell_id % $env->size1d;
    }
}

##

1;
