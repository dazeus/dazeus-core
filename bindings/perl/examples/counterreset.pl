#!/usr/bin/perl
use strict;
use warnings;
use DaZeus;

my ($socket) = @ARGV;
if(!$socket) {
	die "Usage: $0 socket\n";
}

my $dazeus = DaZeus->connect($socket);

my $count = $dazeus->getProperty("examples.counter.count");
print "Current count: " . (defined $count ? $count : "unset") . "\n";
$dazeus->unsetProperty("examples.counter.count");
print "Count reset!\n";
