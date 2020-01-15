#!/usr/bin/perl -w

use strict;

my $output = $ARGV[0] || '/tmp/output.wav';
my $sample_rate = 44100;
my $channels    = 1;
my $bits_sample = 16;
my $pi = 3.14159265358979323846;
my $seconds = 8;
my $gap_seconds = 0.09;
my $gap = ($sample_rate * $gap_seconds);

use Audio::Wav;
my $wav = new Audio::Wav;
my $write = $wav->write($output,
			{
			 bits_sample   => $bits_sample,
			 sample_rate   => $sample_rate,
			 channels      => $channels
			}
		       );
make();

sub make {
    my $max_hz0 = 1810;
    my $min_hz0 = 1400;
    my $diff_hz0 = $max_hz0 - $min_hz0;
    my $hz1 = 20;
    
    my $length = $seconds * $sample_rate;

    my $max_no =  ( 2 ** $bits_sample ) / 2;
    
    my $playing = 0;
    my $last;
    my $vol = 1;
    
    for my $pos ( 0 .. $length ) {
	my $time = $pos / $sample_rate;
	my $point = $pos / $length;

	my $ping = ((1 + sin($pi * $point * $hz1)) / 2);
	my $hz = $min_hz0 + ($diff_hz0 * $ping);
	#print "$hz\n";
	my $val;
	$val = sin($pi * $hz * $time);
	
	if (($pos % ($gap * 2)) > $gap) {
	    if ($vol < 1) {
		$vol += 0.001;
	    }
	}
	elsif ($vol > 0) {
	    $vol -= 0.001;
	}
	
	if ($vol > 0) {
	    $val *= $vol;
	}
	else {
	    $val = 0;
	}

	my $samp = $val * $max_no;
	
	$write->write($samp * 0.5);
    }
}

$write->finish();
