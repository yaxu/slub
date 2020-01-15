package Editor;

# (c) 2004 Alex McLean

use strict;
use Curses;
use Time::HiRes qw/sleep/;

use Sandbox;
use File::Basename;

use constant MAX_COLUMN => 79;
use constant X => 1;
use constant Y => 0;

##

sub new {
    my ($pkg, %p) = @_;
    my $self = bless(\%p, $_[0]);
    $self->init();
    return($self)
}

##

sub init {
    my $self = shift;
    $self->init_curses;

    $self->{sandbox} = Sandbox->_new({code => $self->{code}});
    $self->init_code;
    $self->{cursor} = [0, 0];
    $self->{yscroll} = 0;
    $self->{xscroll} = 0;
    $self->{modification} = 0;
    $self->{revision} = undef;
    $self->redraw;
    $self->exec_thread;
    $self->{sandbox}->reload(1);
    refresh();
}

##

sub exec_thread {
    my $self = shift;
    if (not $self->{exec_thread}) {
        $self->{exec_thread} =
          $self->{sandbox}->new_thread;
    }
}

##

sub init_code {
    my $self = shift;
    my $code = $self->{sandbox}->code;
    
    if ($self->{text}) {
        push(@$code, split("\n", $self->{text}));
        delete $self->{text};
    } else {
        push(@$code, (
                      'sub bang {',
                      '    my $self = shift;',
                      #'',
                      #'    # Type something interesting for me to do here',
                      #'',
                      #'    $self->code->[3] .= ".";',
                      #'    $self->modified;',
                      '}',
                     )
            );
    }
    $self->{code} = $code;
}

##

sub redraw {
    my $self = shift;

    #clear;
    my $code = $self->{code};
    my $text = "";
    for (my $c = $self->{yscroll}; $c < scalar(@$code); ++$c) {
        addstr($c - $self->{yscroll}, 0, $code->[$c] 
               . (' ' x (MAX_COLUMN - length($code->[$c])))
              );
    }
    
    move(@{$self->{cursor}});
    $self->stream();
}

sub stream {
  my $self = shift;
  return unless $ENV{WEHAVEAVIDEO};

  my $code = $self->{code};
  my $text = "";
  my $n = 0;
  for (my $c = $self->{yscroll}; $c < scalar(@$code); ++$c) {
    my $line = substr($code->[$c], 0, MAX_COLUMN) . " \n";
    if ($c == $self->{cursor}->[Y]) {
      $line =~ s/(.{$self->{cursor}->[X]})(.?)/$1£$2£/;
    }
    $text .= $line;
    last if $n++ == 24;
  }
  $text =~ s/</&lt;/gs;
  $text =~ s/>/&gt;/gs;
  $text =~ s/£/<u>/s;
  $text =~ s/£/<\/u>/s;
  
  $self->{videoosc} ||= Net::OpenSoundControl::Client->new(Host => "127.0.0.1",
							   Port => 7777
							  );
  $self->{videoosc}->send(['/text/feedback',
			   's', $text
			  ]);
}

##

sub init_curses {
    initscr();
    #start_color();
    cbreak();
    noecho();
    nonl();
    #idlok(1);
    scrollok(1);
    keypad(1);
    nodelay(1);
    move(0, 0);
}

##

sub run {
    my $self = shift;
    while (1) {
        sleep(0.1);
        while ((my $ch = getch()) ne ERR) {
            my $ord = ord($ch);
            if (length($ch) == 1) {
                if ($ord >= 1 and $ord <= 26) {
                    $ch = ('ctrl_' . chr($ord + ord('a') - 1));
                } else {
                    $self->add($ch);
                }
            }
            # this should really be a hash lookup by now
            if ($ch eq ' ') {
                $ch = 'space';
            } elsif ($ch eq KEY_LEFT) {
                $ch = 'left';
            } elsif ($ch eq KEY_RIGHT) {
                $ch = 'right';
            } elsif ($ch eq KEY_UP) {
                $ch = 'up';
            } elsif ($ch eq KEY_DOWN) {
                $ch = 'down';
            } elsif ($ch eq KEY_ENTER) {
                $ch = 'enter';
            } elsif ($ch eq KEY_BACKSPACE) {
                $ch = 'backspace';
            } elsif ($ord == 9) {
                $ch = 'tab';
            } elsif ($ch eq 'ctrl_d') {
                $ch = 'delete';
            } elsif ($ch eq 'ctrl_b') {
                $ch = 'left';
            } elsif ($ch eq 'ctrl_f') {
                $ch = 'right';
            } elsif ($ch eq 'ctrl_p') {
                $ch = 'up';
            } elsif ($ch eq 'ctrl_n') {
                $ch = 'down';
            } elsif ($ch eq '274') {
                $ch = 'f10';
            } elsif ($ch eq '393') {
                $ch = 'ctrl_left';
            } elsif ($ch eq '402') {
                $ch = 'ctrl_right';
            }
            
            my $func = "key__$ch";
            if ($self->can($func)) {
                $self->$func();
            }
        }
        if ($self->{sandbox}->modification > $self->{modification}) {
            $self->{modification} = $self->{sandbox}->modification;
            if ($ENV{SELFMODIFY}) {
                $self->{sandbox}->reload(1);
            }
            $self->redraw;
        }
	
        refresh;
    }
}

