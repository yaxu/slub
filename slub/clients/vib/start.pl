#!/usr/bin/perl -w

use Client;
use strict;
use Data::Dumper;

my $inner_size = 32;
my $start_bugs = 0;

my @ctrl_names = qw{complexity intensity disorder independence offset dump};
my %ctrls = map {$_ => 0} @ctrl_names;
my $client;

my $pitch = (shift @ARGV or 80);
my $sampleset = (shift @ARGV or 'vibchop');
my $load = ((not @ARGV) or $ARGV[0] ne 'noload');

my $inner_event = undef;

init();
start();

sub start {
  warn("loop\n");
  $client->event_loop;
}

##

sub on_ctrl {
  my ($control, @data) = @_;

  if ($control eq 'complexity')  {
    warn("Complexity: $data[0]\n");
    complexity_change($data[0]);
  }
  elsif ($control eq 'intensity')  {
    warn("Intensity: $data[0]\n");
    intensity_change($data[0]);
  }
  elsif ($control eq 'disorder')  {
    disorder_change($data[0]);
  }
  elsif ($control eq 'independence')  {
    independence_change($data[0]);
  }
  elsif ($control eq 'dump')  {
    dump_structure();
  }
  else {
    $ctrls{$control} = $data[0];
  }
}

##

{
  my @samples;

  sub init_samples {
    my $sample_dir = "/slub/samples/$sampleset";
    
    opendir(DIR, $sample_dir) || die "can't opendir $sample_dir: $!";
    my @files = grep { -f "$sample_dir/$_" } readdir(DIR);
    closedir DIR;
    
    my $inc = 0;
    
    $client->send('MSG', '# will you work') if $load;

    foreach my $file (@files) {
      next if not $file =~ /wav$/i;
      my $as = "${sampleset}$inc";
      $client->send('MSG', "load $sample_dir/$file as $as")
	  if $load;
      push @samples, $as;
      $inc++;
    }
  }

  {
    
    my $inc;
    sub event {
      my $type = shift;
      my $prev_tag = shift;

      my $tag = ("static_" . ($inc || 0));
      if ($type and $type eq 'random') {
	$tag = "close_$tag";
      }
      ++$inc;

      my $event;
      if ($type and $type eq 'random') {
	my $rand_pitch = $pitch + ((rand(16))-8);
	my $sample = $samples[rand @samples];
	$event = sub {
	  my $mover = shift;
	  $client->send('MSG', 
			     'play ' . $sample
			     . ' pitch '
			     . ($rand_pitch + ($ctrls{offset} || 0))
			    );
	};
      }
      else {
	$event = sub {
	  my $mover = shift;
	  $client->ctrl_send("vis0_move",
			     $mover, $tag
			    );
	};
      }

      return({tag => $tag,
	      event => $event,
	      next => undef
	     }
	    );
    }
  }
}

{
  my @last;

  sub init_last {
    for (0 .. ($start_bugs - 1))  {
      push @last, rand_inner();
    }
  }

  sub independence_change {
    $ctrls{independence} = shift;
  }

  sub intensity_change {
    my $intensity = shift;
    my $old_intensity = $ctrls{intensity};
    return if $intensity == $old_intensity;
    while ($intensity < $old_intensity) {
      pop @last;
      --$old_intensity;
    }
    while ($intensity > $old_intensity) {
      push @last, rand_inner();
      ++$old_intensity;
    }
    $ctrls{intensity} = $intensity;
  }
  
  my $disorder;
  sub disorder_change {
    my $value = shift;
    $disorder = ($value || 0);
    $disorder = 1 if $disorder > 1;
    $disorder = 0 if $disorder < 0;
  }

  my $switcher;
  my @independent_switchers;
  
  sub on_clock {
    my $independent = $ctrls{independence};
    foreach my $c (0 .. (scalar(@last) - 1)) {
      $switcher = ($independent
		   ? \$independent_switchers[$c]
		   : $switcher
		  );
      $$switcher ||= 0;
      my $now;
      my $last = $last[$c];
      if ($last->{next}) {
	my $range = scalar(@{$last->{next}});
	if ($range == 1) {
	  $now = $last->{next}->[0];
	}
	else {
	  if (++$$switcher >= $range) {
	    $$switcher = 0;
	  }
	  
	  if (not $disorder or (rand(1) > $disorder)) {
	    $now = $last->{next}->[$$switcher];
	  }
	  else {
	    $now = $last->{next}->[rand @{$last->{next}}];
	  }
	}
      }
      else {
	$now = first();
	print "warning: reverting\n";
      }
      $now->{event}->("mover_$c") if $now->{event};
      $last[$c] = $now;
    }
  }
}  

sub complexity_change {
  my $complexity = shift;
  my $old_complexity = $ctrls{complexity};
  return if $complexity == $old_complexity;
  if ($complexity < $old_complexity) {
    remove_outer($old_complexity - $complexity);
  }
  else {
    add_outer($complexity - $old_complexity);
  }
  $ctrls{complexity} = $complexity;
}

{
  my @inner;
  my @children;
  
  sub dump_structure {
    open FOO, '>/tmp/dump' or die;
    print FOO Dumper \@children;
    close FOO;
  }
  
  sub first {
    $inner[0];
  }

  sub rand_inner {
    return $inner[rand @inner];
  }
  
  sub init {
    $client = Client->new(port      => 6010,
			  ctrls     => \@ctrl_names,
			  on_ctrl   => \&on_ctrl,
			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => 4
			 );
    for my $c (0 .. ($inner_size - 1))  {
      add_inner();
    }
    init_last();
    init_samples();
  }
  
  sub add_inner {
    if (@inner) {
      push @inner, event('normal', $inner[-1]->{tag});
      $inner[-1]->{next} = [$inner[0]];
      shift @{$inner[@inner - 2]->{next}};
      unshift @{$inner[@inner - 2]->{next}}, $inner[@inner - 1];
    }
    else {
      push @inner, event('normal');
      $inner[0]->{next} = [$inner[0]];
    }
  }
  
  sub remove_outer {
    my $by = shift;
    for my $c (0 .. $by - 1) {
      if (@children) {
	my $die = rand @children;
	my $child = $children[$die];
	@children = grep{$_ ne $child} @children;
	@{$child->[0]->{next}} = grep {$_ ne $child->[2]} @{$child->[0]->{next}};
	undef $child->[1]->{next};
      }
    }
  }
  
  sub add_outer {
    my $by = shift;
    for my $c (0 .. $by - 1) {
      my $start = $inner[rand(@inner)];
      my $end = $inner[rand(@inner)];
      my $first;
      my $last;
      my $length = int(rand(4) + 1);
      my $prev = $start;
      for my $c2 (0 .. ($length-1)) {
	my $current = event('random', $prev->{tag});
	push @{$prev->{next}}, $current;
	$first ||= $current;
	$prev = $current;
      }
      $last = $prev;
      push @{$last->{next}}, $end;
      push @children, [$start, $last, $first];
    }}
}

