#!/usr/bin/perl -w

use strict;
use Client;
use Curses;

my($y, $x) = (0, 0);
my($starty, $startx) = (0, 0);
my @me;
my $size = 1;

my @words = `cat /usr/dict/words`;
chop(@words);

my $pitch = (shift @ARGV or 80);
my $sampleset = (shift @ARGV or 'vibchop');

curses_init();
clear;
init_screen();

my $client = Client->new(port      => 6010,
			 ctrls     => [],
			 on_ctrl   => \&on_ctrl,
			 on_clock  => \&on_clock,
			 timer     => 'ext',
			 tpb       => 32
			);
init_samples();
$client->event_loop;

sub on_ctrl {
  my ($control, $value) = @_;
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
      $client->ctrl_send("caudio", "load $sample_dir/$file as $as");
      push @samples, $as;
      $inc++;
    }
  }

  sub play {
    my $ch = shift;
    $client->ctrl_send("caudio", 
		       'play ' . $samples[ord($ch) % scalar(@samples)]
		       . ' pitch '
		       . $pitch
		      );
  }
}

##

{
  my $c;
  my $not_first;
  sub on_clock {
    user_input();
    addch($y, $x, $c) if $not_first++;
    if (($c = inch($y, ++$x)) eq '#') {
      ++$y; $x=0;
      while (inch($y, $x++) ne '#') {}
      if (inch($y, $x+1) eq '#') {
	($y, $x) = ($starty, $startx);
      }
      $c = inch($y, $x);
    }
    if ((my $ch = inch($y, $x)) ne ' ') {
      play($ch);
    }
    addch($y, $x, 'O');
    refresh();
  }
}

sub curses_init {
  initscr;
  cbreak;
  noecho;
  keypad 1;
  nonl;
  nodelay 1;
}
sub curses_deinit {
  endwin;
}

sub init_screen {
  my $gridseed = 
    [
     ' ' x 64,
     ' ' x 64,
     ' ' x 64,
     ' ' x 64,
     ' ' x 64,
     ' ' x 64,
     ' ' x 64,
     ' ' x 64,
     ' ' x 64,
     ' ' x 64,
     ' ' x 64,
     ' ' x 64,
    ];
  my $length = length($gridseed->[0]);
  my $width = @$gridseed;
  my($iy, $ix) = (0, 0);
  $me[0]->[0] = $starty = $y = $iy = int((25 - $width) / 2);
  $me[0]->[1] = $startx = $x = $ix = int((80 - $length) / 2);
  $me[0]->[2] = ' ';
  addstr($iy - 1, $ix - 1, '#' x ($length + 2));
  foreach my $line (@$gridseed) {
    addstr($iy++, $ix - 1, '#' . $line . '#');
  }
  addstr($iy, $ix - 1, '#' x ($length + 2));
}

sub user_input {
  my $mey = $me[0]->[0];
  my $mex = $me[0]->[1];
  if ((my $in = getch($mey, $mex)) ne ERR) {
    if ($in eq KEY_LEFT) {
      moveme($mey,     $mex - 1);
    }
    elsif ($in eq KEY_RIGHT) {
      moveme($mey,     $mex + 1);
    }
    elsif ($in eq KEY_UP) {
      moveme($mey - 1, $mex    );
    }
    elsif ($in eq KEY_DOWN) {
      moveme($mey + 1, $mex    );
    }
    elsif($in eq ">") {
      my $word = $words[rand @words];
      while($word =~ s/^(.)//) {
	last if not add($1, 0, 1);
      }
    }
    elsif($in eq "^") {
      my $word = $words[rand @words];
      while($word =~ s/^(.)//) {
	last if not add($1, -1, 0);
      }
    }
    elsif($in eq "!") {
      init_screen();
    }
    elsif($in eq '-') {
      sizeme(-1);
    }
    elsif($in eq '+') {
      sizeme(1);
    }
    else {
      add($in, 0, 1);
    }
  }
}

{
  my $c;
  my $first;

  sub sizeme {
    my $newsize = ($size + shift);
    $newsize = 1 if ($newsize < 1);
    while ($newsize < scalar(@me)) {
      my $dead = pop @me;
      addch(@$dead);
    }
    $size = $newsize;
  }

  sub moveme {
    my ($ny, $nx, $fnc) = @_;
    my $result = 1;
    my $nc;
    if (($nc = inch($ny, $nx)) ne '#') {
      addch($ny, $nx, (defined($fnc) ? $fnc : 'x'));
      unshift(@me, [$ny, $nx, (defined($fnc) ? $fnc : $nc)]);
      if ($size < scalar(@me)) {
	addch(@{pop @me});
      }
    }
    else {
      $result = 0;
    }
    return $result;
  }

  sub add {
    my($nc, $offsety, $offsetx) = @_;
    $me[-1]->[2] = $nc;
    return moveme($me[0]->[0] + $offsety, $me[0]->[1] + $offsetx);
  }
}

