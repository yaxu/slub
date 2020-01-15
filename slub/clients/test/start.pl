#!/usr/bin/perl -w

use strict;

use Audio::Wav;

my $input = shift @ARGV;
die "no input wav" if (not ($input and -e $input));

my $output = (shift(@ARGV) or 'a.wav');

my $search = (shift(@ARGV) or 16);

my $wav = new Audio::Wav;

my $read = $wav->read($input);

my $total = 0;
my @string = split(//, $read->read_raw( $read->length ));
my $length = scalar(@string);
print("length: " . $read->length . "\n");

my $intervals = find($search, @string);
warn("found " . scalar(keys %$intervals) . " repetitions\n");

undef @string;

my %rint;

while(my ($key, $value) = each(%$intervals)) {
    $key =~ s/(.)\_/$1/g;
    push @{$rint{$value}}, $key;
}

my $write = $wav->write($output, $read->details);

my @buf;
my @keys = keys %rint;
for(my $c = 0; $c < $length; ++$c) {
    print "$c/$length\r";
    foreach my $key (@keys) {
	if(not $c % $key) {
	    my $foo = 0;
	    foreach my $thing (@{$rint{$key}}) {
		foreach my $byte (split(//, $thing)) {
		    push @{$buf[$foo]}, $byte;
		    ++$foo;
		}
	    }
	}
    }
    my $to_write;
    foreach my $buffy (@{shift(@buf) || []}) {
	$to_write += ord($buffy);
	$to_write = $to_write % 128
    }
    $to_write = chr($to_write || 0);
    $write->write_raw($to_write, 1);
}

$write->finish;

sub find {
    my($combo_length, $string) = @_;
    my %look;
    my %intervals;

    # TODO - circular search?
    for(my $c = ($combo_length - 1); $c < scalar(@string); ++$c) {
	my $start = ($c - ($combo_length - 1));
	my $lookup = join('_', @string[$start .. $c]);
	if (exists $look{$lookup}) {
	    my $interval = ($c - $look{$lookup});
	    push(@{$intervals{$lookup}}, $interval);
	}
	$look{$lookup} = $c;
    }

    foreach my $key (keys %intervals) {
	$intervals{$key} = average($intervals{$key});
    }
    return \%intervals;
}

sub average {
    my $values = shift;
    my $result = 0;
    foreach my $value (@$values) {
	$result += $value;
    }
    $result /= @$values;
    return(int($result + 0.5));
}
