#!/usr/bin/perl

use Client;
use strict;
use Data::Dumper;


my $data;
foreach my $line (<STDIN>) {
    chop $line;
    $data .= $line;
}

my $client = Client->new(port      => 6010,
			 on_clock  => \&on_clock,
			 timer     => 'ext',
			 tpb       => 1
			);
my @samples = map {$client->init_samples($_)} @ARGV;
$client->event_loop;

  
sub on_clock {
  $data =~ s/^(.)//;
  my $sample = (($1 eq 'o') ? $samples[-3] : $samples[-8]);
  $client->send('MSG', "play $sample");
  
  print(length($data) . "$1\n");
  if (not length $data) {
      exit 0;
  }
}
