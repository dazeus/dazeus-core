#!/usr/bin/perl
use strict;
use warnings;

use DBI;
use MongoDB;
use Data::Dumper;

my ($mongohost, $dsn, $user, $pass) = @ARGV;
if(!$mongohost || !$dsn) {
	warn "Usage: $0 mongohost[:port] old-db-dsn [old-db-user [old-db-pass]]\n";
	warn "The DSN is used by the Perl DBI module, and might look like one of these:\n";
	warn "  DBI:mysql:database=dazeus;host=localhost\n";
	warn "  DBI:SQLite:dbname=database.sqlite\n";
	die "Did not understand parameters\n";
}

print "Attempting connection to new database...\n";
my $mongo = MongoDB::Connection->new(host => $mongohost) or die $!;
my $mongodb = $mongo->dazeus or die $!;
my $properties = $mongodb->properties or die $!;

print "Attempting connection to old database...\n";
my $olddbh = DBI->connect($dsn, $user, $pass, {RaiseError => 1});
my $sth = $olddbh->prepare("SELECT variable, network, receiver, sender, value FROM properties");
$sth->execute or die $!;
my $inserted = 0;

print "Starting to convert " . $sth->rows . " properties...\n";
while(my $ref = $sth->fetchrow_hashref()) {
	$inserted++;
	$properties->insert($ref);
}

print "Done. Converted $inserted properties.\n";

