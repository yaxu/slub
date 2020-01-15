#!/usr/bin/perl

use strict;

use lib '/yaxu/lc/lib';

use Net::OSC;
use Time::HiRes qw/sleep gettimeofday/;

my $bpm = 0;

my $tickport = 6010;

my $osc = Net::OSC->new;
$osc->server(hostname => 'localhost', port => $tickport);
$osc->command('tick');

my $listen_handle = 
  IO::Socket::INET->new(Proto     => 'udp',
			LocalAddr => 'localhost',
			PeerAddr  => 'localhost',
			PeerPort  => $tickport,
			Blocking  => 0,
		       )
  or die "socket: $@";

my $tps = 0.5;
my $start = gettimeofday;
my $next_tick = $start + 2;
my $ticks = 0;

while (1) {
    if (($next_tick - gettimeofday()) <= 0) {
        $osc->data([['s', sprintf("%.6f", $next_tick)], 
		    ['i', $tps], 
		    ['i', $ticks]
		   ]
		  );
	$osc->send_message;
	++$ticks;
	$next_tick += $tps;
    }
    #my $data;
    #if ($listen_handle->read($data, 1024)) {
    #    print("got: " . length($data) . " bytes.\n");
    #}
    sleep($next_tick - gettimeofday());
}
