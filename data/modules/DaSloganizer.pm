# DaZeus - A highly functional IRC bot
# Copyright (C) 2007  Sjors Gielen <sjorsgielen@gmail.com>
#
## Sloganizer-module
# Copyright (C) 2011  Aaron van Geffen <aaron@aaronweb.net>
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

package DaZeus2Module::DaSloganizer;

use strict;
use warnings;

use base qw(DaZeus2Module);

my @slogans;

sub loadSlogans {
	my ($self) = @_;

	open SLOGANS, "<./slogans.txt" or warn("Unable to load slogans: $!") and return;

	foreach(<SLOGANS>) {
		$_ =~ s/\R//g;
		push (@slogans, $_);
	}
	close SLOGANS;
}

sub sloganize {
	my ($self, $name) = @_;

	warn("Tried to generate a slogan for $name, but no slogan templates have been loaded!") and return if !@slogans;

	my $slogan = $slogans[rand @slogans];
	$slogan =~ s/\%s/$name/g;

	return $slogan;
}

sub told {
	my ($self, $msg) = @_;
	my ($command, $rest, undef) = $self->parseMsg($msg);

	return if !defined $command or $command ne "s" and $command ne "slogan" and $command ne "sloganize";

	# Load slogans if no templates are loaded quite yet.
	$self->loadSlogans() if !@slogans;

	# So, who or what do we Sloganize? (Pick the sender if they're being lazy.)
	my $topic = $rest ? $rest : $msg->{who};

	# Slogan time!
	$self->bot->reply($msg, $self->sloganize($topic));
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
