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
my %eeps;
my @chrs = qw(o x ~ = - + * % @ \ / ! ?);
my %placed;
my @placings;
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

sub drop_eep {
    my $eep = shift;
    $eep->{x} = 0;
    $eep->{y} = 0;
    push @placings, {eep => $eep, tries => 0};

    # sort in order of strength, strongest first
    @placings =
      map {$_->[0]} 
	sort {$b->[1] <=> $a->[1]}
	  map {[$_, $_->{strength}]}
	    @placings;
}

##

sub move_placings {
    my @new_placings;
    foreach my $placing (@placings) {
	if (++$placing->{tries} <= $max_tries) {
	    my ($x, $y) = ($placing->{eep}->{x}, $placing->{eep}->{y});
	    if (++$x > $maxx) {
		$x = 0;
		if (++$y > $maxy) {
		    $y = 0;
		}
	    }
	    if (not place($placing)) {
		($placing->{eep}->{x}, $placing->{eep}->{y}) = ($x, $y);
		push @new_placings, $placing;
		addstr($y + $marginy, ($x * 2) + $marginx,
		       $placing->{eep}->{chr}
		      );
	    }
	}
    }
    @placings = @new_placings;
}

sub place {
    my $placing = shift;
    my $strength = $placing->{eep}->{strength};
    my @stronger = map {@{$placed{$_}}} grep {$_ >= $strength} keys %placed;

    my $ok = 1;
    my $count = 0;
    foreach my $strong_eep (@stronger) {
	my $distance = distance($strong_eep, $placing->{eep});
	my $foo;
	if ($strong_eep->{number} == $placing->{eep}->{number}) {
	    $foo = 'spread';
	}
	else {
	    $foo = 'racism';
	}
	if (($distance <= $strong_eep->{$foo})
	    or ($distance <= $placing->{eep}->{$foo})
	   ) {
	    $ok = 0;
	    last;
	}
	++$count;
    }

    if ($ok) {
	my @weaker = map {@{$placed{$_}}} grep {$_ < $strength} keys %placed;
	my $eep = $placing->{eep};
	my @unplacers;
	foreach my $weak_eep (@weaker) {
	    my $distance = distance($eep, $weak_eep);
	    my $foo;
	    if ($eep->{number} == $weak_eep->{number}) {
		$foo = 'spread';
	    }
	    else {
		$foo = 'racism';
	    }
	    if (($distance <= $weak_eep->{$foo})
		or ($distance <= $eep->{$foo})
	       ) {
		push @unplacers, $weak_eep;
	    }
	}

	foreach my $unplacer (@unplacers) {
	    my $cell = $box->[$unplacer->{y}]->[$unplacer->{x}];
	    @$cell = grep {$unplacer->{number} != $_->{number}} @$cell;
	    @{$placed{$unplacer->{strength}}} = 
	      grep {$_ ne $unplacer}
		      @{$placed{$unplacer->{strength}}};
	    push @placings, {eep => $unplacer,
			     tries => 0
			    };
	}

	push(@{$box->[$eep->{y}]->[$eep->{x}]}, $eep);
	push(@{$placed{$eep->{strength}}}, $eep);
	
    }

    return $ok;
}

##

sub distance {
    my ($eep1, $eep2) = @_;
    my $point1 = $eep1->{x} + ($eep1->{y} * ($maxx + 1));
    my $point2 = $eep2->{x} + ($eep2->{y} * ($maxx + 1));
    my $distance = ($point1 > $point2 ? $point1 - $point2 : $point2 - $point1);

    if ($distance > ($box_size / 2)) {
	$distance = $box_size - $distance;
    }
    return $distance;
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

sub on_clock {
    my $cellno = ($tick % ($box_size));
    my $y = int($cellno / ($maxx + 1));
    my $x = $cellno % ($maxx + 1);
    my $cell = $box->[$y]->[$x];
    addstr(23,60, "x: $x y: $y c: $cellno     ");
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
		if (not $eeps{$number}) {
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
	elsif ($ch =~ /^\d$/ and exists $eeps{$ch}) {
	    my %new_eep = %{$eeps{$ch}};
	    drop_eep(\%new_eep);
	}
    }
    move_placings();
    move(0, 0);
    status();
    refresh;
    $tick++;
}

##

sub status {
    addstr(23, 40, sprintf("placings: %s     ", scalar(@placings)));
}

##

sub add_eep {
    my ($number, $eep) = @_;
    $eeps{$number} = $eep;
    addstr(0, $marginx + ($number * 8), "eep$number");
    addstr(1, $marginx + ($number * 8), $eep->{strength});
    addstr(2, $marginx + ($number * 8), $eep->{spread});
    addstr(3, $marginx + ($number * 8), $eep->{racism});
}

##

sub show_edits {
    addstr(22, 0, "edits: " . join(':', map {$_->{name}} @edits) . "." . rand());
    addstr(23, 0, "edits: " . scalar(@edits));
}

##

sub logit {
    `logger "$_[0]"`;
}

##

1;

