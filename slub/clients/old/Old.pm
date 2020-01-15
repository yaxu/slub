package old::Old;

use strict;

##

use POSIX qw/ fabs /;

##

use Class::MethodMaker 
  new_with_init => 'new',
  get_set => [qw{maxhits gap vol cutoff res pan snapsz offset 
		 loopsize sample pitch
		}
	     ];

use Data::Dumper;


##

sub init {
    my $self = shift;
    my %p = @_;
    
    # seed values
    $self->{sample}   = ($p{sample} or die "no sample");
    $self->{loopsize} = ($p{loopsize} or 64);
    $self->{pitch}    = ($p{pitch} or 60);
    
    $self->{maxhits} 
      = {value      => 16,
	 minimum    => 1,
	 maximum    => 24,
	 format     => "%8d",
	 mutability => 0.1
	};
    $self->{gap} 
      = {value      => 8,
	 minimum    => 1,
	 maximum    => 12,
	 format     => "%8d",
	 mutability => 0.5
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
	 maximum    => 4,
	 format     => "%8d",
	 mutability => 0.1
	};

    $self->{offset} 
      = {value      => 8,
	 minimum    => 1,
	 maximum    => 16,
	 format     => "%8d",
	 mutability => 0.1
	};
    $self->{cutoff} 
      = {value      => 100,
	 minimum    => 0.1,
	 maximum    => 44100,
	 format     => "%8.2f",
	 mutability => 100
	};
    $self->{res} 
      = {value      => 0,
	 minimum    => 0,
	 maximum    => 0.9,
	 format     => "%.6f",
	 mutability => 0.1
	};
    $self->{pan}
      = {value      => 0,
	 minimum    => 0.25,
	 maximum    => 0.75,
	 format     => "%.6f",
	 mutability => 0.05
	};
}

##

sub twiddle {
    my ($self, $pea, $up) = @_;
    
    my $h = $self->$pea();
    
    $h->{value} += ($up
		    ? $h->{mutability}
		    : (0 - $h->{mutability})
		   );
    round($h);
}

##

{
    my @sequence_history;

    sub sequence_history {
	\@sequence_history;
    }

    sub save {
	my($self, $key) = @_;
	
	$self->{saves}->{$key} = $sequence_history[0];
    }
    
    sub restore {
	my ($self, $key) = @_;
	if (exists $self->{saves}->{$key}) {
	    warn("restoring " . Dumper($self->{saves}->{$key}) . "\n");
	    push @{$self->{sequence}}, @{$self->{saves}->{$key}};
	}
    }

    sub mutate_sequence {
	['maxhits', 'offset', 'snapsz'];
    }

    sub mutate_note {
	['gap', 'vol', 'cutoff', 'res', 'pan'];
    }

    sub sequence {
	my $self = shift;
	my @result;
	
	$self->mutate($self->mutate_sequence);
	$self->premutate($self->mutate_note);

	my $last = undef;
	
	my $loopsize = $self->loopsize;
	$#result = $loopsize - 1;
	
	foreach my $hitno (0 .. ($self->maxhits->{picked} - 1)) {
	    $self->mutate($self->mutate_note);
	    my $place;

	    if (defined $last) {
		$place = int(abs($last + $self->gap->{picked}));
	    }
	    else {
		$place = $self->offset->{picked};
	    }

	    if ($place >= $loopsize) {
		last;
	    }
	    $place = $place - ($place % int($self->snapsz->{picked}));
	    $result[$place] ||= {vol    => $self->{vol}->{picked},
				 cutoff => $self->{cutoff}->{picked},
				 res    => $self->{res}->{picked},
				 pan    => $self->{pan}->{picked},
				 pitch  => $self->{pitch}
				};
	    if (ref $self eq 'old::Old::Bang') {
		main::addstr(20 + $hitno, 10, "hit $hitno placed $place $self->{offset}->{picked}\n");
	    }
	    
	    $last = $place;
	}
	my @copy = @result;
	unshift @sequence_history, \@copy;
	if (@sequence_history > 2) {
	    pop @sequence_history;
	}
	$self->demutate($self->mutate_note);
	
	return \@result;
    }
}

##

{
    my $save;
    sub premutate {
	my ($self, $p) = @_;
	$save = {map {$_ => $self->{$_}->{value}} @$p};
    }
    
    ##
    
    sub demutate {
	my ($self) = @_;
	if ($save) {
	    while (my ($foo, $ov) = each %$save) {
		if (exists $self->{$foo}->{deviate}) {
		    my $d = $self->{$foo}->{deviate};
		    my $nv = $self->{$foo}->{value};
		    my $change = $nv - $ov;
		    
		    if (fabs($change) > $d) {
			$self->{$foo}->{value} = (($change > 0)
						  ? ($ov + $d)
						  : ($ov - $d)
						 );
		    }
		}
	    }
	}
	undef $save;
    }
}

##

sub next_beat {
    my $self = shift;
    if (not $self->{sequence} or not @{$self->{sequence}}) {
	$self->{sequence} = $self->sequence;
    }
    return shift @{$self->{sequence}};
}

##

sub mutate {
    my $self = shift;
    foreach my $p (@{$_[0]}) {
	my $h = $self->$p();
	
	$h->{value} = (($h->{value}||0) 
		       + rand($h->{mutability} * 2)
		       - $h->{mutability}
		      );
	round($h);
	if (exists $h->{set}) {
	    $h->{picked} = $h->{set}->[$h->{value}];
	}
	else {
	    $h->{picked} = $h->{value};
	}
    }
}

##

sub round {
    my $h = shift;

    my $minimum;
    my $maximum;

    if (exists $h->{set}) {
	$minimum = 0;
	$maximum = @{$h->{set}};
    }
    else {
	$minimum = $h->{minimum};
	$maximum = $h->{maximum};
    }

    my $value = $h->{value};

    if ($value < $minimum) {
	$value = $minimum + ($minimum - $value);
    }
    elsif ($value > $maximum) {
	$value = $maximum - ($value - $maximum);
    }
    if ($value < $minimum) {
	$value = $minimum;
    }
    elsif ($value > $maximum) {
	$value = $maximum;
    }

    $h->{value} = $value;
}

##

1;

