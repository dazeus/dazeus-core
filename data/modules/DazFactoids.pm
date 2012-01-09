# DaZeus - A highly functional IRC bot
# Copyright (C) 2007  Sjors Gielen <sjorsgielen@gmail.com>

# DazFactoids Module
# Copyright (C) 2011  Aaron van Geffen <aaron@aaronweb.net>
# Original module (C) 2007  Sjors Gielen <sjorsgielen@gmail.com>
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

package DaZeus2Module::DazFactoids;

use strict;
use warnings;

use base qw(DaZeus2Module);
use Data::Dumper;
use v5.10;

# Told function, a.k.a. "the controller".
sub told {
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my $who = $mess->{who};
	my $channel = $mess->{channel};
	my ($command, $rest, @rest) = $self->parseMsg($mess);

	# Are we just doing a factoid lookup?
	if ($mess->{raw_body} =~ /^\](.+)$/) {
		return $self->getFactoid($1, $mess);
	}

	# Basic sanity checking.
	return if !defined $command;

	# Teaching new factoids.
	if ($command eq "learn" || $command eq "reply" || $command eq "forward") {
		return "You don't have dazeus.commands.learn permissions." if (!$p->has("dazeus.commands.learn"));

		# Let's try to keep this as English as possible, okay?
		my ($factoid, $value, $separator);
		if ($command eq "reply") {
			($factoid, $value) = $rest =~ /^(.+?)\s+with\s+(.+)$/;
			$separator = "with";
		} elsif ($command eq "forward") {
			($factoid, $value) = $rest =~ /^(.+?)\s+to\s+(.+)$/;
			$separator = "to";
		} else {
			($factoid, $value) = $rest =~ /^(.+?)\s+is\s+(.+)$/;
			$separator = "is";
		}

		# A few sanity checks ...
		if (!defined($factoid)) {
			return "The " . $command . " command is intended for learning factoids. Please use '}" . $command . " <factoid> " . $separator . " <value>' to add one.";
		} elsif ($value =~ /dcc/i) {
			return "The value contains blocked words (dcc)." ;
		}

		# Legacy syntax: inform the user nicely.
		# NB: blocking is now done in a seperate step with the }block command.
		# TODO: delete this, eventually?
		if ($command eq "learn" && $factoid =~ /^([^=]+?)=(.+)$/) {
			if ($1 eq "block" or $1 eq "forward" or $1 eq "reply") {
				return "This syntax is no longer in use. Please use }" . $1 . " instead.";
			} else {
				return "The learn command is intended for learning simple factoids. Please use '}learn <factoid> is <value>' to add one.";
			}
		}

		# Make direct replies and factoid forwards possible, too...
		# TODO: perhaps expand on the forward interface in the future, to allow e.g. karma forwarding?
		my %opts;
		if ($command eq "reply") {
			return "You don't have dazeus.factoids.reply permissions" if (!$p->has("dazeus.factoids.reply"));
			$opts{reply} = 1;
		} elsif ($command eq "forward") {
			return "You don't have dazeus.factoids.forward permissions" if (!$p->has("dazeus.factoids.forward"));
			$opts{forward} = 1;
		}

		my $result = $self->teachFactoid($factoid, $value, $who, $channel, %opts);
		if ($result == 0) {
			if ($command eq "reply") {
				return "Alright, I will reply to " . $factoid . ".";
			} elsif ($command eq "forward") {
				return "Alright, I will forward " . $factoid . ".";
			} elsif ($command eq "learn") {
				return "Alright, learned " . $factoid . ".";
			}
		} elsif ($result == 1) {
			return "I already know " . $factoid . "; it is '" . $self->getFactoid($factoid, $mess, "short") . "'!";
		}
	}

	# Forgetting factoids.
	elsif ($command eq "unlearn" || $command eq "forget") {
		return "You don't have dazeus.commands.forget permissions." if (!$p->has("dazeus.commands.forget"));

		return "You'll have to give me something to work with, chap." if (!defined($rest) || $rest eq "");

		my $result = $self->forgetFactoid($rest, $who, $channel);
		if ($result == 0) {
			return "Alright, forgot all about " . $rest . ".";
		} elsif ($result == 1) {
			return "I don't know anything about " . $rest . ".";
		} elsif ($result == 2) {
			return "The factoid '" . $rest . "' is blocked; I cannot forget it.";
		}
	}

	# Blocking a factoid.
	elsif ($command eq "block") {
		return "You don't have dazeus.commands.learn.block.convert permissions." if (!$p->has("dazeus.commands.learn.block.convert"));

		return "You'll have to give me something to work with, chap." if (!defined($rest) || $rest eq "");

		my $result = $self->blockFactoid($rest, $who, $channel);
		if ($result == 0) {
			return "Okay, blocked " . $rest . ".";
		} elsif ($result == 1) {
			return "The factoid " . $rest . " was already blocked.";
		}
	}

	# Unblocking a factoid.
	elsif ($command eq "unblock") {
		return "You don't have dazeus.commands.learn.block.unconvert permissions." if (!$p->has("dazeus.commands.learn.block.unconvert"));

		return "You'll have to give me something to work with, chap." if (!defined($rest) || $rest eq "");

		my $result = $self->blockFactoid($rest, $who, $channel);
		if ($result == 0) {
			return "Okay, unblocked " . $rest . ".";
		} elsif ($result == 1) {
			return "The factoid " . $rest . " wasn't blocked.";
		}
	}

	# Statistics! Everyone's favourite biatch.
	elsif ($command eq "factoidstats") {
		return "You don't have dazeus.commands.factoidstats permissions." if (!$p->has("dazeus.commands.factoidstats"));

		return "I know " . $self->countFactoids() . " factoids.";
	}

	# Search for factoids.
	elsif ($command eq "search") {
		return "You don't have dazeus.commands.factoidsearch permissions." if (!$p->has("dazeus.commands.factoidsearch"));

		return "You'll have to give me something to work with, chap." if (!defined($rest) || $rest eq "");

		return "No valid keywords were provided. Keywords must be at least three characters long; shorter ones will be ignored." if (!$self->checkKeywords($rest));

		my ($num_matches, @top5) = $self->searchFactoids($rest);
		if ($num_matches == 1) {
			return "I found one match: '" . $top5[0] . "': " . $self->getFactoid($top5[0], $mess, "short");
		} elsif ($num_matches > 0) {
			return "I found " . $num_matches . " factoids. Top " . (scalar @top5) . ": '" . join("', '", @top5) . "'.";
		} else {
			return "Sorry, I couldn't find any matches.";
		}
	}
}

