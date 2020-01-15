#!/usr/bin/perl -w

package Yaxu;

use strict;

use POE qw(Component::Server::TCP
	   Component::Client::TCP
	   Filter::HTTPD
	   Filter::Stream
	   Component::UserBase
	   Component::Client::Ping
	  );

use HTTP::Response;
use CGI;
use Template;

use Class::MethodMaker
  new_hash_init => 'new',
  get_set => [ qw/ q r session_key uri / ];

init_poe();
exit 0;

##

sub init_poe {
    init_http_server();
    $poe_kernel->run();
}

##

{
    my $tt;
    sub tt {
	$tt ||= Template->new({INCLUDE_PATH => '/home/alex/lab/scan',
			       INTERPOLATE  => 1,
			       PRE_CHOMP    => 1,
			       POST_CHOMP   => 1,
			      }
			     );
    }
}

##

sub init_http_server {
    POE::Component::Server::TCP->new
	( Alias        => "http_server",
	  Port         => 8088,
	  ClientFilter => 'POE::Filter::HTTPD',	  
	  ClientInput  => \&client_input,
	);
}

##

sub init_pinger {
    POE::Component::Client::Ping->spawn(
					Alias       => 'pinger',
					Timeout     => 3,
				       );
}

##

sub client_input {
    my ($kernel, $heap, $request) = @_[KERNEL, HEAP, ARG0];
    
    if ($request->isa("HTTP::Response")) {
	$heap->{client}->put( $request );
	$kernel->yield("shutdown");
	return;
    }
    
    my $uri = $request->uri;
    my $q = CGI->new($request->content);
    
    my @names = $q->param;
    foreach my (@names) {
	$value{$name};
    }
    
    my $response = HTTP::Response->new(200);
    $response->push_header( 'Content-type', 'text/html' );
    $response->content("<html><body>value=$value</body></html>");
    
    $heap->{client}->put($response);
    $kernel->yield("shutdown");
}

##

sub output {
    my $self = shift;
    my $result;
    my $tt = $self->tt();
    my $uri = $self->uri;
    $tt->process($uri,
		 {q       => $self->q, 
		  note    => $self->note,
		  source  => $self->source,
		  session => $self->session
		 },
		 \$result
		)
      or $result = "<head><body>shoo, '$uri'.</body></head>";
    return $result;
}

##

sub init_userbase {
    POE::Component::UserBase->spawn
	( Alias    => 'userbase', # defaults to 'userbase'.
	  Protocol => 'file',     # The repository type.
	  File     => '/home/alex/scan/passwd',
	  Dir      => '/home/alex/scan/persist',
	);
}

##

sub init_scanner {
    POE::Session->create(
			 inline_states =>
			 { _start    => \&next_scan,
			   _child    => \&next_scan_child,
			   next_scan => \&next_scan
			 }
			);
}

##


sub next_scan_child {
    if ($_[ARG0] eq 'lose') {
	$_[KERNEL]->call($_[SESSION], 'next_scan');
    }
}

##

sub next_scan {
    my ($kernel, $heap, $input) = @_[KERNEL, HEAP, ARG0];

    my ($host, $port);

    unless($heap->{host} and $heap->{ports} and @{$heap->{ports}}) {
	$heap->{host}  = new_ip();
	$heap->{ports} = [ports()];
    }
    $port = shift @{$heap->{ports}};
    $host = $heap->{host};
    scan_port($host, $port);
}

##
    
sub scan_port {
    my ($host, $port) = @_;

    POE::Component::Client::TCP->new
	( RemoteAddress => $host,
	  RemotePort    => $port,
	  Filter        => POE::Filter::Stream->new(),
	  
	  Connected => sub {
	      print "connected to $host:$port ...\n";
	      $_[HEAP]->{banner_buffer} = [ ];
	      $_[KERNEL]->delay( send_enter => 5 );
	  },
	  
	  ConnectError => sub {
	      print "could not connect to $host:$port ...\n";
	  },
	  
	  ServerInput => sub {
	      my ($kernel, $heap, $input) = @_[KERNEL, HEAP, ARG0];
	      print "got input from $host:$port ...\n";
	      push @{$heap->{banner_buffer}}, $input;
	      $kernel->delay( send_enter => undef );
	      $kernel->delay( input_timeout => 1 );
	  },
	  
	  InlineStates =>
	  {send_enter => sub {
	       print "sending enter on $host:$port ...\n";
	       $_[HEAP]->{server}->put(""); # sends enter
	       $_[HEAP]->{server}->put(""); # sends enter
	       $_[KERNEL]->delay( input_timeout => 5 );
	   },
	   
	   input_timeout => sub {
	       my ($kernel, $heap) = @_[KERNEL, HEAP];
	       
	       print "got input timeout from $host:$port ...\n";
	       print ",----- Banner from $host:$port\n";
	       foreach (@{$heap->{banner_buffer}}) {
		   print "| $_";
		   # print "| ", unpack("H*", $_), "\n";
	       }
	       print "`-----\n";
	       
	       $kernel->yield("shutdown");
	   },
	  },
	);
}

##

sub call_actions {
    my $self = shift;
    my $q = $self->q;

    foreach my $action ($q->param('action')) {
	my $func = "action__$action";
	if ($self->can($func)) {
	    $self->$func();
	}
    }
}

##

sub action__change_note {
    my $self = shift;
    warn("foo\n");
    $self->note($self->q->param('note'));
}

##

sub session {
    my ($self, $kernel) = @_;
    
    my $session_key = $self->session_key;
    
    if (not $self->{_session}) {
	my %ref;
	$kernel->post('user_base', 'log_on',
		      user_name  => $session_key,
		      persistent => \%ref,
		     );
	$self->{_session} = \%ref;
    }
    $self->{_session};
}

##

sub note {
    my($self, $note) = @_;

    my $session = $self->session;
    
    return('') if not $session->{in_progress};

    if (defined $note) {
	warn("bing\n");
	$session->{note} = $note;
    }
    
    return $session->{note};
}

##

{
    my $source;
    sub source {
	if (not $source) {
	    open(SOURCE, "$0")
	      or return "$! $0.  sorry, mail alex\@slab.org for a copy.";
	    $source = join('', <SOURCE>);
	    close(SOURCE);
	}
	return $source;
    }
}

##

{
    my $chars;
    sub new_username {
	$chars ||= ['a' .. 'f', 0 .. 9];
	return(join('', map {$chars->[rand @$chars]} ( 0 .. 7)));
    }
}

##

sub rand_element {
    my $array = shift;
    $array->[rand @$array];
}

##

sub new_ip {
    my $result;

    $result = rand_element([1 .. 9, 11 .. 254]);
    $result .= '.';

    if ($result eq '192.') {
	$result .= rand_element([0 .. 167, 169 .. 255]);
    }
    else {
	$result .= rand_element([0 .. 255]);
    }
    $result .= '.';

    $result .= rand_element([0 .. 255]);
    $result .= '.';

    $result .= rand_element([2 .. 254]);
    
    return $result;
}

##

sub ports {
    (1 .. 80);
}

##

1;

