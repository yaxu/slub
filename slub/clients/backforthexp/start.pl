#!/usr/bin/perl -w

package main;

##

use strict;
use Client2;
use Data::Dumper;
use Curses;

##

my $tick = 0;
my $selected;
my @samples;
my $client;
my $tpb = ($ENV{TPB} or 8);
my $vol = ($ENV{VOL} or 40);

my $length = 32;
my $mapdir = 0;
my @seq;
my @playmaps = 
  (
   [qw{  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
        16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
      }
   ],
   [qw{
       0  -1 1 -1  2  3  4  6  8 12 16 24 
      }
   ],
  );
my $mapsel = 0;

my @chrs = qw(o x ~ = - + * % @ \ / ! ?);

my $reverse = 0;

##

init();

##

sub init {
    curses_init();
    
    $client = Client2->new(port      => 6010,
			   on_clock  => \&on_clock,
			   timer     => 'ext',
			   tpb       => $tpb
			  );
    @samples = map {$client->init_samples($_)} @ARGV;
    current(0);
    
    @seq = map {undef} (0 .. ($length - 1));

    $client->event_loop;
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

sub on_clock {
    my $playmap = $playmaps[$mapsel];
    my $p = $playmap->[$reverse 
		       ? (scalar(@$playmap) - (($tick + 3) % @$playmap) - 1)
		       : ($tick % @$playmap)
		      ];
    
    if ($p >= 0) {
	if (my $sample = $seq[$p]) {
	    $client->play_sample($sample);
	}
    }
    
    while((my $ch = getch()) ne ERR) {
	my $sub = "key__$ch";
	if (main->can($sub)) {
	    main->$sub();
	}
    }
    
    ++$tick;
    if ($mapdir and (not ($tick % @$playmap))) {
	$mapsel = (($mapsel + $mapdir) % @playmaps);
	addstr(1, 20, $mapsel . '   ');
    }
}

##

sub current {
    my $current = shift;
    if (defined $current)  {
	$selected = $current;
	addstr(1, 3, $samples[$selected % @samples] . '    ');
    }
    else {
	$selected = undef;
	addstr(1, 3, 'rest               ');
    }
}

##

sub place {
    my $place = shift;
    if (defined $selected) {
	$seq[$place] = $samples[$selected];
	addstr(5, 10 + ($place * 2), $chrs[$selected % @chrs]);
    }
    else {
	$seq[$place] = undef;
	addstr(5, 10 + ($place * 2), ' ');
    }
}

##

sub key__1 { current(0) }
sub key__2 { current(1) }
sub key__3 { current(2) }
sub key__4 { current(3) }
sub key__5 { current(4) }
sub key__6 { current(5) }
sub key__7 { current(6) }
sub key__8 { current(7) }

sub key__0 { current( ) }

sub key__q { place(0)   }
sub key__Q { place(1)   }
sub key__w { place(2)   }
sub key__W { place(3)   }
sub key__e { place(4)   }
sub key__E { place(5)   }
sub key__r { place(6)   }
sub key__R { place(7)   }
sub key__t { place(8)   }
sub key__T { place(9)   }
sub key__y { place(10)  }
sub key__Y { place(11)  }
sub key__u { place(12)  }
sub key__U { place(13)  }
sub key__i { place(14)  }
sub key__I { place(15)  }

sub key__a { place(16)   }
sub key__A { place(17)   }
sub key__s { place(18)   }
sub key__S { place(19)   }
sub key__d { place(20)   }
sub key__D { place(21)   }
sub key__f { place(22)   }
sub key__F { place(23)   }
sub key__g { place(24)   }
sub key__G { place(25)   }
sub key__h { place(26)   }
sub key__H { place(27)   }
sub key__j { place(28)   }
sub key__J { place(29)   }
sub key__k { place(30)   }
sub key__K { place(31)   }

sub key__z {
    $reverse = not $reverse;
}

##

sub key__m {
    $mapsel = ($mapsel + 1) % @playmaps;
    addstr(1, 20, $mapsel . '   ');
    $mapdir = 0;
}


##

sub key__M {
    $mapdir = $mapdir ? 0 : 1;
}

##

sub key__n {
    $mapsel = ($mapsel - 1) % @playmaps;
    addstr(1, 20, $mapsel . '   ');
    $mapdir = 0;
}

##

sub key__N {
    $mapdir = $mapdir ? 0 : -1;
}

##

1;

