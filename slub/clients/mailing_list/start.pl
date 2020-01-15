#!/usr/bin/perl

use strict;
use Date::Calc qw{Add_Delta_DHMS Delta_DHMS};
use IO::File;

use Client;

STDOUT->autoflush(1);

my($mboxfile) = @ARGV;

$mboxfile or die "Please specify an mbox file";

my %months = (Jan =>  1, Feb =>  2, Mar =>  3, Apr =>  4, 
	      May =>  5, Jun =>  6, Jul =>  7, Aug =>  8, 
	      Sep =>  9, Oct => 10, Nov => 11, Dec => 12
	     );

my @messages;
my %threads;

parse();
my $client = Client->new(port      => 6010);
$client->poll();
play();

sub parse {
  open(FH, "<$mboxfile")
    or die "Couldn't open '$mboxfile'";

  while (<FH>) {
    $_ =~ s/\s/ /g;
    if ($_ =~ /^From /) {
      my %headers;
      while (<FH>) {
	last if $_ !~ /\w/;
	$_ =~ /^(.*?):(.*)$/;
	if ($1 eq 'Subject'
	    or $1 eq 'Date'
	    or $1 eq 'From'
	   ) {
	  $headers{$1} = $2;
	}
      }
      if (exists($headers{Subject})
	  and exists($headers{Date})
	  and exists($headers{From})
	 ) {
	$headers{From} =~ /<([^>]+)/;
	$headers{From} = ($1 || '');
	my $date = $headers{Date};
	my ($day, $month, $year,
	    $hour, $minute, $second, 
	    $offset_direction, $offset_hour, $offset_minute
	   ) = 
	     $date =~ m{^\s*(?:[^\d]+,?)?\s*  # weekday - ignore
			(\d+)\s+        # day
			(\w+)\s+        # month
			(\d+)\s+        # year
			(\d\d?):         # hour
			(\d\d)          # minute
			(?::(\d\d))?\s* # second, maybe
			(?:             # offset
			 ([+-])         #   direction
			 (\d\d)         #   hour
			 (\d\d)         #   minute
			)?              # maybe
		       }x;
	
	if (not ($month = $months{$month})) {
          warn("bad month '$month' ($date), must be " 
	       . join(':', keys(%months))
	      );
          next;
        }
	
	if ($offset_hour || $offset_minute) {
	  if ($offset_direction eq '+') {
	    $offset_hour   = 0 - $offset_hour;
	    $offset_minute = 0 - $offset_minute;
	  }
          eval {
	    ($year, $month, $day, $hour, $minute, undef) 
	      = Add_Delta_DHMS($year, $month, $day, 
	 		       $hour, $minute, 0,
			       0, $offset_hour, $offset_minute, 0
			      );
          };
          next if $@;
	}
	
	$date = sprintf('%d-%02d-%02d %02d:%02d:%02d', 
			$year, $month, $day, $hour, $minute, $second
		       );
	#print "a: $headers{Date} b: $date\n";
	$headers{Date} = $date;
	
	
	# Mess with the subject a bit to try to create a thread key
	my $mung = $headers{Subject};
	$mung   =  lc($mung);
	$mung   =~ s/re(\s*[\[\(]\d+[\]\)])?\s*:\s*//g;
	$mung   =~ s/\s+//g;
	$headers{thread} = $mung;
	
	$threads{$mung}++;
	
	push @messages, \%headers;
      }
    }
  }
  close FH;
  @messages = sort {$a->{Date} cmp $b->{Date}} @messages;
}


sub play {
  my @date = split_date($messages[0]->{Date});
  my @last_date = @date;
  my %active_threads;
  my $timescale = timescale();

  foreach my $message (@messages) {
    @date = split_date($message->{Date});

    my $first = not $active_threads{$message->{thread}};
    my $final = (not --$threads{$message->{thread}});
    
    my $note = 
      ($active_threads{$message->{thread}} ||= random_note($first && $final));
    
    #print "$message->{Date}\n";
    @date = split_date($message->{Date});

    if ($first) {
      my $foo = printf("\r%-79s\n", $message->{Subject});
      $foo =~ s/[Rr][Ee]:\d*//;
    }
    my $foo = $message->{From} . ': ' . $message->{Subject};
    $foo = substr($foo, 0, 63);
    printf "\r%-63s [$date[0]-$date[1]-$date[2]]", $foo;
    my $wait = $timescale * date_diff(\@last_date, \@date);
    play_note($note);
    select(undef, undef, undef, $wait); 
    @last_date = @date;

    if ($final) {
      # Send note with velocity of 0
      # (a valid alternative to an explicit midi 'note off')
      $note->[2] = 0;
      play_note($note);
      delete $active_threads{$message->{thread}};
    }
  }
}


sub timescale {
  #print "$messages[0]->{Date}\n";
  #print "$messages[-1]->{Date}\n";
  my @startdate = split_date($messages[0]->{Date} );
  my @enddate   = split_date($messages[-1]->{Date});
  my $age = date_diff(\@startdate, \@enddate);
  my $length = 180; # seconds;
  return($length / $age);
}


sub split_date {
  my $date = shift;
  my(@result) = ($date =~ m/^(\d+)-(\d+)-(\d+)\s+(\d+):(\d+):(\d+)$/);
  return @result;
}

sub date_diff {
  my($d, $h, $m, $s);
  eval {
    ($d, $h, $m, $s) = Delta_DHMS(@{$_[0]}, @{$_[1]});
  };
  if ($@) {
    print("Invalid date: " 
	  . join(':', @{$_[0]}) . ' ' . join(':', @{$_[0]}) 
	  . "\n"
	 );
    exit;
  }
  # Get plain seconds

  $s += ((((($d * 24) + $h) * 60) + $m) * 60);
  return $s;
}

{
  my $channel_range;
  my $velocity_range;
  my $note_range;
  sub random_note {
    my $drum = ($_[0] || 0);

    # all channels, plus extra drums
    $channel_range  ||= [ 1 .. 16, 10];
    $velocity_range ||= [ 90 .. 110];
    $note_range     ||= [ 35 ..  66];

    #    $note_range     ||= [ 35 ..  96]; # full
    
    return([
	    $note_range->[rand(@$note_range)],
	    ($drum 
	     ? 10
	     : $channel_range->[rand(@$channel_range)],
	    ),
	    $velocity_range->[rand(@$note_range)],
	   ]
	  );
  }
}

sub play_note {
  $client->ctrl_send('note', join(', ', @{$_[0]}));
  $client->poll();
}

