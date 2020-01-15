#!/usr/bin/perl -w

use strict;

use Tk;
use Tk::Canvas;
use IO::Handle;
use Client;

my $client;

my $slideroffset = 64;

my $bip_radius = 3;

my @names = (
	    );

my @ctrls = ();

my %sample_y;

init();
MainLoop();

sub init {
    $client = Client->new(port	    => 6010,
			  on_clock  => \&on_clock,
			  on_ctrl   => \&on_ctrl,
			  ctrls     => \@ctrls,
			  timer     => 'ext',
			  tpb	    => 16
			 );

    my $main = MainWindow->new();
    $main->wm("geometry", "800x600");
    $main->fileevent($client->sock(), readable => sub {$client->poll});

    foreach my $line (@lines) {
	my $frame = $main->Frame->pack(-side => 'top'
				      );
	init_canvas($frame, $line);
    }

}

##

sub make {
    my $line = shift;
    my $msg = "make $line->{name} from $line->{synth}";
    
    $client->send('MSG', $msg);
}


sub init_canvas {
    my ($main, $line) = @_;
    my $lframe = $main->Frame;
    $lframe->pack(-side => 'left', -expand => 1, -fill => 'both');
    my $rframe = $main->Frame;
    $rframe->pack(-side => 'right', -expand => 1, -fill => 'both');


    my $c = $lframe->Canvas(-width => 500,
			    -height => 100
			   );
    init_lines($c, $line);
    $c->pack(-expand => 1, -fill => 'both');


    my $buttons = $rframe->Frame->pack(-side => 'top');
    
    $buttons->Optionmenu(-options => [qw( s t q r )],
			 -command => sub {set($line, 0, shift)}
			)->pack(-side => 'left');
    $buttons->Button(-text => 'seq+',
		     -command => sub { cp_seq($c, $line) }
		    )->pack(-side => 'left');
    $buttons->Button(-text => 'seq-',
		     -command => sub { rm_seq($c, $line) }
		    )->pack(-side => 'left');
    $buttons->Button(-text => 'rhythm+',
		     -command => sub { cp_rhythm($c, $line) }
		    )->pack(-side => 'left');
    $buttons->Button(-text => 'rhythm-',
		     -command => sub { rm_rhythm($c, $line) }
		    )->pack(-side => 'left');
    
    my $scales = $rframe->Frame->pack(-side => 'top');
    foreach my $place (1 .. 3) {
	$scales->Scale(-from => 0,
		       -to   => 25,
		       -command => sub {set($line, $place, 
					    chr(ord('a') + $_[0]), 
					    @_
					   )
				       },
		      )->pack(-side => 'left');
    }

    $scales->Scale(-from => 0,
		   -to	 => 20000,
		   -variable => \$line->{cutoff},
		   #-resolution => 50,
		  )->pack(-side => 'left');
    $scales->Scale(-from => 0,
		   -to	 => 1,
		   -variable => \$line->{res},
		   -resolution => 0.01
		  )->pack(-side => 'left');

    $scales->Scale(-from => 0,
		   -to	 => 100,
		   -variable => \$line->{vol}
		   #-resolution => 0.5,
		  )->pack(-side => 'left');
    $scales->Scale(-from => 0,
		   -to	 => 20,
		   -variable => \$line->{chorus_size},
		   -resolution => 1,
		  )->pack(-side => 'left');
    $scales->Scale(-from => 0,
		   -to	 => 5,
		   -variable => \$line->{chorus_depth},
		   -resolution => 0.05,
		  )->pack(-side => 'left');
    
    return $c;
}

##

sub set {
    my ($line, $place, $char) = @_;
    my $synth = $line->{synth};

    substr($synth, $place, 1, $char);

    $line->{synth} = $synth;
    make($line);
}

##

sub update_info {
    my $c = shift;

}

##

