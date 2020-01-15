#!/usr/bin/perl -w

use strict;
use Sandbox;
use Word;

my $velocity_d = sub{1};
my $loss_d = sub{1};
my $alpha_d = sub{0.5};
my $tension_d = sub{1};

my $s = Sandbox->_new({code => undef});

play($s, "he");


sub play {
    my $obj = shift;
    my $word = shift;
    while ($word =~ /([bcdfghjklmnpqrstvwxyz]*)([aeiou])/g) {
        my $cs = $1;
        my $v = $2;
        #$cs = 'w' if !$cs;
        my $h = Word::lookup($v);
        warn("v: $v t: $h->{tension}\n");
        $obj->tension($h->{tension} + $tension_d->() );
        $obj->loss($h->{loss} + $loss_d->());
        foreach my $c (split(//, $cs)) {
            $h = {%{Word::lookup($c)}};
            $h->{velocity} += $velocity_d->();
            $h->{alpha} += $alpha_d->();
            $obj->vector($h);
            sleep(0.005 + rand(0.01));
        }
    }
}
