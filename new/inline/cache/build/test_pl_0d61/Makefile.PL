use ExtUtils::MakeMaker;
my %options = %{       
{
  'TYPEMAPS' => [
    '/usr/lib/perl5/5.8.5/ExtUtils/typemap'
  ],
  'NAME' => 'test_pl_0d61',
  'INC' => '-I/music/new/inline/',
  'LIBS' => [
    '-llo'
  ],
  'VERSION' => '0.00'
}
};
WriteMakefile(%options);

# Remove the Makefile dependency. Causes problems on a few systems.
sub MY::makefile { '' }
