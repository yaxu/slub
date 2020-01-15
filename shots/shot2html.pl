#!/usr/bin/perl -w

use strict;
undef $/;

my $pid = shift @ARGV;

die "no pid" unless $pid;

$pid =~ s/\-//;

print <<HTML;
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
 "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
  <head>
    <title>shots of process number $pid</title>
    <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1" />
  </head>
  <body>
  <h1>shots of process number $pid</h1>
HTML

foreach my $file (</yaxu/shots/$pid-*>) {
    open(FH, "<$file");
    my $code = escapeHTML(<FH>);
    close(FH);
    print <<"    HTML";
     <div style="float: left; width: 30%; font-size: 6px; border: solid black 1px;margin: 1px; padding: 3px;"><pre>$code</pre></div>
    HTML
}
print <<HTML;
HTML

sub escapeHTML {
    my ($toencode) = shift;
    return undef unless defined($toencode);
    $toencode =~ s{&}{&amp;}gso;
    $toencode =~ s{<}{&lt;}gso;
    $toencode =~ s{>}{&gt;}gso;
    $toencode =~ s{"}{&quot;}gso;
    return $toencode;
}
