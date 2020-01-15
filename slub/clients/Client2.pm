package Client2;

=head1 NAME

Client

=head1 DESCRIPTION

A module for easing communication with the bug

(c) Slab Laboratories, 2000  Released under the Gnu Public License.

=cut

use IO::Socket;
use IO::Select;
use Time::HiRes qw{gettimeofday tv_interval sleep};
use strict;

use Data::Dumper;

=head1 FUNCTIONS

=over 4

=item B<new> - object constructor

Pass a named parameter list:

=over 4

=item port 

The port number to connect to.

=cut


my $current_tick;

sub new {
  my ($pkg, %p) = @_;
  
  my $self = {};
  bless $self, $pkg;
  
  $self->init(\%p);
  
  $self->init_sock();
  $self->_init_timer;
  $self->_reg_ctrls();
  
  if ($ENV{GROUP}) {
    $self->send("MSG", "addgroup $ENV{GROUP}");
  }
  return $self;
}

##

sub init {
  my ($self, $p) = @_;
  
  $self->{_port}      = (6020                        );
  $self->{_ctrls}     = ($p->{ctrls}     || []                          );
  $self->{_on_ctrl}   = ($p->{on_ctrl }  || undef                       );
  $self->{_on_clock}  = ($p->{on_clock}  || undef                       );
  $self->{_timer}     = ($p->{timer}     || 0                           );
  $self->{_bpm}       = ($p->{bpm}       || 120                         );
  $self->{_tpb}       = ($p->{tpb}       || 16                          );
  $self->{_start_mod} = ($p->{start_mod} || ($self->{_tpb} * 2)         );
  $self->{_started}   = 0;
  $self->{_buffer}    = '';
  $self->{_select}    = IO::Select->new();
}

##

sub init_sock {
  my $self = shift;
  
  my $s;
  while (not $s) {
    $s = IO::Socket::INET->new(PeerAddr => 'localhost',
			       PeerPort => $self->port,
			       Proto    => 'tcp',
			       Reuse    => 1
			      );
    sleep 2;
  }

  $self->select->add($s);
  $self->{_sock} = $s;
}

##

sub _init_timer {
  my $self = shift;
  
  return if not $self->on_clock;
  
  if ($self->timer() eq 'ext') {
    my $fh = $self->sock;
    my $start_mod = $self->start_mod;
    my $tpb = $self->tpb;
    $self->write("timer_listen|$start_mod|$tpb|0\r\n",
		 sub {
		   my($seq, $status, $human, @data) = @_;
		   if ($status != 200) {
		     die "timer_listen failed: $human\n";
		   }
		 }
		);
  }
}

##

sub _reg_ctrls {
  my $self = shift;
  my $fh = $self->sock;
  
  foreach my $ctrl (@{$self->ctrls}) {
    #$self->write("ctl_create|$ctrl\r\n",
#		 sub {
#		   my ($seq, $status, $human, @data) = @_;
#		   if ($status != 200) {
#		     die "Failed to create control '$ctrl': $human\n";
#		   }
#		 }
#		);
    $self->write("ctl_listen|$ctrl\r\n",
		 sub {
		   my ($seq, $status, $human, @data) = @_;
		   if ($status != 200) {
		     die "Failed to register control '$ctrl': $human\n";
		   }
		 }
		);
  }
}

##

{
  my $seq;
  my %subs;
  sub write {
    my($self, $stuff, $sub) = @_;
    ++$seq;
    syswrite($self->sock, "#$seq|$stuff");
    if ($sub) {
      $subs{$seq} = $sub;
    }
  }

  ##

  sub read {
    my ($self) = @_;
    my $messages = $self->buffer_read($self->sock);
    if (not defined $messages) {
      # Server has gone away/mad
      die "Erk";
    }
    if (@$messages == 0) {
      # Partial message read, or something
      return;
    }
    
    my ($status, $human, @data);
    foreach my $message (@$messages) {
      ($seq, $status, $human, @data) = parse($message);
      if ($seq) {
	if (defined $subs{$seq}) {
	  $subs{$seq}->($seq, $status, $human, @data);
	  delete $subs{$seq};
	}
      }
      #warn("got status: $status\n");
      if ($status == 600) {
	$self->process_ctrl(@data);
      }
      elsif ($status == 701) {
	$current_tick = $data[0];
	$self->on_clock->(@data);
      }
      elsif ($status != 200 and $status != 000) {
	print "Well there's a strange thing: $seq, $status, $human, [$message]\n";
      }
    }
    return;
  }
}

##

sub digest_clock {
  my ($self, @data) = @_;
  return {tick      => $data[3],
	  beat_tick => $data[4],
	  tpb       => $data[5],
	  bpm       => $data[6]
	 };
}

##

sub process_ctrl {
  my ($self, $control, @data) = @_;
  if (not (defined($control) and @data)) {
    warn "Received a badly formed control, ignoring.."
  }
  else {
    &{$self->on_ctrl}($control, @data);
  }
}

##

=head1 METHODS

=over 4

=item B<event_loop>

The main client event loop.  This method does not return control.

=cut

