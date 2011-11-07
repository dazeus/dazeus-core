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
package DaZeus2Module::DazFactoids;
use strict;
use warnings;
no warnings 'redefine';
use base qw(DaZeus2Module);
use Data::Dumper;

sub told
{
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	my $who = $mess->{who};
	my $channel = $mess->{channel};
	if($mess->{raw_body} =~ /^\](.+)$/) {
		my $factoid = $1;
		return $self->getFactoid($factoid, "normal", $mess);
	}

	return if !defined $command;
	if($command eq "learn") {
		return "You don't have dazeus.commands.learn permissions."
			if(!$p->has("dazeus.commands.learn"));
		my ($factoid, $value) = $rest =~ /^(.+?)\s+is\s+(.+)$/;
		if(!defined($factoid)) {
			return "Please say '}learn <factoid> is <value>', '}learn ".
				"<opts>=<factoid> is <value>' or ']facthelp' for more help.";
		}
		if($value =~ /dcc/i) {
			return "The value contains blocked words (dcc).";
		}
		
		my $opts;
		if($factoid =~ /^([^=]+?)=(.+)$/) {
			$opts = $1;
			$factoid = $2;
		}
		
		my @opts = split /,/, $opts;
		my %opts;
		foreach(@opts) {
			if($_ eq "block") {
				return "You don't have dazeus.factoids.block permissions"
					if(!$p->has("dazeus.factoids.block"));
				$opts{block} = 1;
			} elsif($_ eq "reply") {
				return "You don't have dazeus.factoids.reply permissions"
					if(!$p->has("dazeus.factoids.reply"));
				$opts{reply} = 1;
			} elsif($_ eq "forward") {
				return "You don't have dazeus.factoids.forward permissions"
					if(!$p->has("dazeus.factoids.forward"));
				$opts{forward} = 1;
			} else {
				return "I don't know the option '$_'.";
			}
		}
		
		if(defined($self->get("factoid_".lc($factoid)))) {
			my $value = $self->getFactoid($factoid, "short", $mess);
			print "DazFactoids: $who tried to teach me $factoid in $channel but I already know it.\n";
			return "... But $factoid is $value!";
		} else {
			print "DazFactoids: $who teached me $factoid in $channel with this value and opts:\n$value\n";
			print "-----\n".Dumper(%opts)."\n-----\n";
			$self->set("factoid_".lc($factoid), {
				value => $value,
				%opts
			});
			return "Ok, learned $factoid.";
		}
	} elsif($command eq "unlearn" or $command eq "forget") {
		return "You don't have dazeus.commands.forget permissions."
			if(!$p->has("dazeus.commands.forget"));
		my $factoid = $rest;
		my $value   = $self->get("factoid_".lc($factoid));
		if(defined($value)) {
			if( (ref($value) eq "HASH" and exists $value->{block})
				or (ref($value) ne "HASH" and
				($value =~ /^<BLOCK>/i or $value =~ /^<REPLY>?<?_?BLOCK>/i
				or $value =~ /^<BLOCK>?<?_?REPLY>/i)))
			{
				return "I'm not allowed to forget that factoid.";
			}
			print "DazFactoids: $who made me forget $factoid in $channel - factoid had this value:\n";
			print ((ref($value) eq "HASH" ? $value->{value} : $value)."\n-----\n");
			$self->unset("factoid_".lc($factoid));
			return "Ok, forgot about $factoid.";
		} else {
			print "DazFactoids: $who tried to make me forget $factoid in $channel but I didn't know that factoid.\n";
			return "But I don't know $factoid...";
		}
	} elsif($command eq "blocklearn") {
		return "You don't have dazeus.commands.learn.block permissions."
			if(!$p->has("dazeus.commands.learn.block"));
	        return "Blocklearn is deprecated, use the 'block' option for learn.";
	} elsif($command eq "blockconvert") {
		return "You don't have dazeus.commands.learn.block.convert permissions."
			if(!$p->has("dazeus.commands.learn.block.convert"));
		my ($factoid) = shift @rest;
		my $value = $self->get("factoid_".lc($factoid));
		if(ref($value) eq "HASH") {
			if(defined $value->{block} and $value->{block}) {
				return "$factoid was already a blocked factoid.";
			}
			$value->{block} = 1;
			$self->set("factoid_".lc($factoid), $value);
		} else {
			if($value =~ /^<BLOCK/i or $value =~ /^<REPLY[><_]*?BLOCK/i) {
				return "$factoid was already a blocked factoid.";
			}
			$value .= "<BLOCK>";
			$self->set("factoid_".lc($factoid), $value);
			return "Ok, turned $factoid into a blocked factoid.";
		}
	} elsif($command eq "blockunconvert") {
		return "You don't have dazeus.commands.learn.block.unconvert permissions."
			if(!$p->has("dazeus.commands.learn.block.unconvert"));
		my ($factoid) = shift @rest;
		my $value = $self->get("factoid_".lc($factoid));
		if(ref($value) eq "HASH") {
			if($value->{block}) {
				delete $value->{block};
				$self->set("factoid_".lc($factoid), $value);
				return "Okay, turned $factoid into a normal factoid.";
			} else {
				return "But $factoid isn't a blocked variable...";
			}
		} else {
			if($value =~ /^<BLOCK>(.*)$/i) {
				$self->set("factoid_".lc($factoid), $1);
				return "Okay, turned $factoid into a normal factoid.";
			} else {
				return "But $factoid isn't a blocked variable...";
			}
		}
    } elsif($command eq "forceunlearn" or $command eq "forceforget") {
    	return "You don't have dazeus.commands.forget.force permissions."
			if(!$p->has("dazeus.commands.forget.force"));
        my $factoid = $rest;
        if(defined($self->get("factoid_".lc($factoid)))) {
        	$self->unset("factoid_".lc($factoid));
            return "Ok, forgot about $factoid (with force :( ).";
        } else {
            return "But I don't know $factoid?";
        }
    } elsif($command eq "factoidstats") {
	return "You don't have dazeus.commands.factoidstats permissions."
		if(!$p->has("dazeus.commands.factoidstats"));
	my @keys = $self->store_keys();
	my @factoids;
	foreach(@keys) {
		push @factoids, $1 if($_ =~ /^factoid_(.+)$/);
	}
	
	my $return = "We have " . scalar @factoids . " factoids. ";
	my ($oldstyle, $newstyle);
	foreach(@factoids) {
		if(ref($self->get("factoid_".lc($_))) eq "HASH") {
			++$newstyle;
		} else {
			++$oldstyle;
		}
	}
	my $newstyle_perc = $newstyle / scalar(@factoids) * 100;
	my $oldstyle_perc = $oldstyle / scalar(@factoids) * 100;
	$return .= "$newstyle (${newstyle_perc}%) of those are new-style factoids, ".
			   "$oldstyle (${oldstyle_perc}%) are old-style.";
	
	return $return;
   } elsif($command eq "pastefactoidlist") {
		return "You don't have dazeus.commands.pastefactoidlist permissions."
			if(!$p->has("dazeus.commands.pastefactoidlist"));
		my $paster = $self->bot->module("DazPaster");
		if(defined($paster)) {
			my @keys = $self->store_keys();
			my @factoids;
			foreach(@keys) {
				push @factoids, $1 if($_ =~ /^factoid_(.+)$/);
			}
			my $list = join "\n", @factoids;
			$paster->Paste("DaZeus", "Factoid list", "Factoid list:\n".$list, 
				sub {
					my $id = shift;
					$self->bot->reply($mess, "Okay, pasted to paster: http://paster.dazjorz.com/?p=$id");
				});
			return 1;
		} else {
			return "I couldn't access DazPaster to paste the list.";
		}
	}
}

