#!/usr/bin/perl -w

use strict;
use YAML;

my $self = Rd04->new;

my $width = 79;

$self->curses_init;
$self->load_schedule;
$self->event_loop;

package Rd04;

use Curses;

use POSIX qw/ mktime strftime /;
use YAML;
use Time::HiRes qw/sleep time/;

##

sub new {
    my $pkg = shift;
    
    bless({
           start_day    => 25,
           start_point => mktime(0, 0, 0, 25, 7, 104),
           warp         => [],
           tz           => 2 * 60 * 60,
           offset       => 2 * 60 * 60
          },
          $pkg
         );
}

##

sub event_loop {
    use YAML;
    
    my $self = shift;
    
    my $start_mod = 4;
    
    my ($start, $point, $ticks_per_minute, $ticks);
    
    close(STDERR);
    open(STDERR, '>>/tmp/rd04.err');
    
    ($start, $ticks_per_minute, $ticks) = (time(), 60, 0);
    $self->{point} = $point = $start;
    
    my $bangs_per_tick = ($self->{bangs_per_tick} ||= 1);
    my $bangs_per_minute = $bangs_per_tick * $ticks_per_minute;
    $self->{bangs} = 0;
    $self->{ticks} = $ticks;
    my $bangs_since_change = 0;
    $start = $point;
    
    while (1) {
        $self->{now} = $point;
        $self->do_bang();
        
        # --
        # sleep till next bang
        
        $self->{bangs} = $self->{bangs}++;
        $bangs_since_change++;
        
        # Recalculate from the start each time so we don't collect errors
        my $bang_seconds = ($bangs_since_change / $bangs_per_minute) * 60;
        
        $self->{point} = $point = $bang_seconds + $start;	
        
        my $sleep = $point - time();

        Time::HiRes::sleep($sleep) if $sleep > 0;
        
        if (($self->{bangs} % $bangs_per_tick) == 0) {
            $self->{ticks} = ++$ticks;
        }
        $self->load_schedule
          if $self->{ticks} % 10 == 0;
    }
}

##

sub do_bang {
    my $self = shift;
    $self->display();

    while((my $ch = getch()) ne ERR) {
        if    ($ch eq KEY_LEFT)      { $ch = 'left'      }
	    elsif ($ch eq KEY_RIGHT)     { $ch = 'right'     }
	    elsif ($ch eq KEY_UP)        { $ch = 'up'        }
	    elsif ($ch eq KEY_DOWN)      { $ch = 'down'      }
        elsif ($ch eq ' ')           { $ch = 'space'     }
        my $func = "key__$ch";
        if ($self->can($func)) {
            $self->$func();
        }
    }
    refresh();
}

##

sub key__space {
    my $self = shift;

    if ($self->{next_warp}) {
        my $warp;

        if (@{$self->{warp}}) {
            push @{$self->{warp}}, $self->{warp}->[-1] + $self->{next_warp};
        }
        else {
            push @{$self->{warp}}, $self->{next_warp};
        }
        $self->refresh_offset;
    }
}

##

sub key__up {
    my $self = shift;
    if ($self->{warp} and @{$self->{warp}}) {
        pop @{$self->{warp}};
    }
    $self->refresh_offset;
}

##

sub key__left {
    my $self = shift;
    if (@{$self->{warp}}) {
        push @{$self->{warp}}, $self->{warp}->[-1] - 60;
    }
    else {
        push @{$self->{warp}}, - 60;
    }
    $self->refresh_offset;
}

##

sub key__right {
    my $self = shift;
    if (@{$self->{warp}}) {
        push @{$self->{warp}}, $self->{warp}->[-1] + 60;
    }
    else {
        push @{$self->{warp}}, 60;
    }
    $self->refresh_offset;
}

##

sub refresh_offset {
    my $self = shift;
    my $offset = $self->{tz};
    if (@{$self->{warp}}) {
        $offset += $self->{warp}->[-1];
    }
    $self->{offset} = $offset;
    $self->parse_schedule;
}

##

sub curses_init {
    initscr;
    cbreak;
    noecho;
    keypad 1;
    nonl;
    nodelay 1;
}

##

