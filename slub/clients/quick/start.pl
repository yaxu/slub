#!/usr/bin/perl

use Client;
use strict;
use Data::Dumper;


my @ctrl_names = qw{};
my %ctrls = map {$_ => 0} @ctrl_names;
my $client;
my @samples;

my $sampleset = (shift @ARGV or 'stomp');
my $sample_no = (shift @ARGV or undef);


init();
start();

sub on_ctrl {
  my ($control, @data) = @_;
}

##

sub init_samples {
  my $sample_dir = "/slub/samples/$sampleset";
  
  opendir(DIR, $sample_dir) || die "can't opendir $sample_dir: $!";
  my @files = grep { -f "$sample_dir/$_" } readdir(DIR);
  closedir DIR;
  
  my $inc = 0;
  
  $client->ctrl_send("MSG", "# will you work");
  
  foreach my $file (@files) {
    next if not $file =~ /wav$/i;
    my $as = "${sampleset}$inc";
    $client->send("MSG", "load $sample_dir/$file as $as");
    push @samples, $as;
    $inc++;
  }
}

sub init {
  $client = Client->new(port      => 6010,
			on_clock  => \&on_clock,
			timer     => 'ext',
			tpb       => 16
		       );
  init_samples();
}
  
my @output;

sub start {
  my @data = 
    map {{sample => $samples[defined($sample_no)?$sample_no:rand(@samples)], note => (rand(70)+20)}}
    (0 .. 200);

  my @dummy = sort {push @output, [$a,$b];$a->{note} <=> $b->{note}} @data;
  $client->event_loop;
}

sub on_clock {
  my ($a, $b) = @{shift(@output)};
  
  $client->send('MSG', "play $a->{sample} pitch $a->{note}");
  $client->send('MSG', "play $b->{sample} pitch $b->{note}");
  exit if not @output;
}
