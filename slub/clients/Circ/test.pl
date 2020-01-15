#!/usr/bin/perl -w

use strict;

use lib '/slub/clients';

use Circ::Env;
use Circ::Feep::Etab;
use Circ::Feep::Etab1;

use Data::Dumper;

##

my $env = Circ::Env->new({x => 32,
			  y => 4
			 }
			);

for (0 .. 31) {
    $env->introduce(Circ::Feep::Etab->new({env       => $env,
					   trigger   => 
					   sub { print "ankle\n" },
					  }
					 )
		   );
}
for (0 .. 31) {
    $env->introduce(Circ::Feep::Etab1->new({env       => $env,
					   trigger   => 
					   sub { print "ankle\n" },
					  }
					 )
		   );
}

print $env->dump_env;

##

1;
