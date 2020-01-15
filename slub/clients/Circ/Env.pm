package Circ::Env;

use strict;

##

use Class::MethodMaker
  get_set => [qw/ size1d sizex sizey env maxsweeps feeps /];

##

sub new {
    my ($pkg, $p) = @_;
    
    my($size1d, $sizex, $sizey);

    if (exists $p->{x} and exists $p->{y}) {
	$sizex = $p->{x};
	$sizey = $p->{y};
	$size1d = $sizex * $sizey;
    }
    elsif(exists $p->{size1d}) {
	$sizex = $size1d = $p->{size1d};
	$sizey = 1;
    }
    else {
	die("no size1d (or x and y)\n");
    }
    
    my $self = {size1d    => $size1d,
		sizex     => $sizex,
		sizey     => $sizey,
		maxsweeps => ($p->{maxsweeps} or 1),
		env       => [],
		feeps     => []
	       };

    bless $self, $pkg;
}

##

sub twod2cell_id {
    my ($self, $x, $y) = @_;
    
    my $cell_id;
    
    if (not defined $y) {
	$y = 0;
    }
    
    $cell_id = $x + ($self->sizex * $y);
    $cell_id = $cell_id % $self->size1d;
    
    return $cell_id;
}

##

sub cell {
    my $self = shift;
    my $cell_id;

    if (@_ > 1) {
	$cell_id = $self->twod2cell_id(@_);
    }
    elsif (@_ == 1) {
	$cell_id = (shift(@_) % $self->size1d);
    }
    else {
	$cell_id = 0;
    }

    return($self->env->[$cell_id]);
}

##

sub cell_diff {
    my ($self, $cell_id0, $cell_id1) = @_;

    if ($cell_id0 < $cell_id1) {
	($cell_id0, $cell_id1) = ($cell_id1, $cell_id0);
    }
    my $size1d = $self->size1d;
    
    my $result1 = ($cell_id0 - $cell_id1) % $size1d;
    my $result2 = ($size1d - $result1);
    return($result1 < $result2 ? $result1 : $result2);
}

##

sub introduce {
    my($self, @addfeeps) = @_;
    
    my @todo = @addfeeps;
    my @displaced;
    
    push @{$self->feeps}, @addfeeps;
    
  SWEEP:
    for (0 .. $self->maxsweeps - 1) {
	for my $cell_id (0 .. ($self->size1d - 1)) {
	    foreach my $feep (@todo) {
		push @displaced, $feep->drop($cell_id);
	    }
	    last SWEEP if not @displaced;
	    
	    @todo = map  { $_->[0] }
	      sort { $b->[1] <=> $a->[1] }
		map  { [ $_, $_->strength ] } @displaced;
	    
	    @displaced = ();
	}
    }
}

##

sub add {
    my ($self, $feep, $cell_id) = @_;
    push(@{$self->env->[$cell_id % $self->size1d]}, $feep);
}

##

sub remove_feep {
    my($self, $feep) = @_;

    my $cell_id = $feep->cell_id;
    if (not defined $cell_id) {
	die "feep didn't have a cell_id";
    }

    my $size1d = $self->size1d;
    
    @{$self->env->[$cell_id % $size1d]} =
      grep { $_ ne $feep } @{$self->env->[$cell_id % $size1d]};

    return $feep;
}

##

sub dump_env {
    my $self = shift;
    my $env = $self->env;
    foreach my $cell_id (0 .. $self->size1d - 1) {
	print "[$cell_id]\n";
	foreach my $feep (@{$env->[$cell_id]}) {
	    print("  (" . ref($feep) . ") #" . $feep->id . "\n");
	}
    }
}

##

1;
