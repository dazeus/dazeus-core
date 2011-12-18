# DaZeus - A highly functional IRC bot
# Copyright (C) 2007  Sjors Gielen <sjorsgielen@gmail.com>
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
package DaZeus2Module::DazMessages;
use strict;
use warnings;
no warnings 'redefine';
use DaZeus2Module;
use base qw(DaZeus2Module);
use MIME::Base64;

sub told
{
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	my $who = $mess->{who};
	return if !defined $command;
	if($command eq "koffie") {
		return "Ey, ga dat ff zelf pakken, $who! Ik ben je SLAAF niet!";
	} elsif($command eq "cola" ) {
		return "Okee, omdat jij het bent, $who. *geeft*";
	} elsif($command eq "test" ) {
		return "Test back - I think it works.";
	} elsif($command eq "moo") {
		return "Mooooooooooo!";
	} elsif($command eq "wocka") {
		return "wocka wocka wocka wocka";
	} elsif($command eq "chumba") {
		return "chumba wamba chumba wamba";
	} elsif($command eq "wengo") {
		return "wengo wengo wengo wengo";
	} elsif($command eq "blaat") {
		return "Blaat! (See also ]blata)";
	} elsif($command eq "hey" or $command eq "hello" or $command eq "hi" or
		$command eq "hey!" or $command eq "hello!" or $command eq "hi!") {
		my $hiwho = $rest || $who;
		return "Hi $hiwho!";
	} elsif($command eq "bye" or $command eq "cya" or $command eq "doei" or
		$command eq "bye!" or $command eq "cya!" or $command eq "doei!" ) {
		return "Bye! I'll miss you!";
	} elsif($command eq "sup?") {
		return "Nothing much, you?";
	} elsif($command eq "ping") {
		return "Pong!";
	} elsif($command eq "sex") {
		return "...With you? Hey, umm, do you have a female bot, too..?";
	} elsif($command eq "troost") {
		my $troostwie = $rest;
		$troostwie = $who if($troostwie eq "mij" or $troostwie eq "me");
		return "*legt een arm om ".$troostwie."'s schouders...*";
	} elsif($command eq "fortune") {
		return "You don't have dazeus.commands.fortune permissions."
			if(!$p->has("dazeus.commands.fortune"));
		while(1) {
			my $output;
			eval {
				$output = `fortune -s`;
			};
			if($@) {
				return "Error: $@";
			}
			my @lines  = split /\n/, $output;
			if(@lines < 3) {
				return $output;
			}
		}
	} elsif($command eq "wp") {
		$rest =~ s/ /_/g;
		$rest = ucfirst($rest);
		return "http://en.wikipedia.org/wiki/$rest";
	} elsif($command eq "nwp") {
		$rest =~ s/ /_/g;
		$rest = ucfirst($rest);
		return "http://nl.wikipedia.org/wiki/$rest";
	} elsif($command eq "unp") {
		$rest =~ s/ /_/g;
		$rest = ucfirst($rest);
		return "http://uncyclopedia.org/wiki/$rest";
	} elsif($command eq "flipcoin") {
		my ($coin) = int(rand(2)) - 1;
		if($coin) {
			return "That'll be... Tail!";
		} else {
			return "That'll be... Head!";
		}
	} elsif($command eq "order") {
		return "You don't have dazeus.commands.order permissions."
			if(!$p->has("dazeus.commands.order"));
		my ($what, $forwho) = $rest =~ /^(.+?)\sfor\s(.+)$/i;
		if(!defined($what)) {
			# Didn't match, maybe he didn't give 'for $who'
			$what = $rest;
			$forwho = $mess->{who};
		}
		if(!defined($what)) {
			# Still didn't match. He probably just said '}order'.
			return "Syntax error, please say }order bag of stuff [for somebody]";
		}
		# We make this a seperate variable so we can change it later
		# for known $what's, such as coffee, beer, tea.
		my $string = "slides $what down the bar to $forwho";
		$self->bot->emote(
			channel => $mess->{channel},
			body    => $string
		);
		return 1;
	} elsif($command eq "rand") {
		return "You don't have dazeus.commands.rand permissions."
			if(!$p->has("dazeus.commands.rand"));
		if(@rest == 0) {
			@rest = (0..9, 'a'..'z', 'A'..'Z');
		}
		my $num = int(rand(10))+10;
		my $str;
		while($num) {
			$str .= $rest [ rand @rest ];
			$num--;
		}
		return "Random string: $str";
	} elsif($command eq "random") {
		return "4"; # chosen by fair dice roll
								# guaranteed to be random
								# -- http://xkcd.com/221/
	} elsif($command eq "darkbot" or $command eq "sock" or $command eq "dirtysock") {
		my $fuckwith = $rest ? $rest : $who;
		$self->bot->emote(
			channel => $mess->{channel},
			body    => "throws a dirty sock into ".$fuckwith."'s mouth"
		);
		return 1;
	} elsif($command eq "rot13") {
		return "You don't have dazeus.commands.rot13 permissions."
			if(!$p->has("dazeus.commands.rot13"));
		foreach(@rest) {
			tr/a-zA-Z/n-za-mN-ZA-M/;
		}
		return join(" ", @rest);
	} elsif($command eq "wordtonumbers") {
		my @str = split //, $rest;
		my @newstr;
		foreach(@str) {
			s/[abc]/2/i; s/[def]/3/i;
			s/[ghi]/4/i; s/[jkl]/5/i;
			s/[mno]/6/i; s/[pqrs]/7/i;
			s/[tuv]/8/i; s/[wxyz]/9/i;
			push @newstr, $_;
		}
		return join '', @newstr;
	} elsif($command eq "dosay") {
		return "You don't have dazeus.commands.dosay permissions."
			if(!$p->has("dazeus.commands.dosay"));
		$self->{channelsSet} ||= [];
		foreach(@{ $self->{channelsSet} }) {
			$self->say({
				channel => $_,
				body    => $rest,
			});
		}
		return 1;
	} elsif($command eq "setchannelsay") {
		return "You don't have dazeus.commands.dosay.setchannel permissions."
			if(!$p->has("dazeus.commands.dosay.setchannel"));
		$self->{channelsSet} = \@rest;
		return "Okay, done.";
	} elsif($command eq "isprime" or $command eq "ispriem") {
		if($rest =~ /\D/) {
			return "$rest: Not a number";
		}
		my $limit = 2**16+1;
		if($rest > $limit) {
			return "$rest: Too big (limit is $limit)";
		}
		if($rest < 1) {
			return "$rest: zero";
		}
		my $num = 1;
		eval {
			local $SIG{ALRM} = sub { die("Took too long"); };
			alarm 5;
			1 while 0 != $rest % ++$num;
			alarm 0;
		};
		alarm 0;
		return "Error: Couldn't check if $rest is prime: $@" if($@);
		if($num == $rest) {
			return "$rest is prime!";
		} else {
			return "$rest isn't prime, diviseble by $num.";
		}
	} elsif($command eq "code" ) {
		my $search = $rest;
		if( $search eq "" ) {
			return "Usage: }code [search string]";
		}
		$search =~ s/ /+/g;
		$search =~ s/&/&amp;/g;
		return "http://www.google.com/codesearch?q=" . $search;
	} elsif( $command eq "moeder" or $command eq "jemoeder" 
		 or $command eq "m" ) {
		if($rest eq "") {
			return "Je moeder is een null-pointer!";
		}
		return "Je moeder is " . $rest . "!";
	} elsif( $command eq "sarcasm") {
		return "+-------+\n|Sarcasm|\n+---+---+\n    | (o.o;\n    o=";
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
