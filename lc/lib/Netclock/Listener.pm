package Netclock::Listener;

use strict;

use threads;
use threads::shared;

use Net::OpenSoundControl::Client;
use Net::OpenSoundControl::Server;
use Netclock::Time;

use YAML;

sub new {
  my ($pkg, $self) = @_;
  $self ||= {}
    unless $self;
  bless($self, $pkg);
  $self->{server_ip} ||= '127.0.0.1';
  $self->{server_port} ||= '57120';
  unless (exists $self->{name}) {
    my $hostname = `hostname`;
    chomp($hostname);
    $self->{name} = "perl-$hostname-$$";
  }
  
  $self->{client} = 
    Net::OpenSoundControl::Client->new(
				       Host => $self->{server_ip}, 
				       Port => $self->{server_port}
				      )
	or die "Could not start client: $@\n";
  return($self);
}

sub sendTempo {
  my ($self, $numRelBeats, $bps) = @_;
  $self->{client}->send(['/clock/changeTempo',
			 's',
			 $self->{name},
			 'i',
			 $numRelBeats,
			 'f',
			 $bps
			]
		       );
}

sub register {
  my ($self, $local_port) = @_;
  
  my $client = 
    Net::OpenSoundControl::Client->new(
				       Host => $self->{server_ip}, 
				       Port => $self->{server_port}
				      )
	or die "Could not start client: $@\n";
  $client->send(['/clock/register',
		 's',
		 $self->{name},
		 's',
		 '127.0.0.1',
		 #'158.223.59.84',
		 'i',
		 $local_port,
		 'i',
		 1 # sync to next beat
		]
	       );
}

sub incoming {
  my ($queue, $sender, $message) = @_;

  my $timestamp;
  #warn "Got message: [$sender] " . Dump($message) . "\n";
  if ($message->[0] eq '#bundle' and $message->[2]->[0] =~ /\/clock\/sync$/) {
    print(STDERR "Oh!!\n");
    lock(@$queue);
    my $change = [];
    share($change);
    push(@$change, ($message->[1],  # time
		    $message->[2]->[2], # bps
		    $message->[2]->[4], # changebeats
		    $message->[2]->[6]  # latency
		   )
	);
    push(@$queue, $change);
    # sort by changebeat
    @$queue = sort {$a->[2] <=> $b->[2]} @$queue;
  }
}

sub tpmListen {
    my ($self, $queue) = @_;
    my $serv;
    my $port;
    foreach my $search (5151) {
	$serv = 
	    Net::OpenSoundControl::Server->new(Handler => 
					       sub {incoming($queue, @_)},
					       Port => $search
					      );
	if($serv) {
	  $port = $search;
	  last;
	}
    }
    die "couldn't listen on a udp port in range 5000-5999" unless $port;
    
    $self->register($port);
    $serv->readloop();
}



sub clocked {
  my ($self, $tpb, $f) = @_;
  
  my @queue : shared;
  my $tpm = Netclock::Time->new({tpb => $tpb});

  threads->create(sub{$self->tpmListen(\@queue)});
  
  while (! @queue) {
    sleep(0.1);
  }
  
  $tpm->update(\@queue);
  $tpm->waitNext();
  while (1) {
    $f->($tpm->{ticks});
    $tpm->update(\@queue);
    $tpm->tick();
  }
}

##

1;
