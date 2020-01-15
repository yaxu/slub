sub bang {
    my $self = shift;
    my $foo = "suiobu";
    $foo =~ s/u/a/g if $self->{bangs} % 32 > 20;
    my $bar = "bobu";
    $bar =~ s/b/s/g if $self->{bangs} % 32 < 20;
    $self->say($foo) unless $self->{bangs} % 4;
    $self->say($bar) if $self->{bangs} % 8 == 4;
}