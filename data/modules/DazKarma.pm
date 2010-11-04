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
package DaZeus2Module::DazKarma;
use strict;
use warnings;
no warnings 'redefine';
use base qw(DaZeus2Module);

sub init {
	my $self = shift;
	#$self->set("karmabans", {});
	#$self->set("bans", {});
}

sub seen
{
	my ($self, $mess) = @_;
	my $body = $mess->{raw_body};
	my $p = $mess->{perms};
	my $num=0;
	if(!defined($p)) {
		my $mod = $self->bot->module("DazAuth");
		if(!defined($mod)) {
			return "DazKarma cannot function without DazAuth being loaded.";
		}
		$p = $mod->getPermissions($mess)
	}
	my $w = lc($mess->{who});
	while($body =~ /\[(.+?)\](\+\+|--)/g) {
		last if(++$num>=5);
		my $thing = lc($1);
		my $op = $2 eq "++" ? "higher" : "lower";
		my $karma = $self->get("karma_$thing");
		$karma = 0 if(!defined($karma));
		if($thing eq lc($mess->{who}) and !$p->has("dazeus.karma.higher.self") and $op eq "higher") {
			$karma--;
			if($p->has("dazeus.karma.higher.self.message")) {
				$self->bot->reply($mess, "Bah! You're trying to increase your own karma? Bad you! Your karma has been lowered to $karma, ".$mess->{who}.".");
			}
		} else  {
			if($p->has("dazeus.karma.$op")) {
				if($op eq "higher") {
					$karma++;
					$self->bot->reply($mess, $mess->{who}." increased karma of $thing to $karma");
					print "$w increased karma of $thing to $karma.\n";
				} 
				else {
					$karma--;
					$self->bot->reply($mess, $mess->{who}." decreased karma of $thing to $karma");
					print "$w decreased karma of $thing to $karma.\n";
				}
			}
		}
		if($karma!=0) {
			$self->set("karma_$thing", $karma);
		} else {
			$self->unset("karma_$thing");
		}
	}
	while($body =~ /(?:\((.+?)\)|(\S+?))(\+\+|--)/g) {
		last if(++$num>=5);
		my $thing = lc($1) || lc($2);
		my $op = $3 eq "++" ? "higher" : "lower";
		my $karma = $self->get("karma_$thing");
		$karma = 0 if(!defined($karma));
		if($thing eq lc($mess->{who}) and !$p->has("dazeus.karma.higher.self") and $op eq "higher") {
			if($p->has("dazeus.karma.higher.self.message")) {
				$self->bot->reply($mess, "Bah! You're trying to increase your own karma? Bad you!");
			}
			$karma--;
		}
		else {
			if($p->has("dazeus.karma.$op"))
			{
				if($op eq "higher") {
					$karma++;
					print "$w increased karma of $thing to $karma.\n";
				}
				else {
					$karma--;
					print "$w decreased karma of $thing to $karma.\n";
				}
			}
		}
		if($karma!=0) {
			$self->set("karma_$thing", $karma);
		} else {
			$self->unset("karma_$thing");
		}
	}
	if($num>=5) {
		my $newkarma = $self->decrease_karma($w);
		$self->bot->reply($mess, $w."--: You karma-spamwhore!\n");
		print "I lowered karma for $w to $newkarma (karma-spamming).\n";
	}
}

sub setkarma {
	my ($self, $thing, $karma) = @_;
	if($karma!=0) {
		$self->set("karma_$thing", $karma);
	} else {
		$self->unset("karma_$thing");
	}
	return $karma;
}
sub increase_karma {
	my ($self, $thing) = @_;
	$thing=lc($thing);
	my $karma = $self->get("karma_$thing");
	$karma = 0 if(!defined($karma));
	return $self->setkarma($thing, $karma+1);
}
sub decrease_karma {
	my ($self, $thing) = @_;
	$thing=lc($thing);
	my $karma = $self->get("karma_$thing");
	$karma = 0 if(!defined($karma));
	return $self->setkarma($thing, $karma-1);
}

sub told {
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	my $who = $mess->{who};
	return if !defined $command;
	if($command eq "karma") {
		return "You don't have dazeus.commands.karma permissions."
			if(!$p->has("dazeus.commands.karma"));
		my $thing = lc($rest);
		if(!defined($thing) or !length($thing)) {
			return "Syntax: karma <forwhat> - will display karma for \$forwhat.";
		}
		my $karma = $self->get("karma_$thing");
		$karma = 0 if(!defined($karma));
		if($karma == 0) {
			return "$thing has neutral karma.";
		} else {
			return "$thing has a karma of $karma.";
		}
	}
}

sub parseMsg {
        my ($self, $mess) = @_;
        my $body = $mess->{body};

        if($body =~ /^}/ or $mess->{channel} eq 'msg' or $mess->{raw_body} =~ /^DazNET(?::|,|;)/i) {
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
                $command = lc($command);
                return ($command, $rest, @rest) if wantarray;
                return [$command, $rest, @rest];
        } else {
                return undef;
        }
}


1;
