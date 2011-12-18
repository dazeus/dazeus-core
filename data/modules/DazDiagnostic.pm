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
package DaZeus2Module::DazDiagnostic;
use strict;
use warnings;
no warnings 'redefine';
use base qw(DaZeus2Module);

sub told
{
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	my $who = $mess->{who};
	return if !defined $command;
	my $output;
	if($command eq "getid") {
		return "You don't have dazeus.commands.getid permissions."
			if(!$p->has("dazeus.commands.getid"));
		eval {
			$output = `id`;
		};
		return $@ ? $@ : $output;
	} elsif($command eq "uptime") {
		return "You don't have dazeus.commands.uptime permissions."
			if(!$p->has("dazeus.commands.uptime"));
		eval {
			$output = `uptime`;
		};
		return $@ ? $@ : $output;
	} elsif($command eq "pwd") {
		return "You don't have dazeus.commands.pwd permissions."
			if(!$p->has("dazeus.commands.pwd"));
		eval {
			$output = `pwd`;
		};
		return $@ ? $@ : $output;
	} elsif($command eq "ls") {
		return "You don't have dazeus.commands.ls permissions."
			if(!$p->has("dazeus.commands.ls"));
		return "Single quotes not allowed." if($rest =~ /'/);
		return "Backquotes at the end not allowed." if($rest =~ /\\$/);
		eval {
			$output = `ls '$rest'`;
			$output =~ s/\n/; /g;
		};
		return $@ ? $@ : $output;
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
