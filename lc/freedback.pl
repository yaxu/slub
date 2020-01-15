#!/usr/bin/perl -w

# by alexatslabdotorg
# http://yaxu.org/

# feedback.pl - a Perl editor for live coding
#
# Copyright (C) Alex McLean 2004
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

use strict;

my $text = '';
if (@ARGV) {
    my $file = shift @ARGV;
    open(FH, "<$file")
      or die "couldn't open '$file': $!\n";
    $text = join('', <FH>);
    close FH;
}

my $editor = Editor->new(text => $text);
$editor->run;

##

package Editor;

# (c) 2004 Alex McLean

use strict;
use Curses;
use Time::HiRes qw/sleep/;

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

    for (my $c = $self->{yscroll}; $c < scalar(@$code); ++$c) {
        addstr($c - $self->{yscroll}, 0, $code->[$c] 
               . (' ' x (MAX_COLUMN - length($code->[$c])))
              );
    }

    move(@{$self->{cursor}});
}

##

sub init_curses {
    initscr();
    start_color();
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
}

sub archive_filename {
    my $self = shift;
    unless ($self->{filename}) {
        my($sec, $min, $hour, $day, $month, $year) = localtime(time);
        $month += 1;
        $year += 1900;
        my $date = sprintf("%04d-%02d-%02d", $year, $month, $day);
        my $dir = "/yaxu/archive/$date/";
        mkdir $dir unless -d $dir;
        mkdir "$dir/RCS" unless -d "$dir/RCS";
        chdir($dir);
        my $file = "$hour:$min:$sec-$$";
    
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
}

##

sub key__backspace {
    my $self = shift;
    my $cursor = $self->{cursor};
    my $line = $self->current_line;
    
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
            }
        }
    }
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
}

##

