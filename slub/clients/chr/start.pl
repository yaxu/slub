#!/usr/bin/perl -w

use strict;
use Tk;
use Tk::Canvas;
use IO::Handle;
use Client;

use Data::Dumper;

my $gridx = 1;
my $gridy = 1;

my $cell_width = 6;
my $cell_height = 6;

my $main_height = $cell_height * 17 * 2;
my $main_width = $cell_width * 17 * 2;

my $hotx = $cell_width * 10;

my $client;

my $last_phrase_move;

my $pitch = (shift @ARGV or 80);
my $sampleset = (shift @ARGV or 'stomp');

my $move = shift @ARGV;

my $line_color = 'grey';
my $active_line_color = 'white';

my $global_c;

my @controls = qw(intensity disorder offset complexity independence volume);
my %values = map {$_ => 0} @controls;

init();
MainLoop;

##

sub init {
  $client = Client->new(port      => 6010,
			ctrls     => \@controls,
			on_ctrl   => \&on_ctrl,
			on_clock  => \&on_clock,
			timer     => 'ext',
			tpb       => ($ENV{TPB} or 4)
		       ); 
  init_samples();
 
  my $sock = $client->sock();

  $values{volume} = 0;
  # Create main window and canvas
  my $main = MainWindow->new();
  #$main->wm("minsize",  400,300);
  $main->wm("geometry", "400x300");
  my $c = $main->Canvas(-width => $main_width,
			-height => $main_height
		       );
  $global_c = $c;
  
  $values{listen} = 1;

  $c->pack(-expand => 1, -fill => 'both');
  
  $main->bind('<KeyPress-r>', [\&reverse_phrase, $c]);
  $main->bind('<KeyPress-l>', [\&toggle_ctrl_listen]);
  
  $main->fileevent($client->sock(), readable => \&read_sock);

  if ($move) { 
    foreach my $control (@controls) {
      $client->ctrl_send('move', $control);
    }
  }

  make_silence($c, $cell_width, $cell_height, 8);
}

##

sub toggle_ctrl_listen {
  $values{listen} = ! $values{listen};
  print($values{listen} ? "listening\n" : "ignoring\n");
}

sub item_move {
  my ($c, $tag, $phrase_tag, $x, $y) = @_;
  $x = ($x - ($x % $gridx));
  $y = ($y - ($y % $gridy));
  return if $y <= 0 or $x <= 0;

  $last_phrase_move = $phrase_tag;

  my @coords = $c->coords($tag);
  my @diff = ($x - $coords[0], $y - $coords[1]);

  foreach my $cell_tag ($c->find('withtag', $phrase_tag)) {
    my @item_coords = $c->coords($cell_tag);
    $item_coords[0] += $diff[0]; 
    $item_coords[1] += $diff[1];
    $item_coords[2] += $diff[0];
    $item_coords[3] += $diff[1];
    
    $c->coords($cell_tag, @item_coords);

    foreach my $line ($c->find('withtag', "from_$cell_tag")) {
      my @line_coords = $c->coords($line);
      $line_coords[0] = $item_coords[0];
      $line_coords[1] = $item_coords[1];
      $c->coords($line, @line_coords);
    }

    foreach my $line ($c->find('withtag', "to_$cell_tag")) {
      my @line_coords = $c->coords($line);
      $line_coords[2] = $item_coords[0];
      $line_coords[3] = $item_coords[1];
      $c->coords($line, @line_coords);
    }
    
    #mung_color($c, $item, $item_coords[0] + $diff[0]);
  }
}

##

sub item_move_end {
  my ($tag, $x, $y) = @_;
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
    
    foreach my $file (@files) {
      next if not $file =~ /wav$/i;
      my $as = "${sampleset}$inc";
      $client->send("MSG", "load $sample_dir/$file as $as");
      push @samples, $as;
      $inc++;
    }
  }

  ##

  sub rand_event {
    my $rand_pitch = $pitch + ((rand(16))-8);
    
    if (rand(8) > 6) {
      return(255, undef);
    }
    my $sample_no = rand @samples;
    my $sample = $samples[$sample_no];
    my $cell_brightness = ((255 - int(($sample_no / @samples) * 205)) + 50);
    return($cell_brightness,
	   sub {
	     my($c, $x, $y, $maxx, $maxy) = @_;
	     
	     $y = ($y / $maxy);
	     
	     my $rand_cutoff = $y * 10000;
	     $rand_cutoff += 100;

	     my $pan = (1 - ($x / $maxx));

	     $client->send("MSG", 
				'play ' . $sample
				. ' vol '
				. $values{volume}
				. ' pitch '
				. ($rand_pitch + $values{offset})
				. ' cutoff '
				. $rand_cutoff
				. ' res 0.6 '
				. ' pan '
				. $pan
			       )
	   }
	  );
  }
}

