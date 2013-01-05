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
$dazeus->subscribe(qw/CONNECT DISCONNECT JOIN PART QUIT NICK MODE
	TOPIC INVITE KICK PRIVMSG NOTICE CTCP CTCP_REP ACTION NUMERIC
	UNKNOWN WHOIS NAMES PRIVMSG_ME CTCP_ME ACTION_ME/);

while(my $event = $dazeus->handleEvent()) {
	print Dumper($event);
	print "\n";
}