sub key__ctrl_e {
    my $self = shift;
    my $cursor = $self->{cursor};
    $cursor->[X] = length($self->{code}->[$self->current_y] or '');
    move(@$cursor);
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

sub crash {
    endwin();
    die(shift);
}

##

1;


package Sandbox;

# (c) Alex McLean 2004/02/29

use strict;

use threads;
use threads::shared;

use Time::HiRes qw/ sleep time /;

use Audio::Beep;

use vars qw/$use_osc/;

BEGIN {
    eval {
        require Audio::OSC::Client;
        $use_osc = 1;
    };
    if ($@) {
        $use_osc = 0;
    }
}

##

sub _new {
    my ($pkg, $p) = @_;
    $p ||= {};
    my $self = bless($p, $pkg);
    $self->_init();
    return($self)
}

##

sub _init {
    my $self = shift;
    
    # variables shared with the running program
    my $reload : shared;
    my $modification : shared;
    
    $self->{reload} = \$reload;
    $self->{modification} = \$modification;
    $self->{bangs} = 0;
    $self->{buffer_time} = 0.05;
    $self->bpm(140 * 4);
    $modification = 1;
    
    if ($use_osc) {
        $self->_init_osc;
    }
    else {
        die;
    }
    
    $self->init if $self->can('init');
}

##

sub _init_osc {
    my $self = shift;
    my $osc = 
      Audio::OSC::Client->new(Host => 'localhost', Port => 57120)
          or die "failed to connect to SuperCollider\n";
    $self->{osc} = $osc;
}

##

sub new_thread {
    my $self = shift;
    
    if ($ENV{SPREAD}) {
        threads->create(sub {$self->spread_event_loop} );
    } else {
        # internal clock
        threads->create(sub {$self->event_loop} );
    }
}

##

sub event_loop {
    my $self = shift;
    close(STDERR);
    open(STDERR, '>sandbox.err');
    close(STDOUT);
    close(STDIN);
    my $sleep;
    my $start = time();
    my $bpm = $self->bpm;
    my $interval = 60 / $bpm;
    $self->_do_bang;
    my $bangs = 1;
    while (1) {
        my $new = $self->bpm;
        if ($new and $new != $bpm) {
            $start = $start + ($bangs * $interval);
            $bpm = $new;
            $interval = 60 / $bpm;
            $bangs = 0;
        }
    
        # what time the new bang should be - start time plus number of
        # ticks so far * amount of time per bang
        my $seconds_in = ($start
                          + ($bangs * $interval)
                         );
        # how long til the new bang?
        $sleep = ($seconds_in - time());
        if ($sleep > $interval) {
            # Oops, haven't had this tick yet, sleep until it's time
            $sleep -= $interval;
        } else {
            $self->{now} = $seconds_in;
            $self->_do_bang;
            $self->{bangs} = $bangs++;
        }
        sleep($sleep) unless $sleep <= 0;
    }
}

##

sub spread_event_loop {
    require YAML;
    require Spread::Session;
    
    my $self = shift;
    
    my $start_mod = 4;

    my ($start, $point, $ticks_per_minute, $ticks, @changes);
    
    close(STDERR);
    open(STDERR, '>sandbox.err');
    close(STDOUT);
    close(STDIN);

    my $spread = $self->{spread} = 
      Spread::Session->new(MESSAGE_CALLBACK => 
                           sub {
                               my ($command, $p) = YAML::Load($_[0]->{BODY});
                               if ($command eq 'ticks_per_minute') {
                                   push(@changes, $p);
                                   @changes = 
                                     sort{$a->{ticks} <=> $b->{ticks}}
                                       @changes;
                               } else {
                                   my $func = "on__$command";
                                   if ($self->can($func)) {
                                       $self->$func($_[0], $p);
                                   }
                               }
                           },
                          )
          or die;
    
    $spread->publish('#tm#localhost', YAML::Dump('new_listener'));

    while (not @changes) {
        $spread->poll();
        $spread->receive();
    }
    $spread->subscribe('ticks_per_minute');
    $spread->subscribe('share');

    my $change = shift @changes;
    ($start, $ticks_per_minute, $ticks) = ($change->{start},
                                           $change->{ticks_per_minute},
                                           $change->{ticks},
                                          );
    my $time = time();
    if ($start >= $time) {
        sleep($start - $time);
    }
    
    $point = $start;
    
    my $started = 0;
    while (not $started) {
        if (($ticks % $start_mod) == 0) {
            $started = 1;
        }
    
        $ticks++;
        my $tick_seconds = ($ticks / $ticks_per_minute) * 60;
        $point = $tick_seconds + $start;
        my $sleep = $point - time();
        sleep($sleep) if $sleep > 0;
    }
    
    my $bangs_per_tick = ($self->{bangs_per_tick} ||= 4);
    my $bpm = $bangs_per_tick * $ticks_per_minute;
    my $bangs = 0;
    my $bangs_since_change = 0;
    $start = $point;

    while (1) {
        $self->{now} = $point;
        $self->_do_bang();
    
        if ($spread->poll()) {
            $spread->receive(1);
        }
    
        # --
        # sleep till next bang
    
        $self->{bangs} = $bangs++;
        $bangs_since_change++;
    
        # Recalculate from the start each time so we don't collect errors
        my $bang_seconds = ($bangs_since_change / $bpm) * 60;

        $point = $bang_seconds + $start;    

        my $sleep = $point - time();
        sleep($sleep) if $sleep > 0;
    
        if (($bangs % $bangs_per_tick) == 0) {
            $self->{ticks} = ++$ticks;
            if (@changes and ($changes[0]->{ticks} <= $ticks)) {
                my $change = shift @changes;
                if ($ticks != $change->{ticks}) {
                    warn("processed a ticks_per_minute change " 
                         . ($ticks - $change->{ticks}) 
                         . " ticks too late.\n"
                        );
                }

                $start = $point;
                $ticks_per_minute = $change->{ticks_per_minute};
                $ticks = 0;
                $bpm = $bangs_per_tick * $ticks_per_minute;
                $bangs_since_change = 0;
            }
        }
    }
}

##

sub say {
    my ($self, $command, $p) = @_;
    
    $self->{spread}->publish('share', YAML::Dump($command, $p));
}

##

sub send_bpm {
    my ($self, $ticks_per_minute) = @_;
    
    $self->{spread}->publish('ticks_per_minute', 
                             YAML::Dump('ticks_per_minute', 
                                        {ticks_per_minute => $ticks_per_minute,
                                         ticks => $self->{ticks} + 2,
                                        }
                                       )
                            );
}

##

sub _do_bang {
    my $self = shift;
    if (__PACKAGE__->can('bang')) {
        eval {
            $self->bang();
        };
        if ($@) {
            print(STDERR $@);
            $self->reload(1);
        }
    }
    if ($self->reload) {
        if ($self->interpret) {
            $self->reload(0);
        }
    }
}

##

sub code {
    my $self = shift;
    if (not $self->{code}) {
        my @code;
        share(@code);
        $self->{code} = (\@code);
    }
    return($self->{code});
}

##

sub regex {
    my ($self, $regex, $replacement) = @_;
    
    return unless defined $replacement;
    
    my $code = join("\n", @{$self->code});
    
    $code =~ s/$regex/$replacement/s;
    
    @{$self->code} = split("\n", $code);
}

##

sub modified {
    my $self = shift;
    $ {$self->{modification}}++;
}

##

sub play {
    my $self = shift;
    my ($num, $gain, $pan, $formfreq, $bwfreq, $ts, $offset, $crackle, $browndel, $env);
    
    if (ref($_[0])) {
        ($num, $gain, $pan, $formfreq, $bwfreq, 
         $ts, $offset, $crackle, $browndel, $env) = 
           map {$_[0]->{$_}}
             qw{num gain pan formfreq bwfreq ts offset crackle browndel env};
    } else {
        ($num, $formfreq, $bwfreq, 
         $ts, $offset, $pan, $gain, $crackle, $browndel, $env) = @_;
    }
    
    $formfreq ||= 10;
    $bwfreq ||= 0;
    
    $pan = 0.5 if not defined $pan;

    $offset ||= 0;
    
    $num ||= 160;
    $browndel ||= 0;
    
    if (not defined $env) {
        $env = 1;
    }
    
    my $osc = $self->{osc};

    $gain ||= 100;
    my ($lgain, $rgain) = ($gain, $gain);
    if ($pan > .5) {
        $lgain *= (1 - (($pan - .5) * 2));
    } elsif ($pan < 0.5) {
        $rgain *= ($pan * 2);
    }
    $crackle ||= 10;
    $ts ||= 20;
    
    $osc->send(['#bundle', 
                $self->{now} + $self->{buffer_time} + $offset,
                ['/play',
                 's', 'on',
                 'i', $num, 
                 'i', $formfreq,
                 'i', $bwfreq,
                 'i', $ts,
                 'i', $lgain,
                 'i', $rgain,
                 'i', $crackle,
                 'i', $browndel,
                 'i', $env      
                ]
               ]
              );
}

##

sub trigger {
    my $self = shift;
    my ($sample, $gain, $offset, $pan, $crackle, $noise, $ts, $browndel, $rate, $env);
    
    if (ref($_[0])) {
        ($sample, $gain, $offset, $pan, $crackle, $noise, $ts, $browndel, $rate, $env) = 
          map {$_[0]->{$_}}
            qw{ sample gain offset pan crackle noise ts browndel rate env };
    } else {
        ($sample, $gain, $offset, $pan, $crackle, $noise, $ts, $browndel, $rate, $env) = @_;
    }
    
    $pan = 0.5 if not defined $pan;

    $offset ||= 0;

    $noise ||= 0;
    $browndel ||= 0;
    $rate ||= 100;
    $env ||= 0;
    
    return unless $sample;
    
    my $osc = $self->{osc};

    $gain ||= 100;
    my ($lgain, $rgain) = ($gain, $gain);
    if ($pan > .5) {
        $lgain *= (1 - (($pan - .5) * 2));
    } elsif ($pan < 0.5) {
        $rgain *= ($pan * 2);
    }
    $crackle ||= 10;
    $ts ||= 20;
    
    if ($sample !~ m,^/,) {
        $sample = '/yaxu/samples/' . $sample; 
    }

    if ($sample =~ m,^(.+?/)(\d+)$,) {
        my ($dir, $number) = ($1, $2);
        my $samples = $self->{sample_cache}->{$sample};
        if (not $samples) {
            opendir(DIR, $dir)
              or return;
            $samples = 
              $self->{sample_cache}->{$sample} =
                [grep {/\.[Ww][Aa][Vv]$/} 
                 readdir(DIR)
                ];

            closedir(DIR);
        }
        return unless @$samples;
        $sample = $dir . $samples->[$number % @$samples];
    }

    $osc->send(['#bundle',
                $self->{now} + $self->{buffer_time} + $offset,
                ['/trigger',
                 's', 'on',
                 's', $sample, 
                 'i', $lgain,
                 'i', $rgain,
                 'i', $crackle,
                 'i', $noise,
                 'i', $ts,
                 'i', $browndel,
                 'i', $rate,
                 'i', $env
                ]
               ]
              );
}

##

sub interpret {
    my $self = shift;
    my $result;
    my $code = join("\n", @{$self->code});
    no warnings 'redefine';
    
    eval("package Sandbox::Test; $code");
    if ($@) {
        print(STDERR $@);
    } elsif (not Sandbox::Test->can('bang')) {
        eval("test failed - no bang!");
    } else {
        eval("package Sandbox; $code");
        $result = 1;
    }
    
    if ($self->can('on_interpret')) {
        $self->on_interpret;
    }
    return($result);
}

##

sub reload {
    return(@_ > 1 ? ($ {$_[0]->{reload}} = $_[1]) : $ {$_[0]->{reload}});
}

##

sub modification {
    return(@_ > 1 
           ? ($ {$_[0]->{modification}} = $_[1]) 
           : $ {$_[0]->{modification}}
          );
}

##

sub bpm {
    my $self = shift;
    if (@_) {
        $self->{bpm} = shift;
    }
    return($self->{bpm});
}

##

sub sin {
    my $self = shift;
    my $div = shift;
    $div ||= 1;
    ((CORE::sin($self->{bangs} / $div) + 1) / 2)
}

##

1;
