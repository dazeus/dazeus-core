# DaZeus - A highly functional IRC bot
# Copyright (C) 2007  Sjors Gielen <sjorsgielen@gmail.com>
#
## Refter-module
# Copyright (C) 2011  Aaron van Geffen <aaron@aaronweb.net>
# Original module (C) 2010  Gerdriaan Mulder <har@mrngm.com>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

package DaZeus2Module::PiepNoms;

use strict;
use warnings;

use base qw(DaZeus2Module);
use Time::localtime;
use XML::DOM::XPath;
use LWP::Simple;

my %dayToIndex = ("zo" => 0, "ma" => 1, "di" => 2, "wo" => 3, "do" => 4, "vr" => 5, "za" => 6);
my @daysOfWeek = qw(zo ma di wo do vr za zo);

sub getDayKey {
	my ($self, $day) = @_;

	# The #ru regulars like their puns, so let's shorten this.
	$day = lc(substr($day, 0, 2)) if($day);

	# A specific request!
	if ($day and exists $dayToIndex{$day}) {
		return $dayToIndex{$day};
	}
	# Tomorrow ("morgen")
	elsif ($day eq "mo") {
		return (localtime->wday() + 1) % 7;
	}
	# The day after tomorrow ("overmorgen")
	elsif ($day eq "ov") {
		return (localtime->wday() + 2) % 7;
	}
	# Just today.
	else {
		return localtime->wday();
	}
}

# NOTE: this sub assumes $day to be an integer in [0..6]
sub pickMenuUrl {
	my ($self, $day) = @_;

	my $next_week = localtime->wday() > $day;
	# except when it's sunday, then always take next week
	$next_week = 1 if(localtime->wday() == 0);

	return "http://www.ru.nl/facilitairbedrijf/eten_en_drinken/weekmenu_de_refter/menu-" . ($next_week ? "komende-" : "") . "week/?rss=true";
}

sub fetchMenuByDay {
	my ($self, $day) = @_;
	my $tree = XML::DOM::Parser->new();

	$day = $self->getDayKey($day);

	my $doc = $tree->parse(get(pickMenuUrl($self, $day)));
	my ($menu_day, $menu);

	foreach ($doc->findnodes('//item')) {
		# The title is used to determine whether we have the right day.
		my $title = $_->getElementsByTagName('title')->item(0)->getFirstChild()->getNodeValue();

		# If this item is not relevant to the query, skip it.
		if ($day != $dayToIndex{lc(substr($title, 0, 2))}) {
			next;
		}

		# What day is it, again?
		$menu_day = lc(($title =~ /([A-z]+dag \d+ [a-z]+):?/)[0]);

		# Fetch the menu for the day.
		$menu = $_->getElementsByTagName('description')->item(0)->getFirstChild()->getNodeValue();

		# Perchance there's additional price info -- we already know!
		$menu =~ s/Prijs\s*:[^\n]*\n//s;

		# Trim any leading and trailing whitespace.
		$menu =~ s/^\s+(.+?)\s+$/$1/s;

		# Strip superfluous information
		$menu =~ s/\(\s*['`]s[ -]avonds\s*\)\s?//;

		last;
	}

	if (!defined $menu_day) {
		$menu_day = $daysOfWeek[$day];
	}

	return ($menu_day, $menu);
}

sub told {
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	my $who = $mess->{who};

	return if !defined $command or $command ne "noms";

	my ($day, $noms);

	# Any specific day?
	if ($rest[0]) {
		($day, $noms) = $self->fetchMenuByDay($rest[0]);
	}
	# Tomorrow?
	elsif (localtime->hour() >= 19) {
		$self->bot->reply($mess, "Na 19 uur zijn er geen refternoms meer. Daarom krijg je de refternoms van morgen te zien ;-)");
		($day, $noms) = $self->fetchMenuByDay("morgen");
	}
	# Just today, please.
	else {
		$self->bot->reply($mess, "Wat heeft de Refter vandaag in de aanbieding...");
		($day, $noms) = $self->fetchMenuByDay();
	}

	# There's something to eat, right?
	if (defined $noms) {
		$self->bot->reply($mess, "Eten bij de Refter op " . $day . ":");
		$self->bot->reply($mess, $noms);
	}
	# No noms whatsoever?! THE AGONY!
	else {
		$self->bot->reply($mess, "Helaas, er is niks :'-(");
	}
}

sub parseMsg {
	my ($self, $mess) = @_;
	my $body = $mess->{body};

	# { argh, stupid vim editing
	if($body =~ /^}/ or $mess->{channel} eq 'msg' or $mess->{raw_body} =~ /^DazNET(?::|,|;)/i) {
			# {{{ argh, stupid vim editing
			my ($command, $rest);
			if($body =~ /^}\s*$/i) {
					return undef;
			} elsif($body =~ /^}\s*(\w+)$/) {
					$command = $1;
			} elsif($body =~ /^}\s*(\w+)\s+(.*)$/) {
					$command = $1; $rest = $2;
			} elsif($mess->{channel} eq 'msg') {
					if($body =~ /^\s*(\w+)\s+(.*)\s*$/) {
							$command = $1; $rest = $2;
					} elsif($body =~ /^\s*(\w+)\s*$/) {
							$command = $1;
					}
			} elsif($mess->{raw_body} =~ /^DazNET(?::|,|;)\s+(\w+)$/i) {
					$command = $1;
			} elsif($mess->{raw_body} =~ /^DazNET(?::|,|;)\s+(\w+)\s+(\w+)$/i) {
					$command = $1; $rest = $2;
			}
			my @rest;
			@rest = split /\s/, $rest if(defined($rest));
			$command = lc($command) if(defined($command));
			return ($command, $rest, @rest) if wantarray;
			return [$command, $rest, @rest];
	} else {
			return undef;
	}
}


1;
