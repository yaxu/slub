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

my @controls = qw();
my %values = map {$_ => 0} @controls;

my $main;

init();
MainLoop;

##

sub init {
  $client = Client->new(port      => 6010,
			ctrls     => \@controls,
			on_ctrl   => \&on_ctrl,
			on_clock  => \&on_clock,
			timer     => 'ext',
			tpb       => 4
		       ); 
  my $sock = $client->sock();

  # Create main window and canvas
  $main = MainWindow->new();
  #$main->wm("minsize",  400,300);
  $main->wm("geometry", "${main_width}x${main_height}");
  $main->Button(-command => \&randomise)->pack(-side => 'top');
  

  #my $c = $main->Canvas(-width => $main_width,
  #-height => $main_height
  #);
  #$c->pack(-expand => 1, -fill => 'both');
  
  #$main->bind('<KeyPress-r>', [\&reverse_phrase, $c]);
  #$main->bind('<KeyPress-l>', [\&toggle_ctrl_listen]);
  
  $main->fileevent($client->sock(), readable => \&read_sock);

  init_samples();
 

}

##

sub read_sock {
  $client->poll();
}

##

sub on_ctrl {
  my ($control, $value) = @_;
}

##

{
  my $tick;
  sub on_clock {
    $tick ||= 0;

    while(my($set, $seth) = each %sets) {
      if ($seth->{interval} 
	  and $seth->{size} 
	  and not $tick % $seth->{interval}
	 ) {
	$seth->{pos} = (($seth->{pos}) % $seth->{size});
	$client->play_sample($seth->{samples}->[$seth->{pos}]);
	print "play $seth->{samples}->[$seth->{pos}]\n";
	++$seth->{pos};
      }
    }

    ++$tick;
  }
}

##

sub new_set {
  my ($sampleset, @samples) = @_;

  $sets{$sampleset}->{samples} = \@samples;

  new_set_widgets($sampleset, @samples);
}

##

sub new_set_widgets {
  my ($sampleset, @samples) = @_;

  my $frame = $main->Frame;
  $frame->pack(-side => 'left');
  
  $frame->Label(-text => $sampleset)->pack(-side => 'top');
  $frame->Scale(-from => 0,
	       -to   => scalar(@samples),
	       -sliderlength => 16,
	       -showvalue => 1,
	       -width=> 8,
	       -command => sub {change_set_size($sampleset, $_[0])},
	      )->pack(-side => 'left');

  $frame->Scale(-from => 0,
	       -to   => 15,
	       -sliderlength => 16,
	       -showvalue => 1,
	       -width=> 8,
	       -command => sub {change_set_interval($sampleset, $_[0])},
	      )->pack(-side => 'right');
}

##

sub change_set_interval {
  my($set, $value) = @_;
  $sets{$set}->{interval} = $value;
}

##

sub change_set_size {
  my($set, $value) = @_;
  $sets{$set}->{size} = $value;
}


##

sub init_samples {
  
  foreach my $sampleset (@samplesets) {
    my @samples = $client->init_samples($sampleset);
    new_set($sampleset, @samples);
  }
}

##

sub randomise {
  foreach my $set (values %sets) {
    fisher_yates_shuffle($set->{samples});
  }
}

##

sub fisher_yates_shuffle {
  my $array = shift;
  my $i;
  for ($i = @$array; --$i; ) {
    my $j = int rand ($i+1);
    @$array[$i,$j] = @$array[$j,$i];
  }
}
