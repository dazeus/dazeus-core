#!/usr/bin/perl
use strict;
use warnings;
use DaZeus;
use Data::Dumper;

my ($socket, $namespace, $network, $receiver, $sender) = @ARGV;
if(!$namespace) {
	die "Usage: $0 socket namespace [network [receiver [sender]]]\n";
}

my $dazeus = DaZeus->connect($socket);

my $keys = $dazeus->getPropertyKeys($namespace, $network, $receiver, $sender);
foreach(@$keys) {
	print "= $_ =\n";
	my $value = $dazeus->getProperty($namespace . "." . $_);
	print Dumper($value);
	print "\n";
}

print scalar(@$keys) . " keys listed.\n";