sub getFactoid {
	my ($self, $factoid, $mode, $mess, @forwards) = @_;
	my $who = $mess->{who};
	my $chan = $mess->{channel};
	$mode ||= "normal";
	my $value = $self->get("factoid_".lc($factoid));
	if(!defined($value)) {
		return "I don't know $factoid.";
	}
	
	if(ref($value) eq "HASH") {
		my $fact = $value->{value};
		$fact =~ s/<who>/$who/gi;
		$fact =~ s/<channel>/$chan/gi;
		push @forwards, $factoid;
		
		if($value->{reply}) {
			return $fact;
		} elsif($value->{forward}) {
			foreach(@forwards) {
				if($_ eq $fact) {
					return "[ERROR] Factoid deeplink detected";
				}
			}
			return $self->getFactoid($fact, $mode, $mess, @forwards, $fact);
		} else {
			return $fact if $mode eq "short";
			return $factoid." is ".$fact;
		}
	} else {
		$value =~ s/<who>/$who/gi;
		$value =~ s/<channel>/$chan/gi;
		push @forwards, $factoid;
	
		if($value =~ /^<REPLY>(.*)$/i) {
			return $1;
		} elsif($value =~ /^<FORWARD>(.+)$/i) {
			foreach(@forwards) {
				if($_ eq $1) {
					return "[ERROR] Factoid deeplink detected";
				}
			}
			return $self->getFactoid($1, $mode, $mess, @forwards, $1);
		} elsif($value =~ /^<BLOCKREPLY>(.*)$/i or $value =~ /^<BLOCK><REPLY>(.*)$/i) {
			return $1;
		} elsif($value =~ /^<BLOCK>(.*)$/i) {
			return $1 if $mode eq "short";
			return $factoid." is ".$1;
		} else {
			return $value if $mode eq "short";
			return $factoid." is ".$value;
		}
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
                $command = lc($command);
                return ($command, $rest, @rest) if wantarray;
                return [$command, $rest, @rest];
        } else {
                return undef;
        }
}


1;