sub init_lines {
    my($c, $line) = @_;

    my $rhythm = $line->{rhythm};
    my $seq    = $line->{seq};
    my ($rhythm_point, $seq_point) = (0, 0);

    my $last;
    my $last_id;

    my $curr = [0, 0];

    foreach my $beat (0 .. 63) {
	my $seq_point = ($beat % scalar(@$seq));
	my $rhythm_point = ($beat % scalar(@$rhythm));
	
	$curr->[1] = $seq->[$seq_point];
	
	my $id = add_bip($c, @$curr, 
			 "seq_$seq_point", "rhythm_$rhythm_point",
			 "beat_$beat"
			);
	
	if ($last) {
	#    $c->create(('line', @$last, @$curr),
	#	       -tags => ["from_$last_id",
	#			 "to_$id",
	#			]
	#	      );
	}

	@$last = @$curr;
	$last_id = $id;

	$curr->[0] += $rhythm->[$rhythm_point];
    }

    foreach my $seq_point (0 .. (scalar(@$seq) -1)) {
	my $seq_tag = "seq_$seq_point";
	$c->bind($seq_tag, '<B1-Motion>',
		 [\&move_seq,
		  $line,
		  $seq_point,
		  $seq_tag,
		  Ev('x'), Ev('y'), 
		 ]
		);
    }

    foreach my $rhythm_point (0 .. (scalar(@$rhythm) -1)) {
	my $rhythm_tag = "rhythm_$rhythm_point";
	$c->bind($rhythm_tag, '<ButtonPress-2>',
		 [\&touch_rhythm,
		  $line,
		  $rhythm_point,
		  $rhythm_tag,
		  Ev('x'), Ev('y'),
		 ]
		);
	$c->bind($rhythm_tag, '<B2-Motion>',
		 [\&move_rhythm,
		  $line,
		  $rhythm_point,
		  $rhythm_tag,
		  Ev('x'), Ev('y'),
		 ]
		);
    }
}

sub move_seq {
    my ($c, $line, $seq_point, $tag, $x, $y) = @_;
    return if $x < 0 or $y < 0;

    my $seq = $line->{seq};

    my @bips = $c->find('withtag', $tag);

    $seq->[$seq_point] = $y;

    foreach my $bip (@bips) {
	my @coords = $c->coords($bip);
	$coords[1] = $y - $bip_radius;
	$coords[3] = $y + $bip_radius;
	$c->coords($bip, @coords);

	my @lines;
	(@lines) = $c->find('withtag', "from_$bip");
	foreach my $line (@lines) {
	    @coords = $c->coords($line);
	    $coords[1] = $y;
	    $c->coords($line, @coords);
	}

	(@lines) = $c->find('withtag', "to_$bip");
	foreach my $line (@lines) {
	    @coords = $c->coords($line);
	    $coords[3] = $y;
	    $c->coords($line, @coords);
	}

    }
}

##

my $start_x;
sub touch_rhythm {
    my ($c, $line, $rhythm_point, $tag, $x, $y) = @_;
    $start_x = $x;
}

##

sub move_rhythm {
    my ($c, $line, $rhythm_point, $tag, $x, $y) = @_;
    return if $x < 0 or $y < 0;
    my $rhythm = $line->{rhythm};
    my $x_offset = $start_x - $x;
    $start_x = $x;
    my @bips = $c->find('withtag', $tag);
    
    $rhythm_point = ($rhythm_point - 1) % scalar(@$rhythm);
    my $next_rhythm_point = ($rhythm_point + 1) % scalar(@$rhythm);

    if (($rhythm->[$rhythm_point] - $x_offset) < 0) {
	$x_offset = 0 - $rhythm->[$rhythm_point];
	$rhythm->[$rhythm_point] = 0;
    }
    else {
	$rhythm->[$rhythm_point] -= $x_offset;
    }
    $rhythm->[$next_rhythm_point] += $x_offset;

    if ($rhythm->[$next_rhythm_point]  < 0) {
	$x_offset -= $rhythm->[$next_rhythm_point];
	$rhythm->[$rhythm_point] += $rhythm->[$next_rhythm_point];
	$rhythm->[$next_rhythm_point] = 0;
    }

    foreach my $bip (@bips) {
	my @coords = $c->coords($bip);
	$coords[0] = $coords[0] - $x_offset;
	$coords[2] = $coords[2] - $x_offset;
	$c->coords($bip, @coords);
	
	my @lines;
	(@lines) = $c->find('withtag', "from_$bip");
	foreach my $line (@lines) {
	    @coords = $c->coords($line);
	    $coords[0] -= $x_offset;
	    $c->coords($line, @coords);
	}

	(@lines) = $c->find('withtag', "to_$bip");
	foreach my $line (@lines) {
	    @coords = $c->coords($line);
	    $coords[2] -= $x_offset;
	    $c->coords($line, @coords);
	}

    }
}

