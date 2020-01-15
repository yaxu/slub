#!/usr/bin/perl

package agpl;

use strict;

use Client;

use Ag::Env;
use Ag::Feep::Simple;
use Ag::Feep::Racist;
use Ag::Feep::Loner;

use Data::Dumper;

my @ctrls = qw/ bear /;

my $c = Client->new(port     => 6010,
		    ctrls    => \@ctrls,
		    on_ctrl  => \&on_ctrl,
		    on_clock => \&on_clock,
		    timer    => 'ext',
		    tpb      => 16,
		   );
my @lighter = $c->init_samples('lighter');
my @bassdm  = $c->init_samples('bassdm');
my @mouth   = $c->init_samples('mouth');

my $env = Ag::Env->new({width => 16, height => 8});

$env->introduce(Ag::Feep::Simple->new(sample => $lighter[rand @lighter]));
$env->introduce(Ag::Feep::Simple->new(sample => $lighter[rand @lighter]));
$env->introduce(Ag::Feep::Simple->new(sample => $lighter[rand @lighter]));

$env->introduce(Ag::Feep::Racist->new(sample => $bassdm[rand @bassdm]));
$env->introduce(Ag::Feep::Racist->new(sample => $bassdm[rand @bassdm]));

$env->introduce(Ag::Feep::Loner->new(sample => $mouth[rand @mouth]));
$env->introduce(Ag::Feep::Loner->new(sample => $mouth[rand @mouth]));
$env->introduce(Ag::Feep::Loner->new(sample => $mouth[rand @mouth]));

$env->drop_feeps;

$c->event_loop;


sub on_ctrl {
    my ($ctrl, @data) = @_;
    my $func = "on__$ctrl";
    if (agpl->can($func)) {
	&$func(@data);
    }
}

sub on__ag_bear {
    my ($type) = @_;
    
    my $pkg = "Ag::Feep::$type";
    if ($pkg->can('new')) {
	my $feep = $pkg->new();
	$env->introduce($feep);
    }
}

sub on_clock {
    my $feeps = $env->cell;

    foreach my $feep (@$feeps) {
	$c->play_sample($feep->sample);
    }
    $env->inc_cell;
}

##

1;
