#!/usr/bin/perl -w

use strict;

use Net::OpenSoundControl::Client;

my $osc = Net::OpenSoundControl::Client->new(Host => 'localhost', Port => 7770)
    or die "could't connect to datadirt";

trigger();

sub trigger {
    my ($self, $p) = @_;
    
    # path to the sample
    my $sample = $p->{sample} || "drumtraks/011_DT Tom1.wav";

    # 1 for normal, -1 for backwards, other numbers as you might imagine
    my $speed  = $p->{speed} || 1;

    # wave shaping - floating point numbers approaching and beyond 1
    # get very grungy. 0 for a clean, un-messed with sound.
    my $shape  = $p->{shape} || 0;

    # -1 to 1 from hard left to hard right, or 0 for centre
    my $pan    = $p->{pan} || 0;
    my $pan_to = $p->{pan_to};

    # there is heavy compression, so this is *relative* volume
    my $volume = $p->{volume};

    # volume envelopes
    my $envelope_name = $p->{envelope_name} || 'percussive';
	
    # these two don't actually work
    my $anafeel_strength  = $p->{anafeel_strength} || 0;
    my $anafeel_frequency = $p->{anafeel_frequency} || 0;
    
    # sample playback gets faster towards the end
    my $accellerate       = $p->{accellerate} || 0;
    
    # formant filter, pass 'a', 'e', 'i', 'o' or 'u'
    my $vowel             = $p->{vowel} || 0;
    
    # i think you can pass 'equal' here and use the speed parameter as
    # pitch
    my $scale             = $p->{scale} || 0;

    # number of times to play the loop.  0.5 only plays half the sample...
    my $loops             = $p->{loops} || 1;

    # number of samples play from the sample.  incompatible with loops
    my $duration          = $p->{duration} || 0;

    # a funky delay filter
    my $delay             = $p->{delay} || 0;
    my $delay2            = $p->{delay2} || 0;

    my $cutoff            = $p->{cutoff} || 0;
    my $resonance         = $p->{resonance} || 0;
    
        
    if (not defined $volume) {
	$volume = 0.5;
    }

    if (not defined $pan_to) {
	$pan_to = $pan;
    }
        
    # don't play a volume of 0
    return unless $volume;
        
        
    $osc->send(['/trigger',
		's', $sample, 
		'f', $speed, 
		'f', $shape, 
		'f', $pan,
		'f', $pan_to,
		'f', $volume,
		's', $envelope_name,
		'f', $anafeel_strength,
		'f', $anafeel_frequency,
		'f', $accellerate,
		's', $vowel,
		's', $scale,
		'f', $loops,
		'f', $duration,
		'f', $delay,
		'f', $delay2,
		'f', $cutoff,
		'f', $resonance
	       ]
	);
}