##

sub read_sock {
  $client->poll();
}

##

sub on_ctrl {
  my ($control, $value) = @_;
  
  return if not $values{listen};

  $value = ($value + 0);

  if ($control eq 'complexity') {
    while (int($values{$control}) < $value) {
      make_phrase($global_c);
    }
    while (int($values{$control}) > $value) {
      unmake_random_phrase($global_c);
    }
  }
  elsif ($control eq 'intensity') {
    while ($values{$control} < $value) {
      make_thing($global_c);
      $values{$control}++;
    }
    while ($values{$control} > $value) {
      unmake_random_thing($global_c);
      $values{$control}--;
    }
  }
  else {
    $values{$control} = $value;
  }
}

##

sub on_clock {
  move_things($global_c);
}

##

{
  my %things;
  my $thing_id;
  sub make_thing {
    my $c = shift;
    my $first_cell = first_cell();
    $thing_id ||= 0;
    my $thing_tag = "thing_$thing_id";
    my ($x1, $y1) = $c->coords($first_cell);
    $c->create(('oval', 
		$x1, $y1, $x1 + $cell_width, $y1 + $cell_height
	       ),
	       -tags    => ['thing',
			    $thing_tag
			   ],
	       -fill    => 'white',
	       -outline => 'white'
	      );

    $things{$thing_tag} = $first_cell;
    ++$thing_id;
    $thing_tag;
  }

  sub unmake_thing {
    my ($c, $thing) = @_;
    $c->delete($thing);
    delete $things{$thing};
  }
  
  sub unmake_random_thing {
    my $c = shift;
    my @things = keys %things;
    unmake_thing($c, $things[rand(@things)]) if @things;
  }

  sub move_things {
    my $c = shift;
    deactivate_lines($c);
    foreach my $thing (keys %things) {
      $c->raise($thing);
      my $to = next_cell($c, $thing, $things{$thing});
      $c->coords($thing, $c->coords($to));
      $things{$thing} = $to;
      play($c, $to);
    }
  }
}


##

