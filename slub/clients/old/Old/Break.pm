package old::Old::Break;

use base 'old::Old';

# deviate
# 

sub init {
    my $self = shift;

    $self->SUPER::init(@_);

    $self->{maxhits} 
      = {value      => 2,
	 minimum    => 0,
	 maximum    => 8,
	 format     => "%8d",
	 mutability => 0.8
	};
    $self->{offset} 
      = {value      => 0,
	 set        => [0, 8],
	 format     => "%8d",
	 mutability => 0.5
	};
    $self->{gap} 
      = {set        => [6, 8, 10],
	 format     => "%8d",
	 mutability => 0.75
	};
    $self->{vol} 
      = {value      => 10,
	 minimum    => 1,
	 maximum    => 127,
	 format     => "%8d",
	 mutability => 0.25
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
	 maximum    => 20000,
	 format     => "%8.2f",
	 mutability => 400,
	};
    $self->{res} 
      = {value      => 0,
	 minimum    => 0,
	 maximum    => 0.75,
	 format     => "%.6f",
	 mutability => 0.1
	};
    $self->{pan}
      = {value      => 0,
	 minimum    => 0.05,
	 maximum    => 0.95,
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
