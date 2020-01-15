#!/usr/bin/perl -w

use strict;
use Tk;
use Tk::Canvas;
use IO::Handle;
use Client;

use POSIX qw{atan};

use Data::Dumper;

my $client;

my $osize = 6;

my $c;

my $beat = 0;
my $gridsize = 300;
my @controls = qw(move);
my %values = map {$_ => 0} @controls;
my @samples;

my $scale = 500;

my %phantoms;

init();
MainLoop;

##

sub init {
  $client = Client->new(port      => 6010,
			ctrls     => \@controls,
			on_ctrl   => \&on_ctrl,
			on_clock  => \&on_clock,
			timer     => 'ext',
			tpb       => 16
		       ); 
  
  my $sock = $client->sock();

  init_canvas($sock);
}

##

sub init_canvas {
  my $sock = shift;
  # Create main window and canvas
  my $main = MainWindow->new();
  #$main->wm("minsize",  400,300);
  $main->wm("geometry", ($scale . 'x' . (($osize * 2 * 8) + 6)));
  $c = $main->Canvas(-width => $gridsize,
		     -height => $gridsize
		    );
  $c->pack(-expand => 1, -fill => 'both');
  
  $main->fileevent($client->sock(), readable => \&read_sock);
  @samples = $client->samples;
  
  add_controls(@ARGV);
}

##

{
  my $y;
  sub add_controls {
    $y ||= $osize;
    foreach my $ctrl (@_) {
      if(exists $values{$ctrl}) {
	$client->ctrl_send($ctrl, $values{$ctrl});
	$client->ctrl_send($ctrl, $values{$ctrl});
	next;
      }
      $values{$ctrl} = 0;
      $c->create(('text',
		  3, $y + 4
		 ),
		 -tags    => ["_text_$ctrl",
			      'text'
			     ],
		 -fill    => 'grey',
		 -text    => $ctrl,
		 -anchor => 'w'
		);
      $c->create(('text', 
		  $scale - 3, $y + 4
		 ),
		 -tags    => ["_value_$ctrl",
			      'valuetext'
			     ],
		 -fill    => 'grey',
		 -text    => 0,
		 -anchor => 'e'
		);
      
      $c->create(('oval', 
		  ($scale / 2) - int($osize / 2), $y, 
		  ($scale / 2) + int($osize / 2), $y + $osize
		 ),
		 -tags    => [$ctrl,
			      'controls'
			     ],
		 -fill    => 'black',
		 -outline => 'black'
		);
      
      
      $c->bind($ctrl, '<ButtonPress-1>', 
	       [\&move,
		'click',
		$ctrl,
		Ev('x'), Ev('y'), 
	       ]
	      );
      $c->bind($ctrl, '<ButtonPress-2>', 
	       [\&zero,
		$ctrl,
		Ev('x'), Ev('y'), 
	       ]
	      );
      $c->bind($ctrl, '<ButtonPress-3>', 
	       [\&phantom,
		'click',
		$ctrl,
		Ev('x'), Ev('y'), 
	       ]
	      );
      $c->bind($ctrl, '<B1-Motion>', 
	       [\&move,
		'drag',
		$ctrl,
		Ev('x'), Ev('y'), 
	       ]
	      );
      $y += $osize * 2;
    }
  }
}

##

sub zero {
  my ($c, $tag) = @_;

  $values{$tag} = 0;
  refresh_thing($tag);
}

##

sub phantom {
  my ($c, $type, $tag, $x, $y) = @_;
  if (not $c->find('withtag', "phantom_$tag")) {
    my @coords = $c->coords($tag);
    $c->create(('oval', 
		$x - int($osize / 2), $coords[1], $x + int($osize / 2), $coords[3]
	       ),
	       -tags    => ["phantom_$tag",
			    'phantoms'
			   ],
	       -fill    => 'grey',
	       -outline => 'grey'
	      );
    $c->bind("phantom_$tag", '<ButtonPress-3>', 
	     [\&delete,
	      "phantom_$tag",
	      $tag
	     ]
	    );
    $c->bind("phantom_$tag", '<B1-Motion>', 
	     [\&move,
	      'phantom',
	      "phantom_$tag",
	      Ev('x'), Ev('y'), 
	     ]
	    );
    $phantoms{$tag} = 0;
  }
}

sub delete {
  my ($c, $tag, $hash_tag) = @_;
  $c->delete($tag);
  delete $phantoms{$hash_tag};
}

{
  my ($last_x, $first_y);
  sub move {
    my ($c, $type, $tag, $x, $y) = @_;
    
    my @coords = $c->coords($tag);
    if ($type eq 'click') {
      ($last_x, $first_y) = ($x, $y);
    }
    elsif ($type eq 'phantom') {
      return if $y <= 0 or $x <= 0;
      $coords[0] = $x - int($osize / 2);
      $coords[2] = $x + int($osize / 2);
      $c->coords($tag, @coords);
      $tag =~ /^phantom_(.*)$/;
      $phantoms{$1} = $x - int($osize / 2);
    }
    else {
      my $offset = int($osize / 2);
      my $dx = $x - $last_x;
      $dx /= 5;
      my $m = abs($y - $first_y);
      $m = $m / 100;
      $m ||= 1;

      $dx *= $m;
      $values{$tag} += $dx;
      $values{$tag} = int($values{$tag} * 100) / 100;
      refresh_thing($tag);
      $last_x = $x;
    }
  }
}

##

sub read_sock {
  $client->poll();
}

##

sub on_ctrl {
  my ($control, $value) = @_;
  add_controls($value);
}

##

sub on_clock {
  while(my($tag, $x) = each %phantoms) {
    my @coords = $c->coords($tag);
    if ($x > $coords[0]) {
      $values{$tag} += 0.1;
      my $new_x = refresh_thing($tag);
      if($x < $new_x) {
	&delete($c, "phantom_$tag", $tag);
      }
    }
    elsif($x < $coords[0]) {
      $values{$tag} -= 0.1;
      my $new_x = refresh_thing($tag);
      if($x > $new_x) {
	&delete($c, "phantom_$tag", $tag);
      }
    }
    else {
      &delete($c, "phantom_$tag", $tag);
    }
  }
}

##

sub refresh_thing {
  my ($tag) = @_;
  my @coords = $c->coords($tag);
  my $new_x = int($values{$tag}) - ($osize / 2) + ($scale / 2);
  if ($new_x != $coords[0]) {
    $coords[0] = $new_x;
    $coords[2] = $coords[0] + $osize;
    $c->coords($tag, @coords);
    $client->ctrl_send($tag, $values{$tag});
    $c->itemconfigure("_value_$tag", -text => $values{$tag});
  }
  return $new_x;
}

##

exit 0;
