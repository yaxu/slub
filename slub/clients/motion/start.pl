#!/usr/bin/perl -w

use strict;
use Client;
use IO::File;


my $client = Client->new(port      => 6010);
chdir '/slub/clients/motion';
my $fh = IO::File->new("./motion -v -s 320x240 -d /dev/video0|")
  or die "couldn't open motion $!\n";

while (my $line = <$fh>) {
  if ($line =~ /Motion detected (\d+) (\d+x\d+)/) {
    $client->ctrl_send('coords', $2);
    $client->poll();
  }
}
