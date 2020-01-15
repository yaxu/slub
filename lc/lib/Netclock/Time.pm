package Netclock::Time;

use strict;

use Time::HiRes qw/ sleep time /;

sub new {
    my ($pkg, $hash) = @_;
    $hash ||= {};
    $hash->{started} = 0;
    $hash->{tpm} = undef;
    $hash->{change_time} = undef;
    $hash->{change_ticks} = undef;
    $hash->{ticks} = 0;
    $hash->{tpb} ||= 8;
    
    return(bless($hash, $pkg));
}

sub waitNext {
    my $self = shift;
    my $logicalNow = 
	$self->{change_time} 
	  + ((60.0 / $self->{tpm}) 
	     * (($self->{ticks} - $self->{change_ticks}
		)
	       )
	    );
    $self->{now} = $logicalNow;
    my $realNow = time();
    my $diff = $logicalNow - $realNow;
    if ($diff > 0) {
      sleep($diff);
    }
}

sub tick {
  my $self = shift;
  $self->{ticks}++;
  $self->waitNext();
}

sub update {
  my ($self, $queue) = @_;
  lock @$queue;
  while ((@$queue and (($queue->[0]->[2] * $self->{tpb}) <= $self->{ticks}) or (not $self->{started}))) {
    my ($change_time, $bps, $change_ticks, $latency) = @{shift @$queue};
    #$self->{latency} = $latency;
    $self->{change_time} = $change_time + $self->{latency};
    $self->{tpm} = $bps * 60 * $self->{tpb};
    $self->{change_ticks} = $change_ticks * $self->{tpb};
    if (not $self->{started}) {
      $self->{ticks} = $self->{change_ticks};
      $self->{started} = 1;
    }
  }
}

##

1;
