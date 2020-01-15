#!/usr/bin/perl -w

use strict;

use Client;

my @ctrls = qw{play_loop loopsize granularity uniform_rand threshold
               strong_unite unity dupe loopvol loopoff
	      };

my $c;
my $loopsize = 16;
my $pos = 0;
my $granularity = 4;
my $threshold = 100;
my $unity = 32;
my $strong_unite = 0;
my $volume = 1;
my $pitch = 60;
my @loops;
my %values;

init();

sub init {
  $c = Client->new(port     => 6010,
		   ctrls    => \@ctrls,
		   on_ctrl  => \&on_ctrl,
		   on_clock => \&on_clock,
		   timer    => 'ext',
		   tpb      => 1,
		  );
  $c->event_loop;
}

sub on_ctrl {
  my ($ctrl, @values) = @_;

  if ($ctrl eq 'play_loop') {
    play_loop(@values);
  }
  elsif ($ctrl eq 'loopsize') {
    $loopsize = $values[0];
  }
  elsif ($ctrl eq 'granularity') {
    $granularity = $values[0];
  }
  elsif ($ctrl eq 'threshold') {
    $threshold = $values[0];
  }
  elsif ($ctrl eq 'unity') {
    $unity = $values[0];
  }
  elsif ($ctrl eq 'strong_unite') {
    $strong_unite = $values[0];
  }
  elsif ($ctrl eq 'loopvol') {
    $volume = $values[0];
  }
  elsif ($ctrl eq 'loopoff') {
    $pitch = $values[0];
  }
  elsif ($ctrl eq 'dupe') {
    dupe();
  }
  else {
    $values{$ctrl} = $values[0];
  }
}


sub dupe {
  #$loops[0] = [((@{$loops[0]})[0 .. (int($loopsize / 2) - 1)]) x 2];
  #$loops[1] = [((@{$loops[1]})[0 .. (int($loopsize / 2) - 1)]) x 2];
}

{
  my $gran;
  sub on_clock {
    $gran ||= 0;
    if ($gran++ % $granularity) {
      return;
    }

    my $to_play;
    if ($pos > $unity) {
      if (not $pos % 2) {
	warn("united\n");
	# unite
	if ($strong_unite) {
	  $to_play = {
		      %{$loops[0]->[$pos    ] || {}}, 
		      %{$loops[1]->[$pos    ] || {}},
		      %{$loops[0]->[$pos + 1] || {}}, 
		      %{$loops[1]->[$pos + 1] || {}}
		     };
	}
	else {
	  $to_play = {
		      %{$loops[0]->[$pos    ] || {}}, 
		      %{$loops[1]->[$pos    ] || {}}
		     };
	}
      }
    }
    else {
      print("loop " . ($pos % 2) . "\n");
      $to_play = $loops[$pos % 2]->[$pos];
    }

    my $rand;
    if ($to_play) {
      if (not $values{uniform_rand}) {
	$rand = rand($threshold);
      }
      while (my($message, $prob) = each %{$to_play}) {
	if ($values{uniform_rand}) {
	  $rand = rand($threshold);
	}
	if($rand < $prob) {
	  play($message);
	}
      }
    }
    ++$pos;
    $pos = $pos % $loopsize;
  }
}

{
  my $not_first;
  sub play_loop {
    my ($message, $loop_no, @etc) = @_;
    if (not $not_first) {
      # first, set up metronome
      $loops[$loop_no]->[0]->{$message} = 100;
    }
    else {
      $loops[$loop_no]->[$pos]->{$message} += 10;
    }
    play($message, @etc);
    $not_first = 1;
  }
}

sub play {
  my $message = shift;
  if ($volume) {
    $c->ctrl_send("MSG", "$message vol $volume pitch $pitch");
  }
}