##

sub key__ctrl_r {
    my $self = shift;
    my $filename = $self->archive_filename;
    my $code = join("\n", @{$self->{code}});

    open(FN, ">$filename")
      or warn "couldn't write to $filename";
    print FN $code;
    close FN;
    system("echo '" . scalar(localtime) . "' | ci -l -q $filename 2>&1");
    $self->{revision} = undef;
}

sub archive_filename {
    my $self = shift;
    unless (exists $self->{filename}) {
        my($sec, $min, $hour, $day, $month, $year) = localtime(time);
        $month += 1;
        $year += 1900;
        my $date = sprintf("%04d-%02d-%02d", $year, $month, $day);
        my $dir;
        my $file;
        if ($self->{file}) {
            $dir = dirname($self->{file});
            $file = basename($self->{file});
        } else {
            $dir = "/yaxu/archive/$date/";
            $file = "$hour:$min:$sec-$$";
        }
        mkdir $dir unless -d $dir;
        mkdir "$dir/RCS" unless -d "$dir/RCS";
        chdir($dir);
        
        $self->{filename} = $file;
    }
    return($self->{filename});
}

sub key__ctrl_x {
    my $self = shift;
    $self->{sandbox}->reload(1);
}

##

sub current_y {
    my $self = shift;
    return $self->{cursor}->[Y] + $self->{yscroll};
}

##

sub current_line {
    my $self = shift;

    my $y = $self->current_y;

    if (not defined $self->{code}->[$y]) {
        $self->{code}->[$y] = '';
    }
    return(\$self->{code}->[$y]);
}

##

sub add {
    my ($self, $ch) = @_;
    my $cursor = $self->{cursor};
    if ($cursor->[X] < (MAX_COLUMN - 1)) {
        my $line = $self->current_line;
	
        if (length($$line) < $cursor->[X]) {
            $$line .= (' ' x ($cursor->[X] - length($$line)));
        }
        substr($$line, $cursor->[X], 0) = $ch;
	
        insstr($ch);
        $self->key__right;
    }
    $self->stream;
}

##

