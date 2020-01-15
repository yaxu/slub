#!/usr/bin/perl

use Client;
use strict;

my @ctrls = qw{intensity};
my %ctrl_vals = map {$_ => 0} @ctrls;
my $client;

STDOUT->autoflush(1);

my $sampleset = (shift || 'click');
my $pitch = (shift || 50);
my $seed = (shift || (time ^ ($$ + ($$ << 15))));
srand($seed);
print "Playing loop $seed\n";
my @samples;

start();

sub start {
  
  $client = Client->new(port      => 6010,
			ctrls     => \@ctrls,
			on_ctrl   => \&on_ctrl,
			on_clock  => \&on_clock,
			timer     => 'ext',
			tpb       => '8',
		       );
  $client->init_samples($sampleset);
  @samples = map {$client->rand_event($pitch)} ( 0 .. 3 );
  $client->event_loop;
}

##

sub on_ctrl {
  my ($control, $value) = @_;
  #print "Control: $control Value $value,\n";
  $ctrl_vals{$control} = $value;
}

##
my $beats;
my $foo;
sub on_clock {
  ++$beats;
  if (not $beats % 32) {
    ++$foo;
  }

  #if (not $beats % 128) {
  #    $client->ctrl_send('note', "35, 10, 127");
  #}

  #if (not $beats % 16) {
  #$client->ctrl_send('note', "35, 10, 100");
  #}
  
  if (not $foo % 4) {
    if (not $beats % 4) {
      $samples[0]->();
    }
    elsif (not $beats % 3) {
      $samples[1]->();
    }
  }
  if (not $foo % 3) {
    if (not $beats % 5) {
      $samples[2]->();
    }
  }
  if (not $foo % 5) {
    if (not $beats % 3) {
      $samples[3]->();
    }
  }
}




