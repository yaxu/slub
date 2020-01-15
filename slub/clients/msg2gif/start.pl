#!/usr/bin/perl -w
use strict;

use GD;

use Client;

my $client;

use IO::File;
my @controls = qw(MSG);

my @samples;

my $imgsize = 512;
my $pixels = $imgsize * $imgsize;
my $im = new GD::Image($imgsize, $imgsize);

my @colours = map {$im->colorAllocate($_,$_,$_)} (0, 255);

my $log = IO::File->new("/tmp/msg-log", "w");

init();

##

sub init {
  $client = Client->new(port      => 6010,
			ctrls     => \@controls,
			on_ctrl   => \&on_ctrl,
			on_clock  => \&on_clock,
			timer     => 'ext',
			tpb       => 16
		       ); 

  $client->event_loop;
}

##

my ($x, $y, $foo);
sub on_clock {
  $x ||= 0;
  $y ||= 0;

  print "pixels: " . $pixels-- . "\n";

  $im->setPixel($x, $y, $colours[$foo || 0]);
  
  if (++$x >= $imgsize) {
    ++$y; $x = 0;
    if ($y >= $imgsize) {
      my $fh = IO::File->new("/tmp/msg2-$$.gif", "w");
      
      print $fh $im->gif;
      close $fh;
      exit;
    }
  }

  $foo = 0;
}

##

sub on_ctrl {
  my ($control, $value) = @_;
  if($value =~ /play/) {
    print $log $value;
    $foo = 1;
  }
}

##

1;

