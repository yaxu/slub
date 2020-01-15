package Ag::Env;

use strict;

use Class::MethodMaker
  get_set       => [ qw / width height mtx x y cell feeps / ];

use Data::Dumper;

##

sub new {
    my($pkg, $self) = @_;
    bless $self, $pkg;
    
    $self->{mtx}   = [map{[map{[]} (1 .. $self->height)]} (1 .. $self->width)];
    $self->{cell}  = $self->{mtx}->[0]->[0];
    $self->{x}     = $self->{y} = 0;
    $self->{feeps} = [];
    return $self;
}

##

sub inc_cell {
    my $self = shift;
    my $x = $self->x;
    my $y = $self->y;
    if (++$x >= $self->width) {
	$x = 0;

	if (++$y >= $self->height) {
	    $y = 0;
	}

	$self->y($y);
    
    }

    $self->x($x);
    $self->cell($self->mtx->[$x]->[$y]);
}

##

sub last {
    my($self, $i) = @_;
    $i ||= 1;

    my $width = $self->width;
    my $height = $self->height;
    
    my $x = $self->x;
    my $y = $self->y;

    $x -= $i;
    while ($x < 0) {
	$x += $width;
	--$y;
    }
    while ($y < 0) {
	$y += $height;
    }

    return $self->mtx->[$x]->[$y];
}
##

sub next {
    my($self, $i) = @_;
    $i ||= 1;

    my $width = $self->width;
    my $height = $self->height;
    
    my $x = $self->x;
    my $y = $self->y;

    $x += $i;
    while ($x >= $width) {
	$x -= $width;
	++$y;
    }
    while ($y >= $height) {
	$y -= $height;
    }

    return $self->mtx->[$x]->[$y];
}

##

sub introduce {
    my ($self, $feep) = @_;

    my $x = $self->x;
    my $y = $self->y;

    my $result = 0;

    push(@{$self->feeps}, $feep);
    $feep->env($self);
}

##

sub drop_feeps {
    my $self = shift;
    my @feeps = (map { $_->[0] }
		 sort { $b->[1] cmp $a->[1] }
		 map  { [ $_, $_->strength ] } 
		 @{$self->feeps}
		);

  FEEPDROP:
    foreach my $feep (@feeps) {
	$self->x(0);
	$self->y(0);
	if ($feep->droppable()) {
	    push @{$self->cell}, $feep;
	    $feep->env($self);
	    next FEEPDROP;
	}
	else {
	    for my $offset (0 .. (($self->width * $self->height) - 1)) {
		$self->inc_cell;
		if ($feep->droppable()) {
		    push @{$self->cell}, $feep;
		    next FEEPDROP;
		}
	    }
	}
	print "picking feep back up again\n";
    }
}

##

1;
