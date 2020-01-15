#!/usr/bin/perl

use strict;
use Client;
use IO::File;

my @ctrls = qw{sin};

my $client = Client->new(port      => 6010,
			 ctrls     => \@ctrls,
			 on_ctrl   => \&on_ctrl,
			);
chdir 'sin_proxy/caudio';
my $fh = IO::File->new("|./caudio")
  or die "couldn't open ./caudio\n";
$fh->autoflush;
$client->event_loop;

##

sub on_ctrl {
  my ($control, $value) = @_;
  print $fh "$value\n";
}