sub event_loop {
  my $self = shift;
  
  my $select = $self->select;
  
  my $start = gettimeofday;
  my $interval = ((60 / $self->bpm) / $self->tpb);
  my(@ready, @r, $now, $seconds_in, $sleep);
  
  my $timer = $self->timer;
  my $ext_timer = ($timer eq 'ext');
  my $int_timer = ($timer eq 'int');
  my $ticks = 0;

  while (1) {
    @ready = $select->can_read($int_timer
			       ? 0
			       : undef
			      );
    
    $self->read() if @ready;

    if ($int_timer) {
      ++$ticks;
      $seconds_in = ($start + ($ticks * $interval));
      $now = gettimeofday();
      $sleep = ($seconds_in - $now);
      
      sleep($sleep) if ($sleep > 0);
      
      # select(undef, undef, undef, $sleep) if ($sleep > 0);
      &{$self->on_clock()};
    }
  }
}

##

sub poll {
  $_[0]->read() if $_[0]->select->can_read(0);
}

sub parse {
  my $message = shift;
  #print "parsing '$message'\n";

  my @data = (split(/(?<!\\)\|/, $message));
  if(not @data) {
    warn "no data!";
    return;
  }
  my $seq;
  if ($data[0] =~ /^\#/) {
    $seq = shift @data;
  }
  my $status = shift @data;
  my $human = shift @data;
  
  warn "Argh" unless $status;
  return($seq, $status, $human, @data);
}

##

sub buffer_read {
  my ($self, $fh) = @_;
  
  my $buf;
  my @result;  
  
  my $bytes = sysread($fh, $buf, 1024);
  $buf = $self->{_buffer} . $buf;
  
  if ($bytes == 0) {
    # Server has gone away
    return undef;
  }
  
  while ($buf =~ s/^(.+?)\r\n//s) {
    push(@result, $1);
  }
  $self->{_buffer} = $buf;
  
  return(\@result);
}

##

sub ctrl_send {
  my($self, @data) = @_;
  
  $self->send('ctl_send', @data);
}

##

sub ctrl_send_offset {
  my($self, $offset, @data) = @_;

  $self->send_offset($offset, 'ctl_send', @data);
}

##

sub send {
    my($self, @data) = @_;
    
    $current_tick ||= 0;
    my $send = join('|', "\@$current_tick", @data);
    $send .= "\r\n";
    $self->write($send,
		 sub {
		     my($seq, $status, $human, @data) = @_;
		     if ($status != 200) {
			 die "$data[0] failed: $human\n";
		     }
		 }
		);
}

##

sub send_offset {
  my($self, $offset, @data) = @_;
  $current_tick ||= 0;
  my $send = join('|', ("\@" . ($current_tick + $offset)), @data);
  $send .= "\r\n";

  $self->write($send,
	       sub {
		 my($seq, $status, $human, @data) = @_;
		 if ($status != 200) {
		   die "$data[0] failed: $human\n";
		 }
	       }
	      );
}

##

{
  my @samples;

  sub samples {
    @samples;
  }

  sub init_samples {
    my ($self, $sampleset, $extra_samples) = @_;
    my $sample_dir = "/slub/samples/$sampleset";
    
    my @l_samples;

    opendir(DIR, $sample_dir) || die "can't opendir $sample_dir: $!";
    my @files = grep { -f "$sample_dir/$_" } readdir(DIR);
    closedir DIR;
    
    $self->send("MSG", "# will you work");

    foreach my $file (sort @files) {
      next if not $file =~ /wav$/i;
      my $as = $file;
      $as =~ s/\.wav$//i;
      $as = $sampleset . '_' . $as;
      $self->send("MSG", "load $sample_dir/$file as $as");
      push @l_samples, $as;
    }

    while (my($as, $file) = each(%$extra_samples)) {
      next if not $file =~ /wav$/i;
      $self->send("MSG", "load $file as $as");
      push @l_samples, $as;
    }

    push @samples, @l_samples;
    return @l_samples;
  }

  sub play_sample {
    my $self = shift;
    my $sample = shift;
    my $msg = "play $sample";
    if (not @_) {
      @_ = (60);
    }
    foreach my $p (qw(pitch cutoff vol pan res)) {
      last if (not @_);
      
      $msg .= " $p " . shift @_;
    }
    if ($ENV{GROUP}) {
        $msg .= " in $ENV{GROUP}";
    }
    if (@_) {
      $self->send_offset($_[0], 'MSG', $msg);
    }
    else {
      $self->send('MSG', $msg);
    }
  }

  sub rand_event {
    my ($self, $pitch) = @_;
    my $rand_pitch = $pitch + ((rand(16))-8);
    my $sample = $samples[rand @samples];
    return(sub {
	     $self->send("MSG", 
			 'play ' . $sample
			 . ' pitch '
			 . $rand_pitch
			)
	   }
	  );
  }
}

##
# Accessors

sub port      { $_[0]->{_port      } }
sub select    { $_[0]->{_select    } }
sub sock      { $_[0]->{_sock      } }
sub ctrls     { $_[0]->{_ctrls     } }
sub on_ctrl   { $_[0]->{_on_ctrl   } };
sub on_clock  { $_[0]->{_on_clock  } };
sub timer     { $_[0]->{_timer     } }
sub bpm       { $_[0]->{_bpm       } };
sub start_mod { $_[0]->{_start_mod } };
sub tpb       { $_[0]->{_tpb       } }; # ticks per beat

##

1;