sub getFactoid {
	my ($self, $factoid, $mess, $mode, @forwards) = @_;
	my $value = $self->get("factoid_" . lc($factoid));
	my $who = $mess->{who};
	my $channel = $mess->{channel};
	$mode = "normal" if (!defined($mode));

	# Do we know this at all?
	if (!defined($value)) {
		return ($mode eq "normal" ? "I don't know $factoid." : undef) ;
	}

	# Fill in some placeholders.
	my $fact = $value->{value};
	$fact =~ s/<who>/$who/gi;
	$fact =~ s/<channel>/$channel/gi;
	push @forwards, $factoid;

	# Traverse forwards if necessary.
	if ($value->{forward}) {
		if ($fact ~~ @forwards) {
			return "[ERROR] Factoid deeplink detected";
		} else {
			return $self->getFactoid($fact, $mess, $mode, @forwards);
		}
	}

	# Finally, the fact we're looking for!
	return $mode eq "normal" && !defined($value->{reply}) ? $factoid . " is " . $fact : $fact;
}

sub teachFactoid {
	my ($self, $factoid, $value, $who, $channel, %opts) = @_;

	# Check whether we already know this one.
	if (defined($self->get("factoid_" . lc($factoid)))) {
		print "DazFactoids: $who tried to teach me $factoid in $channel, but I already know it.\n";
		return 1;
	}

	print "DazFactoids: $who taught me $factoid in $channel with this value and opts:\n$value\n";
	print "-----\n" . Dumper(%opts) . "\n-----\n";

	# Let's learn it already!
	$self->set("factoid_" . lc($factoid), { value => $value, %opts });
	return 0;
}

