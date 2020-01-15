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

my $morph = 0;

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
			  tpb       => ($ENV{TPB} || 8)
			 );
    @samples = $client->init_samples($ARGV[0]);

    box_init();
    curses_init();

    if ($ARGV[1]) {
	load_eeps($ARGV[1]);
    }
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
	my @sorted_eeps =
	  sort {$b->{strength} <=> $a->{strength}}
	    grep {$_}
	      @eeps;

	foreach my $eep (@sorted_eeps) {
	    if ($eep->{strength} > $racist_strength) {
		my $last_tick = $spreads{$eep->{number}};
		if (($tick - $last_tick) >= $eep->{spread}) {
		    $racist_power = [$eep->{racism}, $eep->{strength}];
		    $racist_strength = $eep->{strength};
		    $spreads{$eep->{number}} = $tick;
		    push @result, $eep;
		    #$last_eep = 0;
		}
	    }
	}
	return(@result);
    }
}

##

{
    my $dump;
    sub dump_eeps {
	my $fn = shift;
	$dump ||= 0;

	$fn ||= ($ARGV[0]
		 . '_'
		 . $$
		 . '.'
		 . $dump
		);

	my $fn = "/slub/clients/choc/" . $fn;

	open(FH, ">$fn");
	print FH Dumper \@eeps;
	close FH;

	++$dump;
    }
}

##

sub load_eeps {
    my $fn = shift;
    local $/;
    undef $/;
    if ($fn !~ /\//) {
	$fn = "/slub/clients/choc/$fn";
	logit("fn: '$fn'");
	open(FH, "<$fn")
	  or return;
	my $dump = <FH>;
	close(FH);
	my $VAR1;
	eval($dump);
	@eeps = ();
	clear();
	box_redraw();
	foreach my $eep (grep {$_} @$VAR1) {
	    logit("adding $eep->{number}");
	    add_eep($eep);
	}
    }
    logit("loaded.");
}

##

sub on_clock {
    my $cellno = ($tick % ($box_size));
    my $y = int($cellno / ($maxx + 1));
    my $x = $cellno % ($maxx + 1);
    my $cell = $box->[$y]->[$x];
    addstr(23,60, "x: $x y: $y c: $cellno     ");

    @$cell = next_eeps();
    foreach my $eep (@$cell) {
	$client->play_sample($eep->{sample}, 
			     ($ENV{PITCH} || 60),
			     ($ENV{CUTOFF} || 20000),
			     ($ENV{VOL} || 20),
			     0.5    # pan
			    );
	morph_eep($eep);
    }

    box_redraw();

    if ((not $edit) and @edits) {
	$edit = shift @edits;
	addstr(23, 2,  sprintf("%10s: 0", $edit->{name}, ' ' x 40));
    }

    while((my $ch = getch()) ne ERR) {
	if ($edit) {
	    if (($edit->{alphanum} ? ($ch =~ /^\w$/) : ($ch =~ /^\d$/))) {
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
						add_eep(\%edit)
					    }
			      },
			     )
		    );
	    }
	}
	elsif ($ch eq 's') {
	    push(@edits, ({name => "save",
			   callback => \&dump_eeps,
			   alphanum => 1
			  }
			 )
		);
	}
	elsif ($ch eq 'm') {
	    $morph = ($morph ? 0 : 1);
	}
	elsif ($ch eq 'l') {
	    push(@edits, ({name => "load",
			   callback => \&load_eeps,
			   alphanum => 1
			  }
			 )
		);
	}
    }

    move(0, 0);
    refresh;
    $tick++;
}

##

sub add_eep {
    my ($eep) = @_;
    my $number = $eep->{number};
    $eeps[$number] = $eep;
    addstr(0, $marginx + ($number * 8), "eep$number$eep->{chr}");
    addstr(1, $marginx + ($number * 8), $eep->{strength});
    addstr(2, $marginx + ($number * 8), $eep->{spread});
    addstr(3, $marginx + ($number * 8), $eep->{racism});
}

##

sub morph_eep {
    my $eep = shift;
    morph($eep, 'strength', 0.05, 1, 0, 128);
    morph($eep, 'spread',   0.05, 1, 0, 30 );
    morph($eep, 'racism',   0.05, 1, 0, 16 );
}

sub morph {
    return if not $morph;
    my ($eep, $field, $chance, $amount, $min, $max) = @_;
    my $value = $eep->{$field};
    my $rand = rand(1);
    my $changed;

    if ($rand < ($chance / 2)) {
	$value -= $amount;
	$changed = 1;
    }
    elsif ($rand > (1 - ($chance / 2))) {
	$value += $amount;
	$changed = 1;
    }

    if ($changed) {
	if ($value > $max) {
	    $value = $max - ($value - $max);
	}
	if ($value < $min) {
	    $value = $min + ($min - $value);
	}
	$eep->{$field} = $value;
	add_eep($eep);
    }
}

##

sub logit {
    `logger "$_[0]"`;
}

##

1;

