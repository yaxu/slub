#!/usr/bin/perl

use lib "/slub/clients/OSC_perl";
use sendOSC;
use Socket;
use Client;
use strict; 

$| = 1;
my ($host, $port);
if ($#ARGV == -1) {
  # defaults
  print "usage:\n\t$0 hostname portnumber message\n";
  print "\tmessage format: /address,arg1,arg2,arg3,...\n";
  exit;
}
else {
    $host = $ARGV[0];
    $port = $ARGV[1];
}

my $client = Client->new(port      => 6010,
			 timer     => 'ext',
			 tpb       => 2,
			 ctrls     => ['osc'],
			 on_ctrl   => \&on_ctrl,
                         on_clock  => \&on_clock,
                        );

$client->event_loop;

sub on_clock {
    warn("ping $host $port\n");
}

sub on_ctrl {
    my ($ctrl, $value) = @_;
    warn("sending $value\n");
    my @oscarg = &make_sendargv($host, $port);
    push @oscarg, $value;
    sendOSC::HartCoreMode($#oscarg+1, \@oscarg);
}
