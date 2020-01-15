#!/usr/bin/perl -w



use strict;
use Tk;
use Tk::Canvas;
use IO::Handle;
use Client;
use strict;

my $height = 600;
my $width = 800;

my $client = Client->new(port      => 6010,
                         ctrls     => ['vis0_new', 'vis0_move'],
                         on_ctrl   => \&on_ctrl
                        );

my $sock = $client->sock();

# Create main window and canvas
my $main = MainWindow->new(-width => $width,
			   -height => $height
			  );
my $c = $main->Canvas;
$c->pack(-expand => 1, -fill => 'both');

$main->fileevent($client->sock(), readable => \&read_sock);

MainLoop;

sub read_sock {
  $client->poll();
}

sub on_ctrl {
  my ($control, @ids) = @_;
  
  if ($control eq 'vis0_new') {
    new(@ids);
  }
  elsif ($control eq 'vis0_move') {
    eval {
      move(@ids);
    };
    if ($@) {
      warn $@;
    }
 }
}

sub new {

  my ($new_tag, $from_tag) = @_;
  my ($x, $y) = rand_position(($new_tag =~ /^close/ ? 'close' : 'far'), 
			      $from_tag
			     );

  $c->create(('oval', ($x-2), ($y-2),
		($x+2), ($y+2)
	       ),
	       -tags    => [$new_tag, 'movers'],
	       -fill    => 'white',
	       -outline => 'black'
	      );
  if ($from_tag) {
    my ($x1, $y1, $x2, $y2) = $c->coords($from_tag);
    $c->create(('line', 
		$x1+(($x2-$x1)/2), $y1+(($y2-$y1)/2),
		$x, $y
	       ),
	       -tags    => ["_line_$new_tag", 'line'],
	       -fill    => 'black',
	      );
  }
}

sub move {
  my ($mover_tag, $static_tag) = @_;
  
  if(not $c->find('withtag', $mover_tag)) {
    my ($x, $y) = rand_position('far');
    $c->create(('oval', ($x-3), ($y-3),
		  ($x+3), ($y+3)
		 ),
		 -tags    => [$mover_tag, 'movers'],
		 -fill    => 'black',
		 -outline => 'black'
		);
  }
  
  $c->coords($mover_tag, $c->coords($static_tag));
  $c->raise($mover_tag, $static_tag);
}

sub rand_position {
  my ($type, $close_tag) = @_;
  if ($close_tag) {
    my $distance = ($type eq 'close'
		    ? 5
		    : 2
		   );
    my ($x1, $y1, $x2, $y2) = $c->coords($close_tag);
    my $x = ($x1+(($x2-$x1)/2));
    my $y = ($y1+(($y2-$y1)/2));
    
    my $offset = (($width  / ($distance * 2)) - rand($width  / $distance));
    warn('offsetx: ', $offset);
    $x += $offset;

    $offset    = (($height / ($distance * 2)) - rand($height / $distance));
    warn('offsety: ', $offset);
    $y += $offset;

    if ($x > $width) {
      $x = ($width - ($x - $width));
    }
    if ($y > $height) {
      $y = ($height - ($y - $height));
    }
    $x = abs $x;
    $y = abs $y;
    
    return($x, $y);
  }
  else {
    return(int(rand($width)), int(rand($height)));
  }
}

