#!/usr/bin/perl -w

use strict;
use Client;
use IO::File;

#my $extra = '-record';

my $extra = '-record' if $ARGV[0] and $ARGV[0] eq '-r';

my @ctrls = qw{caudio msg MSG};

my $client = Client->new(port      => 6010,
			 ctrls     => \@ctrls,
			 on_ctrl   => \&on_ctrl,
			 on_clock  => \&on_clock,
			 tpb       => 16,
			 timer     => 'ext'
			);
chdir 'MSG';
my $fh = IO::File->new("|nice -n -15 ./MSG $extra -silent -nobuffer")
  or die "couldn't open ./caudio\n";
$fh->autoflush(1);
$client->event_loop;

##
my $data;
my $foo;
sub on_clock {
    my ($control, @data) = @_;
    $foo++;
    $foo = $foo % 32;
    print( (' ' x $foo) . "x\n") if $data;
    #  print $fh "flush\n" if $data;
    $data = 0;
}

sub on_ctrl {
    my ($control, @data) = @_;
    foreach my $datum (@data) {
	print($fh ($datum . "\n"));
	++$data;
    }
}

