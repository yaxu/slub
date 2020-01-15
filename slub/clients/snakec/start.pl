#!/usr/bin/perl

##

use strict;
use Curses;
use Client;
use Data::Dumper;

my @chrs = qw(o x ~ = - + * % @ \ / ! ?);
my $client;
my @samples;
my @snake;
my $tick;
my $footick;
my $direction = 1;
my $position = 0;
my $dest_length = 6;
my $reverse = 0;
my $change = 0;

init();

##

sub init {
    $client = Client->new(port      => 6011,
			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => ($ENV{TPB} || 2)
			 );
    @samples = map {$client->init_samples($_)} @ARGV;
    
    $tick = 0;
    $footick = 0;
    curses_init();
    draw_key();
    draw_status();
    refresh();
    $client->event_loop;
}

sub draw_key {
    my $y = 0;
    foreach my $sample (@samples) {
	addstr($y + 2, 2, "$sample $chrs[$y]");
	++$y;
    }
}

##

sub draw_status {
    my $length = scalar @snake;
    addstr(1, 1, "[ len $length | dlen $dest_length | rev $reverse | chg $change ]    ");
    
  
    my $x = 4;
    addstr(16, $x, '(');
    $x += 2;
    foreach my $seg (@snake) {
	addstr(16, $x, $chrs[$seg % @chrs]);
	$x += 2;
    }
    addstr(16, $x, ')');
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
    if ((not @snake) or ($change and (not $tick++ % 32))) {
	if (scalar(@snake) <= $dest_length) {
	    push @snake, [int rand @samples, (0.5 - 0.15 + rand(0.3))];
	}
	if (@snake > $dest_length) {
	    shift @snake;
	}
    }

    $position += $direction;
    if ($position < 0 or $position >= @snake) {
	$direction = ($direction > 0
		      ? -1
		      : 1
		     );
	$position += $direction;
	$position += $direction;
    }
    addstr(5, 60, "position: $position  ");
    my $pitch = ($ENV{PITCH} || 60);
    $client->play_sample($samples[$snake[$position]->[0]], 
			 ($reverse ? 
			  ($direction > 0 ? $pitch : 0 - $pitch)
			  : $pitch
			 ),
			 20000, 
			 ($ENV{VOL} || 20), 
			 $snake[$position]->[1]
			);
    while((my $ch = getch()) ne ERR) {
	if ($ch eq 'r') {
	    $reverse = $reverse ? 0 : 1;
	}
	elsif ($ch eq '=' or $ch eq '+') {
	    $dest_length++;
	}
	elsif ($ch eq '-' or $ch eq '_') {
	    $dest_length-- if $dest_length > 0;
	}
	elsif (lc($ch) eq 'c') {
	    $change = $change ? 0 : 1;
	}
    }
    draw_status();
    refresh();
}

##

1;

