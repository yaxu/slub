use strict;

##

sub perms {
    my $p = (ref($_[0]) ? shift(@_) : {@_});
    my $things = $p->{things};
    my $space  = $p->{space};

    my $perms = gen($things, $space);
    
    @$perms = map {pop @$_;$_}
      sort {$a->[$things] <=> $b->[$things]}
        @$perms;
    
    return $perms;
}

##

sub gen {
    my($numbers, $total, $gen, $array, $result) = @_;
    $array  ||= [];
    $gen    ||= 0;
    $result ||= [];

    my $subtotal = 0;
    if ($gen) {
        foreach my $c (0 .. $gen - 1) {
            $subtotal += $array->[$c];
        }
    }

    my $top = ($total - $subtotal - ($numbers - $gen - 1));
    if ($gen == ($numbers - 1)) {
        $array->[$gen] = $top;
        my $deviance;
        foreach my $c (0 .. $numbers - 1) {
            $deviance +=
              abs(int($total / $numbers) - $array->[$c]);
        }
        $array->[$numbers] = $deviance;

        # push a copy of the array on to @$result
        push @$result, [@$array];
    }
    else {
        foreach my $c (1 .. $top) {
            $array->[$gen] = $c;
            gen($numbers, $total, $gen + 1, $array, $result);
        }
    }
    return $result;
}

##

sub shrink {
    my $string = shift;
    if (length($string) % 2) {
        $string .= '.';
    }
    my $length = length($string);
    my $result = $string;
    $result =~ s/(.)./$1/g;

    my $pad = ('.' x (($length - length($result)) / 2));
    $result = "$pad$result$pad";
    return($result);
}

##

sub line {
    my $self = shift;
    my $line = shift;
    my $result;
    if (@_) {
       $self->code->[$line] = "# $_[0]";
    }
    ($result) = ($self->code->[$line] =~ /\#\s+(.+)/);
    return($result);
}

##

1;

