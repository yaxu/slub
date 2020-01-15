package OSC;

use strict;
use IO::Socket;

use Class::MethodMaker
  get_set => [qw/ data command /];

use constant
  DEFAULT_PORT => 57110;

sub new {
    my($pkg, %p) = @_;
    my $self = bless(\%p, $pkg);
}

##

sub message {
    my $self = shift;
    
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
		       $self->format_command,
		       
		       # type tags 
		       # everything is a string ('s')
		       $type_tags,
		       
		       # the actual strings
		       map { format_data($_) } @$data
		      );
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
	$data = pack('N', $data);
    }
    return($data);
}

##

sub format_command {
    my $self = shift;
    my $command = $self->{command} ||= [];
    return(pad_string('/' . join('/', @$command)));
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

{
    my $handle;
    sub send_message {
	my $self = shift;
	my $message = $self->message;
	$handle ||= IO::Socket::INET->new(Proto => 'udp') 
	  or die "socket: $@";
	my $to = $self->to
	  or die "you didn't set a to command yet!";
	my $sent = send($handle, $message, 0, $self->to);
	if (not $sent == length($message)) {
	    die("couldn't send: $!");
	}
	return($sent == length($message));
    }
}

##

sub to {
    my ($self, $hostname, $port) = @_;
    if ($hostname) {
	$port ||= ($ENV{PORT} or DEFAULT_PORT);
	my $addr = sockaddr_in($port, inet_aton($hostname));
	$self->{to_addr} = $addr;
    }
    $self->{to_addr};
}

##

1;
