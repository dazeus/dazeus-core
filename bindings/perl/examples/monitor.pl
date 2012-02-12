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
$dazeus->subscribe(qw/WELCOMED CONNECTED DISCONNECTED JOINED PARTED MOTD
	QUIT NICK MODE TOPIC INVITE KICK PRIVMSG NOTICE CTCPREQ CTCPREPL
	ACTION NUMERIC UNKNOWN WHOIS NAMES COMMAND/);

while(my $event = $dazeus->handleEvent()) {
	print Dumper($event);
	print "\n";
}
