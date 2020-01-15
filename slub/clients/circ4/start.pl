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

my %types = (1 => {
		   feeptype  => 'Etab4',
		   sampleset => 'bassdm', 
		   sampleno  => 4,
		   pitch     => 50,
		   pan       => 0.48,
		   cutoff    => 10000,
		   res       => 0.5,
		  },
	     2 => {
		   feeptype  => 'Etab4',
		   sampleset => 'bassdm', 
		   sampleno  => 3,
		   pitch     => 45
		  },
	     3 => {
		   feeptype  => 'Etab3',
		   sampleset => 'mouth',
		   sampleno  => 0
		  },
	     4 => {
		   feeptype  => 'Etab2',
		   sampleset => 'bubble', 
		   sampleno  => 0,
		   pan       => 0.68,
		  },
	     5 => {feeptype  => 'Etab1',
		   sampleset => 'sid', 
		   sampleno  => 3,
		   pitch     => 30,
		   pan       => 0.48,
		  },
	     6 => {
		   feeptype  => 'Etab',
		   sampleset => 'sid', 
		   sampleno  => 2,
		   pitch     => 30,
		   pan       => 0.45,
		  },
	     7 => {feeptype  => 'Etab',
		   sampleset => 'lighter',
		   sampleno  => 0,
		   pan       => 0.40,
		  },
	     8 => {feeptype  => 'Etab',
		   sampleset => 'lighter',
		   sampleno  => 1,
		   pan       => 0.60,
		  },
	     9 => {feeptype  => 'Etab',
		   sampleset => 'lighter',
		   sampleno  => 2,
		   pan       => 0.55,
		  },
	    );


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
			  y => 4
			 }
			);

{
    my %samples;
    sub samples {
	my ($set) = @_;
	if(not exists $samples{$set}) {
	    $samples{$set} = [$c->init_samples($set)];
	}
	$samples{$set};
    }	
}



sub intro {
    my $p = shift;
    my $pkg = "Circ::Feep::$p->{feeptype}";
    my $samples = samples($p->{sampleset});
    my $sample = $samples->[exists $p->{sampleno}
			    ? $p->{sampleno}
			    : rand @$samples
			   ];
    my @sample_p = ($sample);

    my %d = (pitch => 90,
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

    for (1 .. ($p->{number} or 1)) {
	$env->introduce($pkg->new({env       => $env,
				   trigger   => 
				   sub { my @foo = @sample_p;
				       $c->play_sample(@foo), 
				     },
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
	    intro($types{$feep});
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
    my ($ctrl, @data) = @_;
    if (exists $types{$data[0]}) {
	$q->enqueue($data[0]);
	print "queued $data[0]\n";
    }
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
