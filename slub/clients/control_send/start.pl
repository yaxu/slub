#!/usr/bin/perl

use Client;
use strict;

$| = 1;

my($ctl) = shift @ARGV;

die "no control given" unless $ctl;
my $client = Client->new(port      => 6010,
                         ctrls     => [],
                         on_ctrl   => sub {},
                         on_clock  => sub {},
                        );


my $sock = $client->sock();
my ($control, $value);
$client->poll();

if ($ctl eq 'bpm' and @ARGV) {
   syswrite($sock, "timer|$ARGV[0]\r\n");
   syswrite($sock, "timer|$ARGV[0]\r\n");
   exit;
}

print "[$ctl] ";
while(<STDIN>) {
  print "[$ctl] ";
  chop;
  $value = $_;
  if ($ctl eq 'bpm') {
    syswrite($sock, "timer|$value\r\n");
  }
  else {
    $client->ctrl_send($ctl, $value);
  }
  $client->poll();
}



1;
