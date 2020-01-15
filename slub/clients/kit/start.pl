#!/usr/bin/perl

package Main;

##

use strict;

use Client;
use Data::Dumper;

##

my %samples;
my @kits = qw/ house glitch jazz /;
my @sounds  = qw/ BD CB FX HH OH P1 P2 SN /;

my $client;
my @edits;
my $edit;
my $edit_buffer = '';
my @eeps;
my $box;
my $c_kit = 'house';

init();

sub init {
    $client = Client->new(port      => 6011,
			  on_clock  => \&on_clock,
			  timer     => 'ext',
			  tpb       => ($ENV{TPB} || 8)
			 );
    
    foreach my $kit (@kits) {
	$client->init_samples($kit);
    }

    $client->event_loop;
}

my $tick;
sub on_clock {
    $tick ||= 0;
    foreach my $sound (@sounds) {
	Main->$sound();
    }
    ++$tick;
}

##

sub BD {
    $client->play_sample($c_kit . '_BD')
      if not $tick % 10;
}
sub CB {
}
sub FX {
}
sub HH {
}
sub OH {
}
sub P1 {
}
sub P2 {
}
sub SN {
}

##

1;

