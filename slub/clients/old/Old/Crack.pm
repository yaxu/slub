package old::Old::Crack;

use base 'old::Old';

# deviate
# 

sub init {
    my $self = shift;

    $self->SUPER::init(@_);

    $self->{maxhits} 
      = {value      => 2,
	 minimum    => 1,
	 maximum    => 3,
	 format     => "%8d",
	 mutability => 0.1
	};
    $self->{offset} 
      = {value      => 4,
	 minimum    => 4,
	 maximum    => 4,
	 format     => "%8d",
	 mutability => 0
	};
    $self->{gap} 
      = {value      => 8,
	 minimum    => 8,
	 maximum    => 8,
	 format     => "%8d",
	 mutability => 0
	};
    $self->{vol} 
      = {value      => 100,
	 minimum    => 1,
	 maximum    => 127,
	 format     => "%8d",
	 mutability => 0.5
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
	 mutability => 200
	};
    $self->{res} 
      = {value      => 0,
	 minimum    => 0,
	 maximum    => 0.7,
	 format     => "%.6f",
	 mutability => 0.1
	};
    $self->{pan}
      = {value      => 0,
	 minimum    => 0.35,
	 maximum    => 0.65,
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
