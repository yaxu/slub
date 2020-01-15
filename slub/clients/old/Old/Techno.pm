package old::Old::Techno;

use base 'old::Old';

# deviate
# 

sub init {
    my $self = shift;

    $self->SUPER::init(@_);

    $self->{maxhits} 
      = {value      => 1,
	 minimum    => 1,
	 maximum    => 4,
	 format     => "%8d",
	 mutability => 0.2
	};
    $self->{offset} 
      = {value      => 0,
	 minimum    => -1,
	 maximum    => 1,
	 format     => "%8d",
	 mutability => 0.1
	};
    $self->{gap} 
      = {set        => [4, 4, 4, 8, 8, 12],
	 format     => "%8d",
	 mutability => 0.25
	};
    $self->{vol} 
      = {value      => 100,
	 minimum    => 50,
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
	 maximum    => 22050,
	 format     => "%8.2f",
	 mutability => 150,
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
