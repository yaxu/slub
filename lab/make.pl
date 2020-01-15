#!/usr/bin/perl -w

use strict;

my $output = $ARGV[0] || '/tmp/output.wav';
my $sample_rate = 44100;
my $channels    = 1;
my $bits_sample = 16;
my $pi = 3.14159265358979323846;

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
    my $start_hz0 = 1210;
    my $stop_hz0 = 600;
    my $start_hz1 = 1200;
    my $stop_hz1 = 510;
    
    my $length = 0.1 * $sample_rate;

    my $max_no =  ( 2 ** $bits_sample ) / 2;
    my $vol = 0.5;
    for my $pos ( 0 .. $length ) {
	my $time = $pos / $sample_rate;
	my $point = $pos / $length;

	my $hz = $start_hz0 + (($stop_hz0 - $start_hz0) * $point);
	my $val = sin($pi * $time * $hz);

	
	$hz = $start_hz1 + (($stop_hz1 - $start_hz1) * $point);
	$val += sin($pi * $time * $hz);

	my $noisepart = 0;
	#if ($point < $noisepart) {
	#    $val *= (($point * (1 / $noisepart))) 
	#      + rand(1 - ($point * (1 / $noisepart)));
	#}
	
	my $samp = $val * $max_no;
	
	$write->write($samp * $vol);
    }
}

$write->finish();
