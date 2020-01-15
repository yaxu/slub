package Rhythmspace;

use strict;

sub get_rhythm {
  my ($length, $intensity, $disorder) = @_;
  $intensity = int($intensity * $length);

  my $rhythm = jumble($length, $disorder);

  if ($intensity != $length / 2) {
    $rhythm = mask($rhythm, $intensity - ($length / 2));
  }
  
  return($rhythm);
}

sub jumble {
  my($length, $intensity) = @_;
  my $half = $length / 2;

  my $rhythm = [(1, 0) x $half];
  my $steps = tri($half - 1);
  
  my $strength = $steps * $intensity;

  my $offset = 1;


 FOO:
  while($offset < $half and $strength > 0) {
    my $p = $offset;
    while ($p < ($length-1)) {
      ($rhythm->[$p], $rhythm->[$p+1]) = ($rhythm->[$p+1], $rhythm->[$p]);
      $p += 2;
      $strength--;
      last FOO unless $strength > 0;
    }
    $offset++;
  }

  return($rhythm);
}

sub tri {
  my $n = shift;
  my $result = 0;
  foreach my $i (0 .. $n) {
    $result += $i;
  }
  return($result);
}

sub mask {
  my ($rhythm, $n) = @_;
  
  my $switch = 1;
  if ($n < 0) {
    $switch = '0';
    $n = abs($n);
  }
  
  while($n > 0) {
    my $count = 0;
    foreach my $i (@$rhythm) {
      $count++ if $i == $switch;
    }
    my $point = int($count / 2);

    foreach my $j (0 .. (scalar(@$rhythm) - 1)) {
      if($rhythm->[$j] == $switch) {
	if ($point == 0) {
	  $rhythm->[$j] = $switch ? 0 : 1;
	  last;
	}
	$point--;
      }
    }
    --$n;
  }
  return($rhythm);
}

##

1;

