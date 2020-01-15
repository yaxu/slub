package Circ::Feep::Base;

use strict;

use Circ::Env;

use Class::MethodMaker
  get_set => [qw{ env trigger dispersal racism friendly cell_id strength
		  type
		}
	     ]
  ;
my @rqrd = qw/env trigger/;

##

sub new {
    my($pkg, $p) = @_;
    
    my $default = $pkg->default;

    my $self = {%$default, %$p};
    $self->{dropped} = 0;
    foreach my $rqr (@rqrd) {
	if(not exists $self->{$rqr}) {
	    die("rqrd: $rqr\n");
	}
    }


    bless $self, $pkg;

    $self->id();
    
    return $self;
}

##

{
    my $inc;
    sub id {
	my $self = shift;
	$inc ||= 0;
	$self->{id} ||= $inc++;
    }
}

##

sub default {
    {dispersal => 2,
       racism    => 2,
	 friendly  => 2,
	   strength  => 16
	 };
}

##

sub tolerate {
    my ($self, $other, $try_cell_id) = @_;
    
    $try_cell_id ||= $self->cell_id;

    if (not defined $try_cell_id) {
	die "i don't have a cell_id to compare with the other feep's";
    }
    
    my $cell_diff = $self->env->cell_diff($self->cell_id, $other->cell_id);
    
    my $result   = 0;
    my $tolerate = 1;
    
    if ($self->type eq $other->type) {
	if ($self->dispersal > $cell_diff
	    or $other->dispersal > $cell_diff
	   ) {
	    $tolerate = 0;
	}
    }
    else {
	if ($self->racism > $cell_diff
	    or $other->racism > $cell_diff
	   ) {
	    $tolerate = 0;
	}
    }
    
    if (not $tolerate) {
	if ($self->strength > $other->strength
	    or (($self->strength == $other->strength)
		and ($self->id < $other->id)
	       )
	   ) {
	    $result = 1;
	}
	else {
	    $result = -1;
	}
    }
    return $result;
}

##

sub drop {
    my ($self, $cell_id) = @_;
    
    my @displace;

    $self->pickup if $self->dropped;

    $self->dropped(1);

    $self->cell_id($cell_id);

  FEEPCYCLE:
    foreach my $feep (@{$self->env->feeps}) {
	next if not $feep->dropped;
	next if $feep eq $self;

	my $tolerate = $self->tolerate($feep);
	if ($tolerate > 0) {
	    push @displace, $feep;
	}
	elsif ($tolerate < 0) {
	    @displace = ($self);
	    last FEEPCYCLE;
	}
    }
    
    foreach my $feep (@displace) {
	$feep->pickup();
    }

    if ($self->dropped) {
	$self->env->add($self, $cell_id);
    }

    return @displace;
}

##

sub pickup {
    my $self = shift;
    $self->env->remove_feep($self);
    $self->cell_id(undef);
    $self->dropped(0);
}

##

sub dropped {
    my ($self, $value) = @_;
    if (defined $value) {
	$self->{dropped} = $value;
    }
    else {
	$self->{dropped} ||= 0
    }
    return $self->{dropped};
}

##

1;
