#!/usr/bin/perl -w

use strict;

use Tk;
use Tk::Canvas;
use IO::Handle;
use Client;

my @samples;
my $client;
my $gc;

my @controls = qw(volume);
my %values = map {$_ => 0} @controls;

init();
MainLoop();

sub init {
    $client = Client->new(port      => 6010,
			  ctrls     => \@controls,
			  on_ctrl   => \&on_ctrl,
			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => 4
			 ); 
    
    @samples = $client->init_samples($ARGV[0]);
    
    my $sock = $client->sock();
    
    $values{volume} = 1;

    init_canvas();
    
    $values{listen} = 1;
    
    $gc->pack(-expand => 1, -fill => 'both');
    
    #$main->bind('<KeyPress-r>', [\&reverse_phrase, $c]);
    #$main->bind('<KeyPress-l>', [\&toggle_ctrl_listen]);
    
    
}

##

sub init_canvas {
    # Create main window and canvas
    my $main = MainWindow->new();
    #$main->wm("minsize",  400,300);
    $main->wm("geometry", "400x300");
    $main->fileevent($client->sock(), readable => sub {$client->poll});
    my $c = $main->Canvas(-width => 800,
			  -height => 600
			 );
    $gc = $c;
    add_labels();
    add_radios();
}

##

sub add_labels {
    my $x = 16;
    my $y = 32;
    foreach my $label (qw/ BD HH SN OH CB P1 P2 FX /) {
	$gc->create(('text',
		     $x, $y
		    ),
		    -tags    => ["_text_$label",
				 'text'
				],
		    -fill    => 'black',
		    -text    => $label,
		   );
	make_slide($x, $y, $label . "_1");
	$y += 16;
	make_slide($x, $y, $label . "_2");
	$y += 16;
    }
}

##

sub add_radios {
    my $x = 128;
    my $y = 8;
    foreach my $label (qw/ house glitch jazz /) {
	$gc->create(('text',
		     $x, $y
		    ),
		    -tags    => ["_text_$label",
				 'text'
				],
		    -fill    => 'black',
		    -text    => $label,
		   );
	$gc->create(('oval',
		     $x - 4, $y - 4 + 16,
		     $x + 4, $y + 4 + 16
		    ),
		    -tags    => ["_text_$label",
				 'text'
				],
		    -fill    => 'white',
		    -outline => 'black'
		   );
	
	$x += 64;
    }
}

##

sub make_slide {
    my($x, $y, $label) = @_;
    $gc->create(('line',
		 $x + 16, $y,
		 $x + 16 + 255, $y
		),
		-tags    => ["_line_$label",
			     'line'
			    ],
		-fill    => 'black',
	       );
    my $tag = "_ctrl_$label";
    $gc->create(('rectangle',
		 $x + 16 - 2, $y-2,
		 $x + 16 + 2, $y+2,
		),
		-tags    => [$tag,
			     'ctrl'
			    ],
		-fill    => 'black',
	       );

    $gc->bind($tag, '<B1-Motion>', 
	      [\&move,
	       $tag,
	       Ev('x'), Ev('y'), 
	      ]
	     );
    
}

##

sub move {
    my ($c, $tag, $x, $y) = @_;

    if ($x > (256 + 32)) {
	$x = 256 + 32;
    }
    if ($x < 32) {
	$x = 32;
    }
    my @coords = $c->coords($tag);
    $coords[0] = $x - 2;
    $coords[2] = $x + 2;
    $c->coords($tag, @coords);

    $tag =~ s/^_ctrl_//;

    $client->ctrl_send($tag, $x - 32);
}

##

sub on_ctrl {
}

##

sub on_clock {
}

