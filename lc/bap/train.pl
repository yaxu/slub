# 12341234
# . o oo o
# ==============-=
# 16
# 1

sub bang {
    my $self = shift;
    return unless $self->line(0);

    my $size = $self->line(3);

    my $train = $self->line(1);
    my $line = $self->line(2);

    if ($self->line(4)) {
      $line = ('=' x $size);
      substr($line, $size - 1 - ($self->{bangs} % $size), 1, '-');
    }
    elsif (length($line) < $size) {
      $line .= ('=' x ($size - length($line)));
    }
    elsif (length($line) > $size) {
      $line = substr($line, 0, $size);
    }

    foreach my $point (0 .. (length($train) - 1)) {
        if (substr($train, $point, 1) eq 'o'
            and substr($line, $point, 1) eq '-'
           ) {
          my $pan = (1 - ($point / length($train)) * 2);

          $self->t({sample => "can/$self->{bangs}", pan => $pan,
                    speed => 1.5, shape => rand(1), envelope_name => 'valley',
                    accellerate => -0.000005, loops => rand(5),
#                    vowel => 'e',
                    volume => 0.1
                   }
                  );
        }
    }

    $line =~ s/(.)(.+)/$2$1/g;
    $self->line(2, $line);

    $self->modified;
}