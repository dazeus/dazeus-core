#!/usr/bin/perl
use strict;
use warnings;
use DaZeus;

my ($socket) = @ARGV;
if(!$socket) {
	die "Usage: $0 socket\n";
}

my $dazeus = DaZeus->connect($socket);

# From the documentation:
$dazeus->setProperty("examples.scope.foo", "bar");
print "Set global scope to bar\n";
$dazeus->setProperty("examples.scope.foo", "baz", "oftc");
print "Set network scope for 'oftc' to baz\n";

my $value = $dazeus->getProperty("examples.scope.foo", "q", "#moo");
print "Value for network scope 'q', channel #moo: $value\n"; # should be bar
$value = $dazeus->getProperty("examples.scope.foo", "oftc", "#moo");
print "Value for network scope 'oftc', channel #moo: $value\n"; # should be baz

$dazeus->unsetProperty("examples.scope.foo", "oftc");
print "Unset network scope for 'oftc'\n";
$value = $dazeus->getProperty("examples.scope.foo", "oftc", "#moo");
print "Value for network scope 'oftc', channel #moo: $value\n"; # should be bar
$dazeus->unsetProperty("examples.scope.foo");
print "Unset global scope\n";

$dazeus->setProperty("examples.scope.foo", "quux", "oftc");
print "Set network scope 'oftc' to quux\n";
$value = $dazeus->getProperty("examples.scope.foo", "q");
print "Network value for network 'q': " . (defined $value ? $value : "unset") . "\n";
# should be unset

# Clean up
$dazeus->unsetProperty("examples.scope.foo", "oftc");
