#!/usr/bin/perl
use strict;
use warnings;
use DaZeus;
use Data::Dumper;

my ($socket) = @ARGV;
if(!$socket) {
	die "Usage: $0 socket\n";
}

my $dazeus = DaZeus->connect($socket);

my $array = $dazeus->getProperty("examples.structure.array");
$array ||= [0];
print "Current array: " . Dumper($array);
push @$array, $array->[$#{$array}] + 1;
$dazeus->setProperty("examples.structure.array", $array);
print "Count changed to: " . Dumper($array);
