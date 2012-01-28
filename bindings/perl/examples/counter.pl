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
$count ||= 0;
print "Current count: $count\n";
$dazeus->setProperty("examples.counter.count", ++$count);
print "Count changed to: $count\n";
