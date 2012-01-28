#!/usr/bin/perl
use strict;
use warnings;
use DaZeus;

my ($socket, $namespace) = @ARGV;
if(!$namespace) {
	die "Usage: $0 socket namespace\n";
}

my $dazeus = DaZeus->connect($socket);

my $keys = $dazeus->getPropertyKeys($namespace);
foreach(@$keys) {
	print "$_: ";
	my $value = $dazeus->getProperty($namespace . "." . $_);
	print "$value\n";
}

print scalar(@$keys) . " keys listed.\n";
