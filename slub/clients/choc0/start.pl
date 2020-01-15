#!/usr/bin/perl

##

use strict;

use Curses;

use Client;
use Data::Dumper;

##

my $marginy = 6;
my $marginx = 8;

my $maxx = 15;
my $maxy = 7;
my $box_size = ($maxx + 1) * ($maxy + 1);
my $max_tries = $box_size;

##

my $tick = 0;
my $input;

##

my @samples;
my $client;
my @edits;
my $edit;
my $edit_buffer = '';
my @eeps;
my @chrs = qw(o x ~ = - + * % @ \ / ! ?);
my $box;

init();

sub init {
    $client = Client->new(port      => 6011,
			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => ($ENV{TPB} || 16)
			 );
    @samples = map {$client->init_samples($_)} @ARGV;

    box_init();
    curses_init();

    $client->event_loop;
}

##

sub box_init {
    $box = [map { [map {[]} (0 .. $maxx)] } (0 .. $maxy)];
}

sub box_redraw {
    my($x, $y) = (0, 0);

    foreach my $row (@$box) {
	$x = 0;
	foreach my $cell (@$row) {
	    addstr($y + $marginy, $x + $marginx,
		   (@$cell ? $cell->[$tick % @$cell]->{chr} : '.')
		  );
	    $x += 2;
	}
	++$y;
    }
    move(0, 0);
    refresh();
}

##

sub curses_init {
    initscr;
    cbreak;
    noecho;
    keypad 1;
    nonl;
    nodelay 1;
    box_redraw();
    addstr(1, 1, "strength");
    addstr(2, 1, "spread");
    addstr(3, 1, "racism");
}

##

{
    my $racist_power;
    #my $last_eep;
    my %spreads;
    sub next_eeps {
	my @result;
	#++$last_eep;
	my $racist_strength = 0;
	if ($racist_power) {
	    if (--$racist_power->[0] <= 0) {
		undef $racist_power;
	    }
	    else {
		$racist_strength = $racist_power->[1];
	    }
	}

	foreach my $eep (sort {$b->{strength} <=> $a->{strength}} @eeps) {
	    if ($eep->{strength} > $racist_strength) {
		push @result, $eep;
		my $last_tick = $spreads{$eep->{number}} = $tick;
		if (($tick - $last_tick) >= $eep->{spread}) {
		    $racist_power = [$eep->{racism}, $eep->{strength}];
		    $racist_strength = $eep->{strength};
		    push @result, $eep;
		    #$last_eep = 0;
		}
	    }
	}
	return(@result);
    }
}

##

sub on_clock {
    my $cellno = ($tick % ($box_size));
    my $y = int($cellno / ($maxx + 1));
    my $x = $cellno % ($maxx + 1);
    my $cell = $box->[$y]->[$x];
    addstr(23,60, "x: $x y: $y c: $cellno     ");

    my @$cell = next_eeps();
    foreach my $eep (@$cell) {
	$client->play_sample($eep->{sample}, 
			     ($ENV{PITCH} || 60),    # pitch
			     20000, # cutoff
			     ($ENV{VOL} || 20),
			     0.5    # pan
			    );
    }

    box_redraw();

    if ((not $edit) and @edits) {
	$edit = shift @edits;
	addstr(23, 2,  sprintf("%10s: 0", $edit->{name}, ' ' x 40));
    }

    while((my $ch = getch()) ne ERR) {

	if ($edit) {
	    if ($ch =~ /^\d$/) {
		if (length $ch < 3) {
		    $edit_buffer .= $ch;
		}
	    }
	    elsif ($ch eq KEY_ENTER or $ch eq "\r") {
		$edit->{callback}->($edit_buffer);
		$edit_buffer = '';
		addstr(23, 2, ' ' x 70);
		undef $edit;
	    }
	    elsif ($edit_buffer and $ch eq KEY_BACKSPACE or $ch eq KEY_DC) {
		$edit_buffer =~ s/.$//;
	    }
	    if ($edit) {
		addstr(23, 2, sprintf("%10s: %s_ ", $edit->{name}, $edit_buffer));
	    }
	}
	elsif ($ch =~ /^\d+$/ and $ch >= 265 and $ch <= (265 + $#samples)) {
	    if (not @edits) {
		my $number = $ch - 264;

		my %edit = (chr => $chrs[$number % scalar(@chrs)],
			    number => $number,
			    sample => $samples[$number % scalar(@samples)]
			   );
		push(@edits, ({name => "eep$number strength",
			       callback => sub {$edit{strength} = shift}
			      },
			      {name => "eep$number spread",
			       callback => sub {$edit{spread}   = shift}
			      },
			      {name => "eep$number racism",
			       callback => sub {$edit{racism}   = shift;
						add_eep($number, \%edit)
					    }
			      },
			     )
		    );
	    }
	}
    }

    move(0, 0);
    status();
    refresh;
    $tick++;
}

##

sub add_eep {
    my ($number, $eep) = @_;
    $eeps[$number] = $eep;
    addstr(0, $marginx + ($number * 8), "eep$number");
    addstr(1, $marginx + ($number * 8), $eep->{strength});
    addstr(2, $marginx + ($number * 8), $eep->{spread});
    addstr(3, $marginx + ($number * 8), $eep->{racism});
}

##

sub logit {
    `logger "$_[0]"`;
}

##

1;

