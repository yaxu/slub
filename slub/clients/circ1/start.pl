#!/usr/bin/perl

package circpl;

use strict;

use Client;
use Circ::Env;
use Circ::Feep::Etab;
use Circ::Feep::Etab1;
use Circ::Feep::Etab2;
use Circ::Feep::Etab3;
use Circ::Feep::Etab4;

my @ctrls = ();

my $c = Client->new(port     => 6010,
		    ctrls    => \@ctrls,
		    on_ctrl  => \&on_ctrl,
		    on_clock => \&on_clock,
		    timer    => 'ext',
		    tpb      => 16,
		   );

my $env = Circ::Env->new({x => 32,
			  y => 4
			 }
			);

my @bassdm  = $c->init_samples('bassdm');
my @lighter = $c->init_samples('lighter');
my @bottle  = $c->init_samples('bottle');
my @mouth   = $c->init_samples('mouth');
my @bubble   = $c->init_samples('bubble');

my $lighter_sample = $lighter[rand @lighter];
my $bassdm_sample  = $bassdm[rand @bassdm];
my $bottle_sample  = $bottle[rand @bottle];
my $mouth_sample   = $mouth[rand @mouth];
my $bubble_sample   = $bubble[rand @bubble];

for (0 .. 3) {
    $env->introduce(Circ::Feep::Etab
		    ->new({env       => $env,
			   trigger   => 
			   sub { $c->play_sample($bassdm_sample) },
			  }
			 )
		   );
}
for (0 .. 15) {
    $env->introduce(Circ::Feep::Etab1
		    ->new({env       => $env,
			   trigger   => 
			   sub { $c->play_sample($lighter_sample) },
			  }
			 )
		   );
}
for (0 .. 7) {
    $env->introduce(Circ::Feep::Etab2
		    ->new({env       => $env,
			   trigger   => 
			   sub { $c->play_sample($bottle_sample) },
			  }
			 )
		   );
}
for (0 .. 4) {
    $env->introduce(Circ::Feep::Etab3
		    ->new({env       => $env,
			   trigger   => 
			   sub { $c->play_sample($mouth_sample) },
			  }
			 )
		   );
}

for (0 .. 4) {
    $env->introduce(Circ::Feep::Etab4
		    ->new({env       => $env,
			   trigger   => 
			   sub { $c->play_sample($bubble_sample) },
			  }
			 )
		   );
}

$c->event_loop;

##

sub on_ctrl {
    my ($ctrl, @data) = @_;
    my $func = "on__$ctrl";
    if (circpl->can($func)) {
	&$func(@data);
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
