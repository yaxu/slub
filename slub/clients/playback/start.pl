#!/usr/bin/perl

use Client;
use strict;

use IO::File;

$| = 1;

my ($time, $skip) = @ARGV;

$skip ||= 0;

my $client = Client->new(port      => 6010,
                         ctrls     => [],
                         on_ctrl   => sub {},
                         on_clock  => sub {},
                        );


my $sock = $client->sock();
my ($control, $value);
$client->poll();


##

my $found = 0;
if ($time) {
    while (1) {
	my $line = <STDIN>;
	last unless defined $line;
	
	if ($line =~ /^\# begins .*?$time/) {
	    ++$found;
	    last;
	}
    }
}
else {
    ++$found;
}

my @commands;
if ($found) {
    while (1) {
	my $line = <STDIN>;
	last if not defined $line;
	if ($line =~ /^\# begins/) {
	    last;
	}
	chomp $line;
	push @commands, $line
	  unless $line =~ /^\#/;
    }
}


warn("found " . scalar(@commands) . " commands.\n");

##

foreach my $command (@commands) {
    if ($skip) {
	my($number) = $command =~ /\@(\d+)/;
	if (defined $number) {
	    if ($number < $skip) {
		next if $command =~ /MSG\|play/;
		$number = 0;
	    }
	    else {
		$number -= $skip;
	    }
	    $command =~ s/^\@\d+/\@$number/;
	}
    }
    $client->write("$command\r\n");
    $client->poll();
}

print("playback load complete.\n");

1;
