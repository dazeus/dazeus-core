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
package DaZeus2Module::DazLoader;
use warnings;
no warnings 'redefine';
use strict;
use base qw(DaZeus2Module);

sub connected {
	my ($self) = @_;
	# Identify to Nickserv
	my $nickpass = $self->get("nickservpass");
	if(!defined($nickpass)) {
		warn("-- Couldn't identify: Nickservpass not set. --\n");
	} else {
		$self->say({
			who => 'NickServ',
			channel => 'msg',
			body => 'IDENTIFY '.$nickpass,
		});
		warn("-- Identified to Nickserv. --\n");
	}
}

sub told {
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	return undef if !defined($command);
	if($command eq "reload") {
		return "You don't have dazeus.commands.reload permissions."
			if(!$p->has("dazeus.commands.reload"));
		my $module = shift @rest;
		if(!-e "./modules/$module.pm") {
			return "Couldn't find $module.\n";
		}
		eval { $self->bot->reload($module) };
		if($@) {
			my @errors = split /\n/, $@;
			warn "ERROR: ".$@;
			my $paster = $self->bot->module("DazPaster");
			if(defined($paster)) {
				$paster->Paste("DaZeus", "Error reloading $module", "Could not reload $module:\n$@", 
					sub {
						my $id = shift;
						$self->bot->reply($mess, "Could not reload $module because of ".@errors." errors: http://paster.dazjorz.com/?p=$id\n");
					});
				return 1;
			} else {
				return "Couldn't reload $module because of ".@errors." errors.\n";
			}
		}
		return "Reloaded $module.";
	} elsif($command eq "reloaddazloader") {
		return "You don't have dazeus.commands.reload permissions."
			if(!$p->has("dazeus.commands.reload"));
		# in case of emergency...
		eval { $self->bot->reload("DazLoader") };
		if($@) {
			my @errors = split /\n/, $@;
			warn "ERROR: ".$@;
			return "Couldn't reload DazLoader because of ".@errors." errors.\n";
		}
	} elsif($command eq "load") {
		return "You don't have dazeus.commands.load permissions."
			if(!$p->has("dazeus.commands.load"));
		my $module = shift @rest;
		if(!defined($module)) {
			return "Syntax: load <module>";
		}
		if(!-e "./modules/$module.pm") {
			return "Couldn't find $module.\n";
		}
		eval { $self->bot->load($module) };
		if($@) {
			return "$module: Already loaded!\n" if($@ =~ /^Already loaded/);
			my @errors = split /\n/, $@;
			warn "ERROR: ".$@;
                        my $paster = $self->bot->module("DazPaster");
                        if(defined($paster)) {
                                $paster->Paste("DaZeus", "Error loading $module", "Could not load $module:\n$@",
                                        sub {
                                                my $id = shift;
                                                $self->bot->reply($mess, "Could not load $module because of ".@errors." errors: http://paster.dazjorz.com/?p=$id\n");
                                        });
				return 1;
                        } else {
				return "Couldn't load $module because of ".@errors." errors.\n";
			}		
		}
		return "Loaded $module.";
	} elsif($command eq "unload") {
		return "You don't have dazeus.commands.unload permissions."
			if(!$p->has("dazeus.commands.unload"));
		my $module = shift @rest;
		if($module eq "DazLoader") {
			return "Don't! You bastard!\n";
		}
		elsif(!-e "./modules/$module.pm") {
			return "Couldn't find $module.\n";
		}
		$self->bot->unload($module);
		return "Unloaded $module.";
	} elsif($command eq "nickserv_identify") {
		return "You don't have dazeus.commands.nickserv.identify permissions."
			if(!$p->has("dazeus.commands.nickserv.identify"));
		my $pass = shift @rest;
		$pass  ||= $self->get("nickservpass");
		$self->say({
			who => "NickServ",
			channel => 'msg',
			body => "IDENTIFY $pass",
		});
		return "Ok.\n";
	} elsif($command eq "nickserv_setpass") {
		return "You don't have dazeus.commands.nickserv.setpass permissions."
			if(!$p->has("dazeus.commands.nickserv.setpass"));
		my $pass = shift @rest;
		if(!defined($pass)) {
			return "I need a new password.\n";
		}
		$self->set("nickservpass", $pass);
		return "okay, reset nickserv password.\n";
	} elsif($command eq "shutdown" or $command eq "sterf") {
		return "You don't have dazeus.commands.shutdown permissions."
			if(!$p->has("dazeus.commands.shutdown"));
		my $who = $mess->{who};
		warn("$who asked me to shutdown!");
		exit;
	} elsif($command eq "poepost") {
		return "You don't have dazeus.commands.poepost permissions."
			if(!$p->has("dazeus.commands.poepost"));
		my $alias = shift @rest;
		my $event = shift @rest;
		POE::Kernel->post($alias => $event => @rest);
		return "Ok, done.";
	}
	return undef;
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
