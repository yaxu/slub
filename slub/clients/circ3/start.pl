#!/usr/bin/perl

package circpl;

use strict;

use Thread;
use Client;
use Circ::Env;
use Circ::Feep::Etab;
use Circ::Feep::Etab1;
use Circ::Feep::Etab2;
use Circ::Feep::Etab3;
use Circ::Feep::Etab4;

my @ctrls = (map {"pitch_$_"} qw/ Etab Etab1 Etab2 Etab3 Etab4 /);

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


intro({number    => 2,
       feeptype  => 'Etab4',
       sampleset => 'bassdm', 
       sampleno  => 4,
       pitch     => 50,
       pan       => 0.48,
       cutoff    => 10000,
       res       => 0.5,
      }
     );
intro({number    => 1,
       feeptype  => 'Etab4',
       sampleset => 'bassdm', 
       sampleno  => 3,
       pitch     => 45
      }
     );

intro({number    => 3, 
       feeptype  => 'Etab3',
       sampleset => 'mouth',
       sampleno  => 0
      }
     );

intro({number    => 3, 
       feeptype  => 'Etab2',
       sampleset => 'bubble', 
       sampleno  => 0,
       pan       => 0.68,
      }
     );

intro({number    => 4, 
       feeptype  => 'Etab1',
       sampleset => 'sid', 
       sampleno  => 3,
       pitch     => 30,
       pan       => 0.48,
      }
     );

intro({number    => 4, 
       feeptype  => 'Etab',
       sampleset => 'sid', 
       sampleno  => 2,
       pitch     => 30,
       pan       => 0.45,
      }
     );

sub intro {
    my $p = shift;
    my $pkg = "Circ::Feep::$p->{feeptype}";
    my $samples = samples($p->{sampleset});
    my $sample = $samples->[exists $p->{sampleno}
			    ? $p->{sampleno}
			    : rand @$samples
			   ];
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

    for (1 .. ($p->{number} or 1)) {
	$env->introduce($pkg->new({env       => $env,
				   trigger   => 
				   sub { my @foo = @sample_p;
					 $foo[1] += (value('pitch_' 
							    . $p->{feeptype}
							   )
						     or 0
						    );
				       $c->play_sample(@foo), 
				     },
				  }
				 )
		       );
    }
}


$c->event_loop;

##

sub on_ctrl {
    my ($ctrl, @data) = @_;
    $values{$ctrl} = $data[0];
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