{
  my %loops;
  my %phrase_starts;
  my $last;
  my $last_tag;
  my $complete;
  my $silence_id;
  my @silence_ids;
  my $cell_id;
  my $phrase_id;
  my @active_lines;
  my(%incs, $inc);
  my $head_pos;
  
  sub unmake_phrase {
    my ($c, $phrase_tag) = @_;
    foreach my $cell ($c->find('withtag', $phrase_tag)) {
      $c->delete($cell);
      delete $loops{$cell};
    }
    
    $c->delete($phrase_tag . '_line');

    my ($from, $to) = @{$phrase_starts{$phrase_tag}};
    delete $phrase_starts{$phrase_tag};
    $loops{$from} = [grep {$_ ne $to} @{$loops{$from}}];
    $values{complexity}--;
  }

  sub reverse_phrase {
    my($main, $c) = @_;
    my $phrase = $last_phrase_move;
    return if not $phrase;
    my ($from, $to) = @{$phrase_starts{$phrase}};
    my @cells;
    $loops{$from} = [grep {$_ ne $to} @{$loops{$from}}];
    while ($to !~ /^silence/) {
      push @cells, $to;
      $to = $loops{$to}->[0];
    }
    my @rev = reverse @cells;
    
    push @{$loops{$from}}, $rev[0];

    my $cell;
    my @colors = map {$c->itemcget($_, '-fill')} @cells;

    while(@rev) {
      $cell = shift @rev;
      $c->itemconfigure($cell, -fill => shift(@colors));
      if(@rev) {
	$loops{$cell} = [$rev[0]];
      }
    }
    $loops{$cell} = [$to];
    $phrase_starts{$phrase} = [$from, $cells[-1]];
  }
  
  sub unmake_random_phrase {
    my $c = shift;
    
    my @phrases = keys %phrase_starts;
    unmake_phrase($c, $phrases[rand @phrases]);
  }

  sub first_cell {
    return $loops{begin}->[0];
  }

  sub next_cell {
    my($c, $thing, $tag) = @_;
    
    my $linc = $values{independence} ? \$incs{$thing} : \$inc;

    my $next_tag;
    my $next_tags = $loops{$tag};
    if (not $next_tags) {
      warn "reverting";
      $next_tags = $loops{first_cell()};
    }
    $$linc ||= 0;
    
    if (not @$next_tags) {
      $next_tag = first_tag();
      warn("reverting\n");
    }
    else {
      if (@$next_tags == 1) {
	$next_tag = $next_tags->[0];
      }
      else {
	if (rand(1) < (abs($values{disorder}) / 100)) {
	  $next_tag = $next_tags->[rand(@$next_tags)];
	}
	else {
	  if ($$linc >= @$next_tags) {
	    $$linc = 0;
	  }
	  $next_tag = $next_tags->[$$linc];
	  ++$$linc;
	}
      }
    }
    
    $last_tag ||= '';
    my $active_line;
    if($tag !~ /^silence/) {
      ($active_line) = $c->find('withtag', "to_$next_tag");
    }
    if ($last_tag !~ /^silence/) {
      ($active_line) = $c->find('withtag', "from_$last_tag");
    }

    if ($active_line) {
      $c->itemconfigure($active_line, 
 			'-fill' => $active_line_color
 		       );
      push @active_lines, $active_line;
    }
    $last_tag = $tag;
    return $next_tag;
  }

  sub deactivate_lines {
    my $c = shift;
    foreach my $active_line (@active_lines) {
      $c->itemconfigure($active_line, 
			'-fill' => $line_color
		       );
    }
    undef @active_lines;
  }
  
  sub start_phrase_link {
    die "last is set and shouldn't be" if $last;
    $phrase_id ||= 0;
    $head_pos = rand @silence_ids;
    my $head = $silence_ids[$head_pos];
    $last = $head;
    
    my $phrase_tag = "phrase_$phrase_id";
    
    return($head, $phrase_tag);
  }
    
  sub hack_start_phrase_link {
    my ($phrase_tag, $head, $to) = @_;
    $phrase_starts{$phrase_tag} = [$head, $to];
  }
  
  sub cell_phrase_link {
    my $cell = shift;
    
    push @{$loops{$last}}, $cell;
    $last = $cell;
  }
  
  sub complete_phrase_link {
    my $tail_pos = (int(rand(scalar(@silence_ids) / 4)) * 4);

    $tail_pos += $head_pos;
    $tail_pos = ($tail_pos % scalar(@silence_ids));

    my $tail = $silence_ids[$tail_pos];
    push @{$loops{$last}}, $tail;
    
    undef $last;
    ++$phrase_id;
    return $tail;
  }
  
  sub silence_link {
    $silence_id ||= 0;
    my $tag = "silence_$silence_id";

    die "tried to link into completed loop" if $complete;

    $loops{$last || 'begin'} = [$tag];
    $loops{$tag} = 'last';

    $last = $tag;

    push @silence_ids, $tag;

    ++$silence_id;
    return $tag;
  }

  sub complete_silence {
    $loops{$last} = [first_cell()];
    undef $last;
    $complete = 1;
  }
}

##

sub make_silence {
  my ($c, $x1, $y1, $length) = @_;
  my $cell_brightness = 127;
  
  my $width = $cell_width * 2;
  my $height = $cell_height * 2;
  
  foreach my $cell (0 .. ($length - 1)) {
    my $silence_tag = silence_link();
    make_cell($c, 
	      $x1 + ($cell * $width), 
	      $y1,
	      $cell_brightness,
	      'silence',
	      $silence_tag
	     );
  }

  foreach my $cell (0 .. ($length - 1)) {
    my $silence_tag = silence_link();
    make_cell($c, 
	      $x1 + ($length * $width),
	      $y1 + ($cell * $height),
	      $cell_brightness,
	      'silence',
	      $silence_tag
	     );
  }

  foreach my $cell (0 .. ($length - 1)) {
    my $silence_tag = silence_link();
    make_cell($c, 
	      $x1 + ($length * $width) - ($cell * $width), 
	      $y1 + ($length * $height),
	      $cell_brightness,
	      'silence',
	      $silence_tag
	     );
  }

  foreach my $cell (0 .. ($length - 1)) {
    my $silence_tag = silence_link();
    make_cell($c, 
	      $x1, 
	      ($y1 + ($length * $height)) - ($cell * $height),
	      $cell_brightness,
	      'silence',
	      $silence_tag
	     );
  }
  complete_silence();
}

