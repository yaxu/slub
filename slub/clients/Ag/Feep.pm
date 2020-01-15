package Ag::Feep;

use strict;

use Ag::Env;

use Class::MethodMaker
  new_hash_init => 'new',
  get_set => [qw/ env sample /]
  ;

##
# try to put this feep on a cell

sub droppable {
    my($self) = @_;
    
    return($self->test_cell and $self->test_context);
}

##

sub test_cell {
    my($self) = @_;

    my $feeps = $self->env->cell();
    my $result = 1;
    my $pkg = ref $self;

  CHECK: {
	if ($self->dispersal > 0) {
	    if (grep{$pkg eq ref($_)} @$feeps) {
		#warn("  disperse\n");
		$result = 0;
		last CHECK;
	    }
	}
	if ($self->racism > 0) {
	    if (grep{$pkg ne ref($_)} @$feeps) {
		#warn("  i'm too racist\n");
		$result = 0;
		last CHECK;
	    }
	}
	if (@$feeps > $self->friendly) {
	    warn("  too crowded\n");
	    $result = 0;
	    last CHECK;
	}
    };
    return $result;
}

##

sub test_context {
    my($self) = @_;

    my $result = 1;
    my $pkg = ref $self;

  CHECK: {
	if ($self->dispersal > 1) {
	    for my $distance (1 .. ($self->dispersal - 1)) {
		my @feeps = (@{$self->env->last($distance)},
			     @{$self->env->next($distance)}
			     );
		if (grep{$pkg eq ref($_)} @feeps) {
		    #warn("  disperse ($distance)\n");
		    $result = 0;
		    last CHECK;
		}
	    }
	}
	if ($self->racism > 1) {
	    for my $distance (1 .. ($self->racism - 1)) {
		my @feeps = (@{$self->env->last($distance)},
			     @{$self->env->next($distance)}
			     );
		if (grep{$pkg ne ref($_)} @feeps) {
		    warn("  i'm too racist ($distance)\n");
		    $result = 0;
		    last CHECK;
		}
	    }
	}
    };
    return $result;
}

##
# decides who gets first choice over a cell

sub strength {
    256;
}

##
# min number of cells between each feep of this subclass.

sub dispersal {
    3;
}

##
# tolerable distance from other feeps not of this subclass

sub racism {
    0;
}

##
# tolerable number of feeps that this feep will share a cell with

sub friendly {
    3;
}

##

1;
