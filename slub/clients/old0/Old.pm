package old::Old;

use strict;

##

use Class::MethodMaker 
  new_with_init => 'new',
  get_set => [qw{maxhits gap vol cutoff res pan snapsize offset 
		 loopsize sample pitch
		}
	     ];


##

sub init {
    my $self = shift;
    my %p = @_;
    
    # seed values
    $self->{sample}   = ($p{sample} or die "no sample");
    $self->{loopsize} = ($p{loopsize} or 64);
    $self->{pitch}    = ($p{pitch} or 60);
    
    $self->{maxhits} 
      = {value      => 8,
	 minimum    => 1,
	 maximum    => 16,
	 mutability => 0.1
	};
    $self->{gap} 
      = {value      => 8,
	 minimum    => 1,
	 maximum    => 12,
	 mutability => 3
	};
    $self->{vol} 
      = {value      => 100,
	 minimum    => 50,
	 maximum    => 127,
	 mutability => 10
	};
    $self->{snapsize} 
      = {value      => 1,
	 minimum    => 1,
	 maximum    => 16,
	 mutability => 0.1
	};
    $self->{offset} 
      = {value      => 8,
	 minimum    => 1,
	 maximum    => 16,
	 mutability => 0.1
	};
    $self->{cutoff} 
      = {value      => 100,
	 minimum    => 0.1,
	 maximum    => 44100,
	 mutability => 100
	};
    $self->{res} 
      = {value      => 0,
	 minimum    => 0,
	 maximum    => 1,
	 mutability => 0.1
	};
    $self->{pan}
      = {value      => 0,
	 minimum    => 0.25,
	 maximum    => 0.75,
	 mutability => 0.05
	};
}

##

sub sequence {
    my $self = shift;
    my @result;
    $self->mutate(['maxhits', 'offset', 'snapsize']);
    my $last = 0 - $self->offset->{value};

    my $loopsize = $self->loopsize;
    $#result = $loopsize - 1;

    foreach my $hitno (0 .. ($self->maxhits - 1)) {
	$self->mutate(['gap', 'vol', 'cutoff', 'res', 'pan']);
	my $place = int(abs($last + $self->gap->{value}));
	if ($place >= $loopsize) {
	    last;
	}
	$place = $place - ($place % int($self->snapsize->{value}));
	$result[$place] ||= {vol    => $self->{vol}->{value},
			     cutoff => $self->{cutoff}->{value},
			     res    => $self->{res}->{value},
			     pan    => $self->{pan}->{value},
			     pitch  => $self->{pitch}
			    };
	
	$last = $place;
    }
    return \@result;
}

##

sub next_beat {
    my $self = shift;
    if (not $self->{sequence} or not @{$self->{sequence}}) {
	$self->{sequence} = $self->sequence;
    }
    return shift @{$self->{sequence}};
}

sub mutate {
    my $self = shift;
    foreach my $p (@{$_[0]}) {
	my $h = $self->$p();
	my $value = ($h->{value} 
		     + rand($h->{mutability} * 2)
		     - $h->{mutability}
		    );
	if ($value < $h->{minimum}) {
	    $value = $h->{minimum} + ($h->{minimum} - $value);
	}
	elsif ($value > $h->{maximum}) {
	    $value = $h->{maximum} - ($value - $h->{maximum});
	}
	if ($value < $h->{minimum}) {
	    $value = $h->{minimum};
	}
	elsif ($value > $h->{maximum}) {
	    $value = $h->{maximum};
	}
	$h->{value} = $value;
    }
}

##

1;

