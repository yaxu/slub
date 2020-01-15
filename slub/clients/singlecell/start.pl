#!/usr/bin/perl -w

package main;

use strict;

use Client;
use Curses;

use old::Old;
use old::Old::Bang;
use old::Old::Break;
use old::Old::Crack;
use old::Old::Line;
use old::Old::Off;
use old::Old::Techno;
use old::Old::Techno1;
use old::Old::Two;

my $loopsize = 32;

my $margin = 8;

my $cellsize = 12;

my $update_freq = 62;

my $update_freq2 = 1;

my $max_samples = 6;

my $c = Client->new(port     => 6010,
		    on_ctrl  => \&on_ctrl,
		    on_clock => \&on_clock,
		    timer    => 'ext',
		    tpb      => ($ENV{TPB} || 4),
		   );

my @samples = map {$c->init_samples($_)} @ARGV;


if (@samples > $max_samples) {
    $#samples = $max_samples - 1;
}

my @aged;
foreach my $c (0 .. $#samples) {
    my $sample = $samples[$c];

    if ($sample =~ /^(drum|lighter)0$/) {
	push @aged, old::Old::Bang->new(sample   => $sample,
					loopsize => 16,
					pitch    => 60
				       );
    }
    elsif ($sample =~ /^break/) {
	push @aged, old::Old::Break->new(sample   => $sample,
					 loopsize => 32,
					 pitch    => 60
					);
    }
    elsif ($sample =~ /^(drum|lighter)1$/) {
	push @aged, old::Old::Crack->new(sample   => $sample,
					 loopsize => 16,
					 pitch    => 60
					);
    }
    elsif ($sample eq 'techno0') {
	push @aged, old::Old::Techno->new(sample   => $sample,
					  loopsize => 16,
					  pitch    => 60
					 );
    }
    elsif ($sample =~ /^jvbass/) {
	push @aged, old::Old::Line->new(sample   => $sample,
					loopsize => 16,
					pitch    => 60
				       );
    }
    elsif ($sample =~ /^gabba/) {
	push @aged, old::Old::Two->new(sample   => $sample,
					loopsize => 16,
					pitch    => 60
		         	       );
    }
    elsif ($sample =~ /^off/) {
	push @aged, old::Old::Off->new(sample   => $sample,
				       loopsize => $loopsize,
				       pitch    => 60
				      );
    }
    else {
	push @aged, old::Old->new(sample   => $sample,
				  loopsize => $loopsize,
				  pitch    => ($ENV{PITCH} or 60)
				 );
    }
}

curses_init();

$c->event_loop;

sub on_ctrl {
}

my $tick;
my %mute;

my $played;

my ($channel, $effect);