sub forgetFactoid {
	my ($self, $factoid, $who, $channel) = @_;
	my $value = $self->get("factoid_" . lc($factoid));

	# Is this a factoid known at all?
	if (!defined($value)) {
		print "DazFactoids: $who tried to make me forget $factoid in $channel, but I don't know that factoid.\n";
		return 1;
	}

	# Blocked, perhaps?
	if (defined($value->{block})) {
		print "DazFactoids: $who tried to make me forget $factoid in $channel, but it is blocked.\n";
		return 2;
	}

	print "DazFactoids: $who made me forget $factoid in $channel - factoid had this value:\n";
	print "'" . $value->{value} . "'\n";

	# Let's forget about it already!
	$self->unset("factoid_" . lc($factoid));
	return 0;
}

sub blockFactoid {
	my ($self, $factoid, $who, $channel) = @_;
	my $value = $self->get("factoid_" . lc($factoid));

	# Already blocked?
	if (defined($value->{block})) {
		return 1;
	}

	# Okay chaps, let's block this.
	$value->{block} = 1;
	$self->set("factoid_" . lc($factoid), $value);
	return 0;
}

sub unblockFactoid {
	my ($self, $factoid, $who, $channel) = @_;
	my $value = $self->get("factoid_" . lc($factoid));

	# Not blocked?
	if (!defined($value->{block})) {
		return 1;
	}

	# Let's unblock it, then!
	delete $value->{block};
	$self->set("factoid_" . lc($factoid), $value);
	return 0;
}

sub countFactoids {
	my ($self) = @_;
	my @keys = $self->store_keys();
	my $num_factoids = 0;
	foreach (@keys) {
		++$num_factoids if ($_ =~ /^factoid_(.+)$/);
	}
	return $num_factoids;
}

sub searchFactoids {
	my ($self, $keyphase) = @_;
	my @keywords = split(/\s+/, $keyphase);
	my @keys = $self->store_keys();
	my %matches;
	my $num_matches = 0;

	# Alright, let's search!
	foreach my $factoid (@keys) {
		next if (!($factoid =~ /^factoid_(.+)$/));
		$factoid = $1;

		my $relevance = 0;
		foreach my $keyword (@keywords) {
			next if (length($keyword) < 3);
			$relevance++ if (index($factoid, $keyword) > -1);
		}

		next if $relevance == 0;
		$matches{$factoid} = $relevance;
		$num_matches++;
	}

	# Return the five most relevant results.
	my @sorted = sort { $matches{$b} <=> $matches{$a} } keys %matches;
	return ($num_matches, splice(@sorted, 0, 5));
}

sub checkKeywords {
	my ($self, $keyphrase) = @_;
	my @keywords = split(/\s+/, $keyphrase);

	foreach my $keyword (@keywords) {
		return 1 if length $keyword >= 3;
	}

	return 0;
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
		} elsif($body =~ /^}\s*(\S+)$/) {
				$command = $1;
		} elsif($body =~ /^}\s*(\S+)\s+(.*)$/) {
				$command = $1; $rest = $2;
		} elsif($mess->{channel} eq 'msg') {
			if($body =~ /^\s*(\S+)\s+(.*)\s*$/) {
					$command = $1; $rest = $2;
			} elsif($body =~ /^\s*(\S+)\s*$/) {
					$command = $1;
			}
		} elsif($mess->{raw_body} =~ /^DazNET(?::|,|;)\s+(\S+)$/i) {
				$command = $1;
		} elsif($mess->{raw_body} =~ /^DazNET(?::|,|;)\s+(\S+)\s+(.+)$/i) {
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
