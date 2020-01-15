#!/usr/bin/perl -w

# by alexatslabdotorg
# http://yaxu.org/

# tm.pl - a spread client for keeping tabs on time
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

use Spread::Session;
use YAML;
use Time::HiRes qw/ time sleep /;


my $spread = 
  Spread::Session->new(private_name     => 'tm',
                       TIMEOUT          => 3,
                       MESSAGE_CALLBACK => 
                       sub {
                           my ($command, $p) = Load($_[0]->{BODY});
                           my $func = "command__$command";
                           if (__PACKAGE__->can($func)) {
                               __PACKAGE__->$func($_[0], $p);
                           }
                           else {
                               warn("bad command: $command\n");
                           }
                       }
                      );

$spread->subscribe('ticks_per_minute');
$spread->subscribe('midi');

my $start_mod = 16;
my $start = time();
my $ticks_per_minute = 120;

my $ticks = 0;
my $total_ticks = 0;
my $point = $start;
my $message;
my @events;

while(1) {
    while ($spread->poll) {
        $spread->receive();
    }
    
    $ticks++;
    $total_ticks++;
    my $tick_seconds = ($ticks / $ticks_per_minute) * 60;
    
    $point = $tick_seconds + $start;
    my $sleep = $point - time();
    if ($message) {
        undef $message;
    }
    sleep($sleep)
      if $sleep > 0;
    
    while (@events and ($events[0]->{ticks}) <= $ticks) {
        
        my $event = shift @events;
        
        if ($ticks != $event->{ticks}) {
            warn("processed a ticks_per_minute event " 
                 . ($ticks - $event->{ticks}) 
                 . " ticks too late.\n"
                );
        }
        if ($event->{ticks_per_minute}) {
            $start = $point;
            $ticks = 0;
            $ticks_per_minute = $event->{ticks_per_minute};
        }
        elsif ($event->{midi}) {
        }
    }
}

##

sub command__new_listener {
    my ($pkg, $response, $p) = @_;
    my $sender = $response->{SENDER};
    $spread->publish($sender, Dump('ticks_per_minute',
                                   {start => $start,
                                    ticks_per_minute   => $ticks_per_minute,
                                    ticks => $ticks,
                                    total_ticks => $total_ticks,
                                   }
                                  )
                    );
}

##

sub command__ticks_per_minute {
    my ($pkg, $response, $p) = @_;
    
    push(@events, $p);
    @events = 
      sort{$a->{ticks} <=> $b->{ticks}}
        @events;
}

sub command__midi {
    my ($pkg, $response, $p) = @_;
    warn("got midi for $p->{ticks} when the time is $ticks\n");
    push(@events, $p);
    @events = 
      sort{$a->{ticks} <=> $b->{ticks}}
        @events;
}

