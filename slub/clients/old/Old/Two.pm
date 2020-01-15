package old::Old::Two;

use base 'old::Old';

sub init {
    my $self = shift;

    $self->SUPER::init(@_);

    $self->{maxhits} 
       = {value      => 0,
	  set        => [4, 6, 8, 16],
	  format     => "%8d",
	  mutability => 0.25
	 };
    $self->{offset} 
      = {value      => 0,
	 set        => [0, 2, 4, 8],
	 format     => "%8d",
	 mutability => 0.25
	};
    $self->{gap}
      = {value      => 0,
	 set        => [1, 2, 4],
	 format     => "%8d",
	 mutability => 0.25
	};
    $self->{vol} 
      = {value      => 100,
	 minimum    => 10,
	 maximum    => 127,
	 format     => "%8d",
	 mutability => 10
	};
    $self->{snapsz} 
      = {value      => 1,
	 minimum    => 1,
	 maximum    => 1,
	 format     => "%8d",
	 mutability => 0
	};
    $self->{cutoff} 
      = {value      => 100,
	 minimum    => 0.1,
	 maximum    => 44100,
	 format     => "%8.2f",
	 mutability => 400,
	 deviate    => 200
	};
    $self->{res} 
      = {value      => 0,
	 minimum    => 0,
	 maximum    => 0.8,
	 format     => "%.6f",
	 mutability => 0.1
	};
    $self->{pan}
      = {value      => 0.5,
	 minimum    => 0.40,
	 maximum    => 0.60,
	 format     => "%.6f",
	 mutability => 0.05
	};
}

##

sub mutate_sequence {
    ['maxhits', 'offset', 'snapsz', 'gap'];
}

##

sub mutate_note {
    ['vol', 'cutoff', 'res', 'pan'];
}

##

1;