sub on_clock {

    $tick ||= 0;

    my $lookup = key_lookup();

    while((my $ch = getch()) ne ERR) {
	if(exists $lookup->{$ch}) {
	    ($channel, $effect) = @{$lookup->{$ch}};
	    cursor($channel, $effect);
	}
	elsif ($ch =~ /^\d+$/ and $ch >= 265 and $ch <= (265 + $#samples)) {
	    # f-key
	    my $channel = $ch - 265;
	    # toggle mute
	    $mute{$channel} = $mute{$channel} ? 0 : 1;
	    update_values();
	}
	elsif (defined $channel) {
	    if ($ch eq KEY_RIGHT) {
		$aged[$channel]->twiddle(peas()->[$effect], 1);
		update_values();
	    }
	    elsif ($ch eq KEY_LEFT) {
		$aged[$channel]->twiddle(peas()->[$effect], 0);
		update_values();
	    }
	}
    }


    foreach my $channel (0 .. $#aged) {
	next if $mute{$channel};
	my $old = $aged[$channel];
	if (my $next = $old->next_beat) {
	    $played->[$channel]++;
	    my @p;
	    foreach my $sp (qw(pitch cutoff vol pan res)) {
		push @p, $next->{$sp};
	    }
	    $c->play_sample($old->sample, @p);
	}
    }
    
    if (not $tick % $update_freq) {
	update_values();
    }

    if (not $tick % $update_freq2) {
	update_beats();
    }

    refresh();

    ++$tick;
}

##

sub update_beats {

    foreach my $column ( 0 .. (@samples - 1)) {
	my $title = $samples[$column];
	if ($played->[$column]) {
	    addstr(0, $margin + ($column * $cellsize) + int($cellsize / 2) - int(length($title) / 2) - 1, 
		   "-$title-"
		  );
	}
	else {
	    addstr(0, $margin + ($column * $cellsize) + int($cellsize / 2) - int(length($title) / 2) - 1, 
		   " $title "
		  );
	}
    }
    
    $played = [];
}

##

{
    my ($x, $y);
    sub cursor {
	my ($channel, $effect) = @_;
	
	if (defined $x) {
	    addch($y, $x + 1, '[');
	    addch($y, $x + $cellsize - 2, ']');
	}

	$x = $margin + ($channel * $cellsize);
	$y = ($effect * 2) + 2;
	addch($y, $x + 1, '>');
	addch($y, $x + $cellsize - 2, '<');
    }
}

##

{
    my $keys;
    sub _keys {
	$keys ||= [
		   [qw| 1 2 3 4 5 6 7 8 |],
		   [qw| q w e r t y u i |],
		   [qw| a s d f g h j k |],
		   [qw| z x c v b n m |,','],
		   [qw| ! " £ $ % ^ & * |],
		   [qw| Q W E R T Y U I |],
		   [qw| A S D F G H J K |],
		   [qw| Z X C V B N M < |]
		  ];
    }
}

##

{
    my $peas;
    sub peas {
	$peas ||= [qw| maxhits gap vol snapsz offset cutoff res pan |];
    }
}

##

{
    my $lookup;
    sub key_lookup {
	my $keys = _keys();
	
	if (not $lookup) {
	    foreach my $y (0 .. @$keys - 1) {
		foreach my $x (0 .. @samples - 1) {
		    $lookup->{$keys->[$y]->[$x]} = [$x, $y];
		}
	    }
	}

	return $lookup;
    }
}

##

sub hilite {
    my ($thing, $on) = @_;

    my $text = $on ? "[*$thing*]" : "  $thing  ";
    addstr(0, $margin + (($thing - 1) * $cellsize) + int($cellsize / 2), 
	   $text
	  );
}

##

sub draw_titles {
    my $keys = _keys();
    my $peas = peas();

    foreach my $column ( 0 .. (@samples - 1)) {
	my $title = $samples[$column];
	addstr(0, $margin + ($column * $cellsize) + int($cellsize / 2) - int(length($title) / 2), 
	       $title
	      );
	foreach my $row (0 .. @$peas - 1) {
	    addstr(2 + ($row * 2), $margin + ($column * $cellsize), 
		   $keys->[$row]->[$column] . '[' . (' ' x ($cellsize - 4)) .  ']'
		  );
	}
    }
    
    foreach my $row (0 .. @$peas - 1) {
	addstr(2 + ($row * 2), 0, 
	       $peas->[$row]
	      );
    }
}

##

sub update_values {
    my $peas = peas();
    foreach my $x (0 .. @aged - 1) {
	foreach my $y (0 .. @$peas - 1) {
	    my $func = $peas->[$y];
	    my $hash = $aged[$x]->$func();
	    addstr(2 + ($y * 2), $margin + ($x * $cellsize) + 2,
		   ($mute{$x}
		    ? "  stop  "
		    : sprintf($hash->{format}, $hash->{value})
		   )
		  );
	}
    }
}

##

sub curses_init {
  initscr;
  cbreak;
  noecho;
  keypad 1;
  nonl;
  nodelay 1;
  draw_titles();
}

##

sub curses_deinit {
  endwin;
}

##


1;
