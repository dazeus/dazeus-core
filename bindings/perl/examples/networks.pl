#!/usr/bin/perl
use strict;
use warnings;
use DaZeus;

my ($socket) = @ARGV;
if(!$socket) {
	die "Usage: $0 socket\n";
}

my $dazeus = DaZeus->connect($socket);

$dazeus->subscribe('NAMES');

my $networks = $dazeus->networks();
foreach my $net (@$networks) {
	print "Network: $net\n";
	my $channels = $dazeus->channels($net);
	foreach my $chan (@$channels) {
		print "  Channel: $chan\n";
		# Ask for NAMES:
		$dazeus->sendNames($net, $chan);
		my $had_names = 0;
		until($had_names) {
			my $event = $dazeus->handleEvent();
			if(printNames($event, $chan)) {
				$had_names = 1;
			}
		}
	}
}

sub printNames {
	my ($event, $chan) = @_;
	warn "Not NAMES!" if($event->{event} ne "NAMES");

	my $names = $event->{params};
	shift @$names; # network
	shift @$names; # origin

	# We might receive a NAMES for a different channel, if something else
	# asked for it.
	my $channel = shift @$names;
	return if($channel ne $chan);

	print "    " . scalar(@$names) . " nicks on this channel:\n";
	until(@$names == 0) {
		my @these = splice @$names, 0, 10;
		print "    " . join(", ", @these) . "\n";
	}
	return 1;
}