##

sub cp_seq {
    my ($c, $line) = @_;

    push(@{$line->{seq}}, $line->{seq}->[-1]);
    redraw($c, $line);
}

##

sub rm_seq {
    my ($c, $line) = @_;
    return if scalar @{$line->{seq}} <= 1;
    pop(@{$line->{seq}});
    redraw($c, $line);
}

##

sub cp_rhythm {
    my ($c, $line) = @_;

    push(@{$line->{rhythm}}, 15);
    redraw($c, $line);
}

##

sub rm_rhythm {
    my ($c, $line) = @_;
    return if scalar @{$line->{rhythm}} <= 2;
    $line->{rhythm}->[-2] += $line->{rhythm}->[-1];
    pop(@{$line->{rhythm}});
    redraw($c, $line);
}

##

sub redraw {
    my ($c, $line) = @_;

    my $rhythm = $line->{rhythm};
    my $seq = $line->{seq};
    my $curr = [0, 0];
    my $last;
    my $last_id;
    
    foreach my $beat (0 .. 63) {
	my ($bip) = $c->find('withtag', "beat_$beat");

	my $seq_point = ($beat % scalar(@$seq));
	my $rhythm_point = ($beat % scalar(@$rhythm));
	
	$curr->[1] = $seq->[$seq_point];

	move_bip($c, $bip, @$curr);

	foreach my $tag ($c->gettags($bip)) {
	    $c->dtag($bip, $tag);
	}

	foreach my $tag ("rhythm_$rhythm_point", "beat_$beat", 
			 "seq_$seq_point"
			) {
	    $c->addtag($tag, 'withtag', $bip);
	}
	
	if ($last) {
	#    $c->create(('line', @$last, @$curr),
	#	       -tags => ["from_$last_id",
	#			 "to_$id",
	#			]
	#	      );
	}

	@$last = @$curr;
	$last_id = $bip;

	$curr->[0] += $rhythm->[$rhythm_point];
    }
}

##

sub add_bip {
    my ($c, $x, $y, @tags) = @_;

    $c->create(('oval',
		$x - $bip_radius, $y - $bip_radius,
		$x + $bip_radius, $y + $bip_radius
	       ),
	       -fill => 'gray',
	       -tags => \@tags
	      );
}

##

sub move_bip {
    my ($c, $bip, $x, $y) = @_;

    $c->coords($bip,
	       $x - $bip_radius, $y - $bip_radius,
	       $x + $bip_radius, $y + $bip_radius
	      );
}

##

sub add_bop {
    my ($c) = @_;
}

##

sub on_clock {
    foreach my $line (@lines) {
	if ($line->{silence}) {
	    --$line->{silence};
	    next;
	}
	
	@{$line->{rhythm_copy}} = @{$line->{rhythm}}
	  if not @{$line->{rhythm_copy}};
	
	@{$line->{seq_copy}} = @{$line->{seq}}
	  if not @{$line->{seq_copy}};
	
	$line->{silence} = shift @{$line->{rhythm_copy}};
	$line->{silence}--;
	
	if ($line->{vol}) {
	    my $sample;
	    my $pitch;
	    
	    $pitch = 40 + shift @{$line->{seq_copy}};
	    $pitch = 1 if not $pitch;
	    $pitch = ($pitch % 100);
	    my @chorus;
	    if (my $size = $line->{chorus_size}) {
		my $depth = $line->{chorus_depth};
		foreach my $voice (0 .. $size) {
		    
		    my $chorus_value = ($depth * ($voice / $size)
					- ($depth / 2)
				       );
		    warn("chorus_value: $chorus_value\n");
		    push @chorus, ($pitch + $chorus_value);
		}
	    }
	    else {
		@chorus = ($pitch);
	    }
	    foreach my $note (@chorus) {
		$client->send('MSG', 
			      "play $line->{name} pitch $note decay 50 "
			      . "vol $line->{vol} cutoff $line->{cutoff} "
			      . "res $line->{res}"
			     );
	    }
	}
	
    }
}

