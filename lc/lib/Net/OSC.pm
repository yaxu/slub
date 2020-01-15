package Net::OSC;

=head1 NAME

Net::OSC - interface to Open Sound Control

=head1 SYNOPSIS

  my $osc = Net::OSC->new;

  $osc->server(hostname => 'localhost', port => 57120);

  $osc->command(["play"]);

  $osc->data(['jazz', ['i', 60], ['i', 70]]);

=head2 DESCRIPTION

Net::OSC is an object oriented style interface to the Open Sound
Control (OSC) protocol. 

Currently it only supports transmission over UDP, which is the most
common means of sending OSC over a network.  It only sends OSC
messages, it doesn't receive them yet.

OSC is a protocol for communication among computers, sound
synthesizers, and other multimedia devices that is optimized for
modern networking technology.

It can be used to communicate with a great deal of music software
including SuperCollider and MAX/MSP, as well as hardware devices.

More information can be found at the OSC homepage here:
http://www.cnmat.berkeley.edu/OpenSoundControl/

This is an early release of Net::OSC, later versions may have a
different interface.

=cut

use strict;
use warnings;
use IO::Socket;

use constant
  DEFAULT_PORT => 57110;


=head2 CONSTRUCTOR

  Net::OSC->new()

Nice and simple.

=cut

sub new {
    my($pkg) = @_;
    my $self = bless({}, $pkg);
}

=head2 METHODS

=over 5

=item $osc->server(hostname => HOSTNAME, port => PORT)

Set the remote host and port.  The hostname must be supplied, give
'localhost' if you are sending the OSC packets locally.

The port default to 57110, which I think is the standard port for
SuperCollider at least.

=cut

sub server {
    my ($self, %p) = @_;
    
    my $hostname = $p{hostname};
    my $port     = $p{port};

    if ($hostname) {
	$port ||= DEFAULT_PORT;
	my $addr = sockaddr_in($port, inet_aton($hostname));
	$self->{to_addr} = $addr;
    }

    return($self->{to_addr});
}

##

=item $osc->command(COMMAND)

You can specify the OSC command to be sent like this:

  $osc->command('/foo/bar');

... or like this:

  $osc->command(['foo', 'bar']);

... or like this:

  $osc->command('foo/bar');

It's all the same, although the first is probably slightly more
efficient.

=cut

sub command {
    my $self = shift;
    if (@_) {
      my $command = shift;
      if (ref($command) eq 'ARRAY') {
	$command = join('/', @$command);
      }
      
      if (not substr($command, 0, 1) eq '/') {
	$command = '/' . $command;
      }
      
      $self->{command} = pad_string($command);
    }

    return($self->{command});
}

=item $osc->data(DATA])

This is where you set the parameters for your OSC message.

You'll need to pass it an arrayref, with each element representing one
datum or parameter.

You have to specify the data type of each parameter, or accept the
default of string.

To send the string 'foo', you can do this:

  $osc->data(['foo']);

You can explicitly state the data type for a string (represented by
's'), rather than accept the default.  This would send the string
'foo':

  $osc->data([['s', 'foo']]);

Right now the only other data type that this module supports is an
integer (represented by 'i').  You can send one of those like this:

  $osc->data([['i', 242451]]);

You can of course mix and match these:

  $osc->data([['s', 'beep'], ['i', 643], ['i', 463], 'parp']);

... which would send the string 'beep', followed by the integer '643',
another integer '463' and finally the string 'parp'.

=cut

##

sub data {
    my $self = shift;
    if (@_) {
        $self->{data} = shift @_;
    }
    return($self->{data});
}

##

sub message {
    my ($self, $time) = @_;
    
    my $type_tags = 
	pad_string(',' 
		   . join('',
			  map {ref($_) 
				   ? $_->[0] 
				   : 's'
			      } @{$self->data}
			 )
		  );
    my $data = $self->{data};
    my $message = join('',
		       $self->command,
		       
		       # type tags 
		       # everything is a string ('s')
		       $type_tags,
		       
		       # the actual strings
		       map { format_data($_) } @$data
		      );
    if ($time) {
        $message = (pad_string('#bundle')
                    . timetag($time)
                    . pack('N', length($message))
                    . $message
                   );
        my $foo = $message;
        $foo =~ s/\0/!/g;
    }
    return($message);
}

##

sub timetag {
    my $timetag = shift;
    my $secs = int($timetag);
    my $frac = $timetag - $secs;
    $secs += 2208988800;
    return(pack("NB32", $secs, frac2bin($frac)));
}

##

sub frac2bin {
    my $bin  = '';
    my $frac = shift;
    while ( length($bin) < 32 ) {
        $bin  = $bin . int( $frac * 2 );
        $frac = ( $frac * 2 ) - ( int( $frac * 2 ) );
    }
    
    return $bin;
}

##

=item $osc->send_message($time)

This is what actually sends the message.  Make sure you've at least
set the server and a command, and also some data as required, before
calling this method.

After calling this, you can use the object to send further OSC
messages.  If (for example) you want to send the same command but with
different data, you can just call the data() method with the new data,
and then call send_message() again.

If you supply the optional $time, the message will be sent
as a single-element OSC 'bundle' with the appropriate timetag.  The
time value is the number of seconds since epoch (January 1, 1970 for
most systems).

=cut

{
    my $handle;
    sub send_message {
      my ($self, $time) = @_;
	my $message = $self->message($time);
	$handle ||= IO::Socket::INET->new(Proto => 'udp') 
	  or die "socket: $@";
	my $server = $self->server
	  or die "you didn't set a server yet!";
	my $sent = send($handle, $message, 0, $server);

	if (not $sent == length($message)) {
	    die("couldn't send: $!");
	}
	return($sent == length($message));
    }
}

##

sub format_data {
    my $data = shift;
    my $type = 's';
    
    if (ref $data) {
        ($type, $data) = @$data;
    }
    
    if ($type eq 's') {
        $data = pad_string($data);
    }
    elsif ($type eq 'i') {
        $data = pack('N', $data || 0);
    }
    elsif ($type eq 'f') {
        $data = scalar(reverse(pack('f', $data || 0)));
    }
    
    return($data);
}

##

# OSC strings must be padded to a length that is a multiple of 4
sub pad_string {
    my $string = shift;
    if (not length($string) % 4) {
	$string .= "\0\0\0\0";
    }
    else {
	$string .= ("\0" x (4 - length($string) % 4));
    }
    return($string);
}

##

=back

=head2 BUGS

Doesn't support bundles yet.

Doesn't support TCP.

Can't receive OSC messages, only send them.

=head2 AUTHOR

Alex McLean - http://slab.org

=head2 SEE ALSO

MIDI::Music - for realtime MIDI

=head2 COPYRIGHT and LICENSE

Copyright (c) 2004 Alex McLean. All rights reserved. This program is
free software; you can redistribute it and/or modify it under the same
terms as Perl itself.

=cut


##

1;
