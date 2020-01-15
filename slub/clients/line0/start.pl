#!/usr/bin/perl -w

use strict;

use Tk;
use Tk::Canvas;
use IO::Handle;
use Client;

my $client;

my @samples;

my $slideroffset = 64;

my $rhythm = [7, 15, 15, 15, 7, 15, 7, 7];
my $seq = [100, 120, 150, 100, 120, 150];
my $bip_radius = 3;

my %sample_y;

init();
MainLoop();

sub init {
    $client = Client->new(port      => 6010,
    			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => 16
			 );

    @samples = map {$client->init_samples($_)} $ARGV[0];

    if ($ARGV[1]) {
	@samples = ($samples[0]);
    }

    my $c = init_canvas();

    $c->pack(-expand => 1, -fill => 'both');
}

##

sub init_canvas {
    my $main = MainWindow->new();
    $main->wm("geometry", "400x300");
    $main->fileevent($client->sock(), readable => sub {$client->poll});

    my $c = $main->Canvas(-width => 800,
			  -height => 600
			 );
    init_lines($c);
    return $c;
}

##

#sub init_info {
#    my $c = shift;
#    $c->create(('text',
#		$x, $y
#	       ),
#	       -fill    => 'black',
#	       -text    => $sampleset,
#	       -tags    => 'rhythm'
#	      );
#    $c->create(('text',
#		$x, $y
#	       ),
#	       -fill    => 'black',
#	       -text    => $sampleset,
#	       -tags    => 'seq'
#	      );
#
#}

##

sub update_info {
    my $c = shift;

}

##

sub init_lines {
    my $c = shift;

    my ($rhythm_point, $seq_point) = (0, 0);

    my $last;
    my $last_id;

    my $curr = [0, 0];

    foreach my $beat (0 .. 80) {
	my $seq_point = ($beat % scalar(@$seq));
	my $rhythm_point = ($beat % scalar(@$rhythm));
	
	$curr->[1] = $seq->[$seq_point];
	
	my $id = add_bip($c, @$curr, 
			 "seq_$seq_point", "rhythm_$rhythm_point"
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
		  $rhythm_point,
		  $rhythm_tag,
		  Ev('x'), Ev('y'),
		 ]
		);
	$c->bind($rhythm_tag, '<B2-Motion>',
		 [\&move_rhythm,
		  $rhythm_point,
		  $rhythm_tag,
		  Ev('x'), Ev('y'),
		 ]
		);
    }
}

sub init_samplelines {
    my $c = shift;
    my $y = 150;
    my $y_inc = int(400 / scalar(@samples));
    foreach my $sample (@samples) {
	$c->create(('line', 0, $y, 600, $y),
		   -tags => ["line_$sample"]
		  );
	$y += $y_inc;
    }
}

sub move_seq {
    my ($c, $seq_point, $tag, $x, $y) = @_;

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
    my ($c, $rhythm_point, $tag, $x, $y) = @_;
    $start_x = $x;
}

##

sub move_rhythm {
    my ($c, $rhythm_point, $tag, $x, $y) = @_;
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

sub add_bop {
    my ($c) = @_;
}

##
my @rhythm_copy;
my @seq_copy;
my $silence;
sub on_clock {
    if ($silence) {
	--$silence;
	return;
    }

    @rhythm_copy = @$rhythm if not @rhythm_copy;
    @seq_copy = @$seq if not @seq_copy;

    $silence = shift @rhythm_copy;

    my $sample;
    my $pitch;
    #if (@samples == 1) {
	$pitch = 40 + shift @seq_copy;
	$pitch = 1 if not $pitch;
	$sample = $samples[0];
    #}
    #else {
#	$pitch = 60;
#	$sample = $samples[int(shift(@seq_copy) / 16) % @samples];
#    }
    $pitch = (($pitch + 100) % 200) - 100;
      
    #$client->play_sample($sample, $pitch);
    my $msg = ("make foo from "
	       . join('', 
		      #(qw/s t q r/)[rand(4)],
		      'saab',
		      #chr(ord('l') + rand(3)),
		      #chr(ord('u') + rand(3)),
		      #chr(ord('b') + rand(10)),
		     )
	      );
    print "$msg\n";
    $client->send('MSG', $msg);
    $client->send('MSG', "play foo pitch $pitch decay 50");
    print("play foo pitch $pitch decay 40\n");
}

