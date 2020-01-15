# ..x...x...x...x...x...x...x...x...x...x...
# .
# .x...x...x...x.x.....o...x..x.x..x...x...x
sub bang {
    my $self = shift;
    my $foo = $self->line(0);
    $foo =~ s/(.)//;
    my $char = $1;
    $foo .= $self->{bangs} % 4 ? '.' : 'x';

    if (rand() > 0.8 and $self->{bangs} % 4 == 0) {
        my @foo = split(//, $foo);
        my $point = int(scalar(@foo) / 2);
        my $strength = 4;
        my $max = scalar(@foo) - 1;
        my @bar = (('.') x scalar(@foo));
        $bar[$point] = 'o';
        foreach my $bar (0 .. $point) {
            if ($point + $bar < $max and $foo[$point + $bar] ne '.') {
                my $to = $point + $bar + $strength;
                $to = $max if $to > $max;
                $bar[$to] = $foo[$point + $bar];
                $foo[$point + $bar] = '.';
                $strength--;
                last unless $strength;
            }
            if ($point - $bar > 0 and $foo[$point - $bar] ne '.') {
                my $to = $point - $bar - $strength;
                $to = 0 if $to < 0;
                $bar[$to] = $foo[$point - $bar];
                $foo[$point - $bar] = '.';
                $strength--;
                last unless $strength;
            }
        }
        foreach my $bar (0 .. $max) {
            if ($bar[$bar] eq 'o' or $bar[$bar] eq 'x') {
                $foo[$bar] = $bar[$bar];
            }
        }
        my $bing = join('', @foo);
        $self->line(2, $bing);
        $foo = $bing;
    }

    $self->line(0, $foo);

    $self->line(1, $char);
    $self->modified;

    if ($char eq 'x') {
#        $self->t();
    }
    elsif ($char eq 'o') {
#        $self->t({sample => "drum/0"});
    }
}
