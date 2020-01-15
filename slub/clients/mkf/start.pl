#!/usr/bin/perl -w

use strict;
use Tk;
use Tk::Canvas;
use IO::Handle;
use Client;

use Data::Dumper;

my $gridy = 1;

my $main_height = 158;
my $main_width = 300;

my $client;

my $last_phrase_move;

my @samplesets = @ARGV;
my %sets;

my $line_color = 'grey';
my $active_line_color = 'white';

my %values;

my $main;

init();
MainLoop;

##

sub init {
  $client = Client->new(port      => 6010);
  my $sock = $client->sock();

  # Create main window and canvas
  $main = MainWindow->new();
  #$main->wm("minsize",  400,300);
  $main->wm("geometry", "${main_width}x${main_height}");
  
  $main->fileevent($client->sock(), readable => \&read_sock);

  make_widgets();
}

##

sub read_sock {
  $client->poll();
}

##


sub make_widgets {

  my $frame = $main->Frame;
  $frame->pack(-side => 'top');
  $frame->Label(-text => 'new')->pack(-side => 'left');
  $frame->Button(-command => \&new)->pack(-side => 'right');

  foreach my $thing (qw/dispersal friendly strength racism type/) {
      my $frame = $main->Frame;
      $frame->pack(-side => 'top');
      $frame->Label(-text => $thing)->pack(-side => 'left');
      $frame->Scale(-from => 0,
		    -to   => 32,
		    -sliderlength =>8,
		    -showvalue => 1,
		    -width=> 8,
		    -length => 50,
		    -command => sub {change_set($thing, $_[0])},
		    -orient => 'horizontal',
		    -resolution => 1,
		   )->pack(-side => 'right')->set('1');
  }
}


sub change_set {
    my ($thing, $value) = @_;
    $values{$thing} = $value;
}

sub new {
    $client->ctrl_send('feep_new', %values);
}
