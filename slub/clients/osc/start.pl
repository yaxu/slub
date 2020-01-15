#!/usr/bin/perl
# dumpOSC emulator example script
# usage: ./dumpOSC.pl 5151
# 200207, jdl@xdv.org

use lib "/slub/clients/OSC_perl";
use dumpOSC;
use Socket;
use Client;

$| = 1;

my $client = Client->new(port      => 6010,
                         ctrls     => [],
                         on_ctrl   => sub {},
                         on_clock  => sub {},
                        );


my $sock = $client->sock();
my ($control, $value);
$client->poll();


($port) = @ARGV;
$port = 55023 unless $port;
$MAXLEN = 1024;
$newmsg = "";

$sock = UDPsrv_open($port);

while ($peeraddr = recv($sock, $newmsg, $MAXLEN, 0)) {
  if($newmsg ne "") {
    ($peerport, $peerip) = sockaddr_in($peeraddr);
    $newmsglen = length $newmsg;
    $peerhost = inet_ntoa($peerip);
    print "rcvd: packet from: $peerhost:$peerport, length: $newmsglen\n";
    %OSCi = &ParseOSC($newmsg,$newmsglen,);
    print "rcvd: message: ",$OSCi{addr}," ";
    $client->ctrl_send('pedro', $OSCi{addr});
    foreach $val (split /,/,$OSCi{vals}) {
      print $val," ";
    }
    print "\n";
  }
}

close($sock);


sub UDPsrv_open {
  my($port) = @_;
  my $sockaddr = 'S n a4 x8';

  ($name, $aliases, $proto) = getprotobyname('udp');
  if ($port !~ /^\d+$/) {
    ($name, $aliases, $port) = getservbyport($port, 'udp');
  }

  print "Port = $port\n";

  my $this = pack($sockaddr, AF_INET, $port, "\0\0\0\0");

  socket(S, AF_INET, SOCK_DGRAM, $proto) || die "socket: $!";
  bind(S,$this) || die "bind: $!";
  return S;
}