##

{
  my %events;
  sub attach_event {
    my ($event, $tag) = @_;
    
    $events{$tag} = $event;
  }

  sub clear_event {
    my ($tag) = @_;
    
    delete $events{$tag};
  }
  
  sub play {
    my($c, $tag) = @_;
    if (my $event = $events{$tag}) {
      my @coords = $c->coords($tag);
      $event->($c, $coords[0], $coords[1], $main_width, $main_height);
    }
  }
}  

sub make_phrase {
  my $c = shift;
  
  my($head, $phrase_tag) = start_phrase_link();

  my($first_cell_tag, $last_cell_tag);
  
  my ($x1, $y1) = rand_position();
    
  my $length = int(rand(4) + 1);
  foreach my $cell_id (0 .. ($length - 1)) {
    my($cell_brightness, $event) = rand_event();

    my $cell_tag = make_cell($c, $x1, $y1, $cell_brightness,
			     $phrase_tag
			    );
    $first_cell_tag ||= $cell_tag;
    $last_cell_tag = $cell_tag;

    cell_phrase_link($cell_tag);

    attach_event($event, $cell_tag);
    
    
    $x1 += $cell_width;
  }

  hack_start_phrase_link($phrase_tag, $head, $first_cell_tag);
  my $tail = complete_phrase_link();

  make_line($c, $phrase_tag, $head, $first_cell_tag);
  make_line($c, $phrase_tag, $last_cell_tag, $tail);
  

  $c->bind($phrase_tag, '<B1-Motion>', [\&item_move,
					$first_cell_tag,
					$phrase_tag,
					Ev('x'), Ev('y'), 
				       ]
	  );
  $c->bind($phrase_tag, '<ButtonRelease-1>', [\&item_move_end, 
					      $first_cell_tag,
					      $phrase_tag,
					      Ev('x'), Ev('y'), 
					     ]
	  );
  $c->bind($phrase_tag, '<Button-3>', [\&unmake_phrase,
				 $phrase_tag
				]
	  );
  $values{complexity}++;
}

##

sub make_line {
  my ($c, $phrase_tag, $from, $to) = @_;
  my ($fx, $fy) = $c->coords($from);
  my ($tx, $ty) = $c->coords($to);
  my $line = $c->createLine($fx, $fy, $tx, $ty, 
			    -tags => ["from_$from", "to_$to", "line",
				      "${phrase_tag}_line"
				     ], 
			    -arrow => 'last', 
			    -fill => $line_color,
			    #			    -dash => [2, 1, 1, 2]
			   );
  $c->lower($line);
  return $line;
}

##

sub make_cell {
  my ($c, $x1, $y1, $cell_brightness, @tags) = @_;
  my $tag = $c->create(('rectangle', 
			$x1, $y1, $x1 + $cell_width, $y1 + $cell_height
		       ),
		       -tags    => ['cell',
				    @tags,
				   ],
		       -outline    => sprintf("#%02x%02x%02x",
					      $cell_brightness,
					      $cell_brightness,
					      $cell_brightness
					     ),
		       -fill    => sprintf("#%02x%02x%02x",
					   $cell_brightness,
					   $cell_brightness,
					   $cell_brightness
					  ),
		       -width => 1,
		      );
  return $tag;
}


##

{
  my $inc;
  sub rand_position {
    my ($x, $y);
    $inc ||= 0;
    $inc = ($cell_height * 3) if $inc < ($cell_height * 3);
    if ($inc > $main_height) {
      $inc = $cell_height * 3;
    }
    my $offsetx = int(rand($main_width - ($cell_width * 4)));
    $offsetx -= $offsetx % $cell_width;
    $x = $cell_width * 2 + $offsetx;
    
    $y = $inc;
    
    $inc += $cell_height * 3;
    return($x, $y);
  }
}

##

exit 0;

