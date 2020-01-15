package old::Old::Line;

use base 'old::Old';

# deviate
# 

sub init {
    my $self = shift;

    $self->SUPER::init(@_);

    $self->{maxhits} 
      = {value      => 2,
	 minimum    => 0,
	 maximum    => 3,
	 format     => "%8d",
	 mutability => 0.5
	};
    $self->{offset} 
      = {value      => 2,
	 minimum    => 0,
	 maximum    => 4,
	 format     => "%8d",
	 mutability => 0.5
	};
    $self->{gap} 
      = {value      => 8,
	 minimum    => 4,
	 maximum    => 8,
	 format     => "%8d",
	 mutability => 0.5
	};
    $self->{vol}
      = {value      => 80,
	 minimum    => 50,
	 maximum    => 100,
	 format     => "%8d",
	 mutability => 10
	};
    $self->{snapsz} 
      = {value      => 1,
	 minimum    => 1,
	 maximum    => 2,
	 format     => "%8d",
	 mutability => 1
	};

    $self->{cutoff} 
      = {value      => 5000,
	 minimum    => 0.1,
	 maximum    => 44100,
	 format     => "%8.2f",
	 mutability => 200
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
	 minimum    => 0.35,
	 maximum    => 0.65,
	 format     => "%.6f",
	 mutability => 0.05
	};
}

##

sub mutate_sequence {
    ['maxhits', 'offset', 'snapsz'];
}

##

sub mutate_note {
    ['vol', 'cutoff', 'res', 'pan', 'gap'];
}

##

1;
