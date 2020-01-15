package C;

use strict;

use IO::Socket;
use IO::Select;
use Time::HiRes qw{gettimeofday tv_interval sleep };

use Data::Dumper;


use Class::MethodMaker
  new_with_init => 'new',
  new_hash_init => 'hash_init',
  get_set       => [qw{ current_tick started buffer sock seq subs samples }];

sub init {
    my $self = shift;
    $self->hash_init(@_);

    $self->init_values;
    $self->init_sock();
    $self->_init_timer;
    $self->_reg_ctrls();

    return $self;
}


sub init_values {
    my $self = shift;
    $self->current_tick(0);
    $self->started(0);
    $self->buffer('');
    $self->seq(0);
    $self->subs({});
    $self->samples([]);
}


##

sub on_clock {
    warn("override");
}

##

sub on_ctrl {
    warn("override");
}

##

sub port        { 6020 }
sub ctrls       {   [] }
sub bpm         {  120 }
sub tpb         {   16 }

##

sub start_mod   { $_[0]->{start_mod} ||= $_[0]->tpb * 2 }
sub select      { $_[0]->{select} ||= IO::Select->new }

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
    $self->sock($s);
}

##

sub _init_timer {
    my $self = shift;

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

##

sub _reg_ctrls {
    my $self = shift;
    my $fh = $self->sock;

    foreach my $ctrl (@{$self->ctrls}) {
	$self->write("ctl_create|$ctrl\r\n",
		     sub {
			 my ($seq, $status, $human, @data) = @_;
			 if ($status != 200) {
			     die "Failed to create control '$ctrl': $human\n";
			 }
		     }
		    );
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

sub write {
    my($self, $stuff, $sub) = @_;
    my $seq = $self->seq;
    ++$seq;
    syswrite($self->sock, "#$seq|$stuff");
    if ($sub) {
	$self->subs->{$seq} = $sub;
    }
    $self->seq($seq);
}

##

sub read {
    my ($self) = @_;

    my $subs = $self->subs;
    my $seq = $self->seq;

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
	    if (defined $subs->{$seq}) {
		$subs->{$seq}->($seq, $status, $human, @data);
		delete $subs->{$seq};
	    }
	}

	#warn("got status: $status\n");
	if ($status == 600) {
	    $self->process_ctrl(@data);
	}
	elsif ($status == 701) {
	    $self->current_tick($data[0]);
	    $self->on_clock->(@data);
	}
	elsif ($status != 200 and $status != 000) {
	    print "Well there's a strange thing: $seq, $status, $human, [$message]\n";
	}
    }
    return;
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
	$self->on_ctrl($control, @data);
    }
}

##

sub event_loop {
    my $self = shift;

    my $select = $self->select;

    while (1) {
	if ($select->can_read(undef)) {
	    $self->read();
	}
    }
}

##

sub poll {
  $_[0]->read() if $_[0]->select->can_read(0);
}

##

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

    my $current_tick = $self->current_tick;
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

    my $current_tick = $self->current_tick;
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

sub init_samples {
    my ($self, $sampleset, $extra_samples) = @_;
    my $sample_dir = "/slub/samples/$sampleset";

    my @l_samples;

    opendir(DIR, $sample_dir) || die "can't opendir $sample_dir: $!";
    my @files = grep { -f "$sample_dir/$_" } readdir(DIR);
    closedir DIR;
	
    my $inc = 0;
	
    $self->send("MSG", "# will you work");
	
    foreach my $file (sort @files) {
	next if not $file =~ /wav$/i;
	my $as = "${sampleset}$inc";
	$self->send("MSG", "load $sample_dir/$file as $as");
	push @l_samples, $as;
	$inc++;
    }

    while (my($as, $file) = each(%$extra_samples)) {
	next if not $file =~ /wav$/i;
	$self->send("MSG", "load $file as $as");
	push @l_samples, $as;
	$inc++;
    }
	
    push @{$self->samples}, @l_samples;
    return @l_samples;
}

##

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
    if (@_) {
	$self->send_offset($_[0], 'MSG', $msg);
    }
    else {
	$self->send('MSG', $msg);
    }
}

##

sub rand_event {
    my ($self, $pitch) = @_;
    my $rand_pitch = $pitch + ((rand(16))-8);
    my $samples = $self->samples;
    my $sample = $samples->[rand @$samples];
    return(sub {
	       $self->send("MSG", 
			   'play ' . $sample
			   . ' pitch '
			   . $rand_pitch
			  )
	   }
	  );
}

##

1;
