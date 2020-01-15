#!/usr/bin/perl -w

use strict;

package foo;

my $self = bless {}, __PACKAGE__;

my $foo = 0;

my @studied = map { [$_ => $self->study($_)] } $self->comb(8);

my @by_regularity = 
  map  { $_->[0] }
  sort { $b->[1] <=> $a->[1]
	   || $a->[2] <=> $b->[2]
       }
  map  { [$_->[0], $_->[1]->{regularity}, $_->[1]->{count}] }
  @studied;

my @on_by_regularity = 
  grep {substr($_, 0, 1) eq '1'}
  @by_regularity;
my @off_by_regularity = 
  grep {substr($_, 0, 1) eq '0'}
  @by_regularity;

foreach my $line (@on_by_regularity) {
  print "$foo $line\n";
  $foo++;
}

sub study {
  my ($self, $line) = @_;
  
  my %stats;
  
  $stats{on} = (substr($line, 0, 1) eq '1' ? 1 : 0);
  
  $stats{count} = $line =~ tr/1/1/;
  
  $stats{distinct} = (($line =~ /11/) ? 0 : 1);
  
  my $regularity;
  my $half = length($line) / 2;
  my $first = substr($line, 0, $half);
  my $second = substr($line, $half, $half);
  if ($first eq $second) {
    $regularity += 2;
  }
  if (($first =~ tr/1/1/) == ($second =~ tr/1/1/)) {
    $regularity += 2;
  }
  foreach my $point (0 .. ($half - 1)) {
    if (substr($first, $point, 1) eq substr($second, $point, 1)) {
      $regularity++;
    }
  }
  $stats{regularity} = $regularity || 0;
  return(\%stats);
}

sub comb {
  my ($self, $length) = @_;
  my @result;
  if ($length > 1) {
    @result = map {'1' . $_, '0' . $_} $self->comb($length - 1);
    
  }
  else {
    @result = (1, 0);
  }
  return(@result);
}