sub parse_schedule {
    my $self = shift;
    my $day = 0;
    my $hour;
    my $minute;
    my $category;
    my $result;
    my $padding = 'none';

    foreach my $line (`cat /yaxu/rd04/schedule.txt`) {
        chomp $line;
        if ($line =~ /DAY\s*(\d)/) {
            $day = $1;
        }
        elsif ($line =~ m{^\s*\=\s*(\d\d):(\d\d)}) {
            $hour = $1;
            $minute = $2;
            $padding = 'none';
        }
        elsif ($line =~ m{^\s*==\s*([^=]+)}) {
            $category = $1;
        }
        elsif ($line =~ m{^\*\s*(.+?)\s*(\d+)\s*min}) {
            my $start = ($self->{start_point} 
                         + (($day - 1) * 60 * 60 * 24) 
                         + (($hour * 60) + $minute) * 60
                        );
            $minute += $2;
            while ($minute >= 60) {
                $hour++;
                $minute -= 60;
            }
            while ($hour > 23) {
                $hour -= 24;
            }
            
            my $stop = ($self->{start_point} 
                        + (($day - 1) * 60 * 60 * 24) 
                        + (($hour * 60) + $minute) * 60
                       );
            push(@$result,
                 {start    => $start,
                  padding  => 5 * 60,
                  stop     => $stop,
                  category => $category,
                  title    => $1
                 }
                );
            
            $padding = 5 * 60;
                    
        }       
    }
    $self->{schedule} = $result;
    return $result;
}

##

sub display {
    my $self = shift;

    my $day  = $self->{start_day} - (gmtime(time()+$self->{offset}))[3] + 1;
    my $schedule = $self->{schedule};
    my $ticks  = $self->{ticks};
    
    my $point = $self->{point};

    my $shown = 0;
    my $padding = 0;
    my $active = 0;
    my $first = 0;
    $self->{next_warp} = 0;
    
    foreach my $presentation (@{$self->{schedule}}) {
        my $first = $presentation->{start};
        my $start = $presentation->{start} + $padding;
        my $stop  = $presentation->{stop} + $padding;
        my $questions = 0;
        my $current = 0;
        if (($stop - $self->{offset}) >= $point) {
            $current = 1;
        }
        elsif ($presentation->{padding} ne 'none') {
            if ((($stop - $self->{offset}) + $presentation->{padding}) >= $point) {
                $questions = 1;
                $current = 1;
                $stop += $presentation->{padding};
            }
        }
        $presentation->{category};
            
        my $text = '';

        if ($current) {
            if (($start - $self->{offset}) >= $point) {
                
                my $remaining = $start - $point - $self->{offset};
                $self->{next_warp} ||= $remaining;
                $text = sprintf("%s %-25s (starting in %s%s%ds)",
                                strftime("%H:%M:%S -", 
                                         gmtime($start)
                                        ),
                                $presentation->{title},
                                int($remaining / 60 / 60)
                                ? int($remaining / 60 / 60) . 'h '
                                : '',
                                int($remaining / 60)
                                ? (int($remaining / 60) % 60) . 'm '
                                : '',
                                $remaining % 60
                               );
            } else {
                my $remaining = $stop - $point - $self->{offset};
                $text = sprintf("%s %-25s (finishing in %s%s%ds)",
                                strftime("%H:%M:%S -", 
                                         gmtime($start)
                                        ),
                                $questions
                                ? "*questions*"
                                : $presentation->{title}
                                ,
                                int($remaining / 60 / 60)
                                ? int($remaining / 60 / 60) . 'h '
                                : '',
                                int($remaining / 60)
                                ? (int($remaining / 60) % 60) . 'm '
                                : '',
                                $remaining % 60
                               );
                warn("remaining: $remaining\n");
                if (int($remaining) == (2 * 60)) {
                    system("play /yaxu/rd04/dorkbeep.aif 2>&1 >/dev/null");
                }
                elsif (int($remaining) == 1) {
                    warn("play!\n");
                    system("play /yaxu/rd04/dorkbeep.aif 2>&1 >/dev/null");
                }
            }
            $shown++;
        }
            
        if (length($text) > $width) {
            $text = substr($text, 0, $width);
        } elsif (length($text) < $width) {
            $text .= ' ' x ($width - length($text));
        }
            
        my $y = ($shown > 1) ? 2 + ($shown * 2) : 4;

        addstr($y, 0, $text);
        addstr(3, 11, 'category: ' . $presentation->{category})
          if $shown == 1;
            
        last if $shown == 5;

        addstr(1, 0, 
               '[ Runme Dorkbot Citycamp |'
               . strftime(" %H:%M:%S rd04 time |", 
                          gmtime($point + $self->{offset}),
                         )
               . strftime(" %H:%M:%S real time ]", 
                          gmtime($point + $self->{tz}),
                         )
              );

        if ($presentation->{padding} eq 'none') {
            $padding = 0;
        }
        else {
            $padding += $presentation->{padding};
        }
    }
}

sub load_schedule {
    my $self = shift;
    if ($ENV{YAML}) {
        local $/ = undef;
        my $fn = "/yaxu/rd04/schedule.yaml";
        open(FH, "<$fn") or die "Couldn't open schedule: $!";
        $self->{schedule} = Load(<FH>);
        close FH;
    }
    else {
        return($self->parse_schedule);
    }
    
    return($self->{schedule});
}
