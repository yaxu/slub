#!/usr/bin/perl -w
use IO::File;
use Time::HiRes qw(sleep);

my $fh = IO::File->new("|./MSG -nobuffer")
#my $fh = IO::File->new("|/slub/old/clients/caudio/bin/caudio")
  or die "couldn't open ./MSG\n";
$fh->autoflush(1);

print $fh "load /slub/samples/stomp/0.wav as foo\n";

while(1) {
  sleep(0.2);
#  system("play /slub/samples/stomp/0.wav");
  print $fh "play foo pitch 86\n";
  print "x\n";
}
