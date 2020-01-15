#!/usr/bin/perl -w

use strict;

use lib '/yaxu/lc/lib';
use Editor;

my $text = '';
my $file = '';
if (@ARGV) {
    $file = shift @ARGV;
    if (open(FH, "<$file")) {
      $text = join('', <FH>);
      close FH;
    }
}

my $editor = Editor->new(text => $text, file => $file);
$editor->run;