sub key__backspace {
    my $self = shift;
    my $cursor = $self->{cursor};
    my $line = $self->current_line;
    my $drawn  = 0;
    if ($cursor->[X] > 0) {
        --$cursor->[X];
        move(@$cursor);
        delch();
        if (length($$line) > $cursor->[X]) {
            substr($$line, $cursor->[X], 1) = '';
        }
    } else {
        my $y = $self->current_y;
        if ($y > 0) {
            if (length($self->{code}->[$y - 1]) + length($$line) 
                <= MAX_COLUMN
               ) {
                $cursor->[Y]--;
                $cursor->[X] = length($self->{code}->[$y - 1]);
                $self->{code}->[$y - 1] .= $$line;

                # perl can't splice shared arrays!
                #splice(@{$self->{code}}, $y, 1);
                my @code = @{$self->{code}};
                my @head = @code[0 .. $y - 1];
                my @tail = @code[$y+1 .. $#code];
                @{$self->{code}} = (@head, @tail);
		
                $self->redraw();
		$drawn++;
            }
        }
    }
    $self->stream unless $drawn;
}

##

sub key__delete {
    my $self = shift;
    my $cursor = $self->{cursor};
    my $line = $self->current_line;
    
    if ($cursor->[X] < length($$line) ) {
        delch();
        substr($$line, $cursor->[X], 1) = '';
    } else {
        my $y = $self->current_y;
        if ($y < scalar(@{$self->{code}})) {
            if (length($self->{code}->[$y + 1]) + length($$line) 
                <= MAX_COLUMN
               ) {
                $$line .= $self->{code}->[$y + 1];

                # perl can't splice shared arrays!
                #splice(@{$self->{code}}, $y + 1, 1);
                my @code = @{$self->{code}};
                my @head = @code[0 .. $y];
                my @tail = @code[$y+2 .. $#code];
                @{$self->{code}} = (@head, @tail);
		
                $self->redraw();
            }
        }
    }
}

##

sub key__ctrl_a {
    my $self = shift;
    my $cursor = $self->{cursor};
    $cursor->[X] = 0;
    move(@$cursor);
    $self->stream();
}

##

sub key__ctrl_e {
    my $self = shift;
    my $cursor = $self->{cursor};
    $cursor->[X] = length($self->{code}->[$self->current_y] or '');
    move(@$cursor);
    $self->stream();
}

##

sub key__ctrl_k {
    my $self = shift;
    my $cursor = $self->{cursor};

    clrtoeol();
    my $line = $self->current_line;
    if (length($$line) == 0) {
        deleteln();
        my $y = $self->current_y;
	
        # perl can't splice shared arrays!
        #splice(@{$self->{code}}, $y, 1);
        my @code = @{$self->{code}};
        my @head = @code[0 .. $y - 1];
        my @tail = @code[$y + 1 .. $#code];
        @{$self->{code}} = (@head, @tail);
    } elsif (length($$line) > $cursor->[X]) {
        $$line = substr($$line, 0, $cursor->[X])
    }
}

##

sub key__ctrl_l {
    my $self = shift;

    clear();
    $self->redraw();
    refresh();
}

##

sub key__ctrl_m {
    my $self = shift;
    my $cursor = $self->{cursor};
    my $line = $self->current_line;
    my $cut = '';
    if ($cursor->[X] < length($$line)) {
        $cut = substr($$line, $cursor->[X]);
        substr($$line, $cursor->[X]) = '';
    }
    
    $cursor->[Y]++;
    $cursor->[X] = 0;

    my $y = $self->current_y;

    # perl can't splice shared arrays!
    #splice(@{$self->{code}}, $y, 0, $cut);
    my @code = @{$self->{code}};
    my @head = @code[0 .. $y - 1];
    my @tail = @code[$y .. $#code];
    @{$self->{code}} = (@head, $cut, @tail);

    $self->redraw();
}

##

sub key__up { 
    my $self = shift;
    my $cursor = $self->{cursor};

    if ($cursor->[Y] > 0) {
        --$cursor->[Y];
    } else {
        if ($self->{yscroll}) {
            scrl(-1);
            $self->{yscroll}--;
            $self->redraw();
        }
    }

    my $max = length($self->{code}->[$self->current_y] or '');
    $cursor->[X] = $max if $cursor->[X] > $max;

    move(@$cursor);
    $self->stream();
}

##

sub key__down { 
    my $self = shift;
    my $cursor = $self->{cursor};

    if ($self->current_y < scalar(@{$self->{code}})) {
        my ($maxy, $maxx);
        getmaxyx(stdscr(), $maxy, $maxx);
        if ($cursor->[Y] >= $maxy -1) {
            scrl(1);
            $self->{yscroll}++;
            $self->redraw();
        } else {
            ++$cursor->[Y] if $cursor->[Y] < (MAX_COLUMN - 1);
	    
            my $max = length($self->{code}->[$self->current_y] or '');
            $cursor->[X] = $max if $cursor->[X] > $max;
            move(@$cursor);
	    $self->stream();
        }
    }
}

##

sub key__left { 
    my $self = shift;
    my $cursor = $self->{cursor};
    if ($cursor->[X] > 0) {
        --$cursor->[X] 
    } elsif ($cursor->[Y] > 0) {
        $cursor->[Y]--;
        $cursor->[X] = length($ {$self->current_line});
    }
    move(@$cursor);
    $self->stream();
}

##

sub key__right { 
    my $self = shift;
    my $cursor = $self->{cursor};
    ++$cursor->[X];
    if ($cursor->[X] > length($ {$self->current_line})) {
        $cursor->[X] = 0;
        $cursor->[Y]++;
    }
    move(@$cursor);
    $self->stream();
}

##

sub key__f10 {
    my $self = shift;
    $self->{shot} ||= 0;
    my $code = join("\n", @{$self->{code}});
    my $fn = "/yaxu/shots/$$-" . $self->{shot}++;
    open(FH, ">$fn")
      or die "couldn't open shotfile";
    print(FH $code);
    print(FH "\n");
    addstr(10, 10, "saved $fn");
    close(FH);
}

##

sub key__tab {
    my $self = shift;
    
    my $current_line = $self->current_line;
    
    # do something clever here
    
    my $insert = $self->{cursor}->[X] % 4;
    if ($insert == 0) {
        $insert = 4;
    }
    for (my $c = 0; $c < $insert; ++$c) {
        $self->add(' ');
    }
}

##

sub key__ctrl_left {
    my $self = shift;
    if ($self->{revision}) {
        $self->retrieve_revision(--$self->{revision});
    }
    elsif (not defined $self->{revision}) {
        if (my $total = $self->total_revisions()) {
            $self->retrieve_revision($total);
            $self->{revision} = $total;
        }
    }
}

##

sub key__ctrl_right {
    my $self = shift;

    if (defined $self->{revision}) {
        my $revisions = $self->total_revisions;
        if (($self->{revision} + 1) <= $revisions) {
            $self->retrieve_revision(++$self->{revision});
        }
    }
}

##

sub retrieve_revision {
    my ($self, $revision) = @_;
    return unless $revision;
    $revision = "1.$revision";
    my $filename = quotemeta($self->archive_filename);
    my @text = `co -p$revision -q $filename`;
    chomp @text;
    @{$self->{code}} = @text;
    clear();
    $self->redraw;
}

sub total_revisions {
    my $self = shift;
    my $filename = quotemeta($self->archive_filename);
    my $revisions = `rlog -h $filename |grep "total revisions"`;
    ($revisions) = ($revisions =~ /(\d+)/);
    return($revisions);
}

##

sub crash {
    endwin();
    die(shift);
}

##

1;
