#!/usr/bin/perl -w

use strict;

use Tk;
use Tk::Canvas;
use Tk::Photo;
use IO::Handle;
use Client;

my $granularity = 64;

my $client;

my @ctrls = qw/ recap /;

my %remote_ctrls;

my $tick;
my $p;

my @colours;
{
    local $^W = 0;
    @colours = qw(
		  #c0c0c0
		  #b0b0b0
		  #a0a0a0
		  #909090
		  #808080
		  #707070
		  #606060
		  #505050
		  #404040
		  #484848
		  #303030
		  #383838
		  #202020
		  #282828
		  #101010
		  #181818
		  #000000
		 );
}

init();
MainLoop();

sub init {
    $tick = 0;

    $client = Client->new(port	    => 6010,
			  on_clock  => \&on_clock,
			  on_ctrl   => \&on_ctrl,
			  timer     => 'ext',
			  tpb	    => 16,
			  ctrls     => \@ctrls,
			 );

    my $main = MainWindow->new();
    $main->wm("geometry", "100x100");
    $main->fileevent($client->sock(), readable => sub {$client->poll});

    $p = $main->Photo(-width => 100,
		      -height => 100,
		     );

    $main->Label(-background => 'white',-image => $p)->pack;
}

##

sub on_clock {
    $tick++
}

##

{
    my %ctrl_lookup;
    my @ctrls;
    my @history;

    sub on_ctrl {
	my ($local_control, $remote_ctrl, @data) = @_;
	
	if (not exists $ctrl_lookup{$remote_ctrl}) {
	    add_ctrl($remote_ctrl);
	}
	paint_ctrl($remote_ctrl, \@data);
    }

    ##

    sub add_ctrl {
	my $ctrl = shift;
	push @ctrls, {name => $ctrl
		     };
	$ctrl_lookup{$ctrl} = $#ctrls;
    }

    ##

    sub paint_ctrl {
	my($remote_ctrl, $data) = @_;

	push(@{$history[$tick]}, [$remote_ctrl, @$data]);
	
	my $start = $tick - $tick % $granularity;
	my $stop = $start + $granularity - 1;

	my $count;
	foreach my $blocktick ($start .. $stop) {
	    last if $blocktick >= @history;
	    if ($history[$blocktick]) {
		$count += scalar(@{$history[$blocktick]});
	    }
	}
	# hmm - this will never get at element #1...

	my $colour = $colours[($count >= @colours) ? $#colours : $count];
	my $y = $ctrl_lookup{$remote_ctrl};
	my $x = int($tick / $granularity);
	
	$p->put($colour, -to => ($x, $y, $x + 1, $y + 1));
	#$p->put($colour, -to => qw/10 10 30 30/);
	
    }
}

##

1;
