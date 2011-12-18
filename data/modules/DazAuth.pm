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
package Permissions;
use warnings;
no warnings 'redefine';
use strict;

my %defaults = (
	# type => [qw(
	#     dazeus.commands.permission
	#     dazeus.commands.shutdown
	# )],
	# global: things everybody may do
	# (adding to this list after loading this module doesn't help,
	# it is only read the first time the module is run!)
	global => [qw(
		dazeus.commands.checkident
		dazeus.commands.hasperm
		dazeus.commands.makeadmin
		dazeus.commands.host
		dazeus.karma.higher
		dazeus.karma.higher.self.message
		dazeus.karma.lower
		dazeus.commands.karma
		dazeus.commands.lastfm
		dazeus.commands.lispcafe.mop
		dazeus.commands.lispcafe.r5rs
		dazeus.commands.lispcafe.clhs
		dazeus.commands.fortune
		dazeus.commands.order
		dazeus.commands.rot13
		dazeus.commands.rand
		dazeus.games.mastermind.new
		dazeus.games.mastermind.guess
		dazeus.games.mastermind.quit
		dazeus.games.mastermind.getthecode
		dazeus.games.mastermind.help
		dazeus.dazbrain.handle
		dazeus.commands.dazbrain
		dazeus.commands.notify
		dazeus.commands.memo
		dazeus.commands.paste
		dazeus.commands.perldoc
		dazeus.commands.phpref
		dazeus.commands.pokemon
		dazeus.quiz.guess
		dazeus.commands.quiz
		dazeus.commands.shorten
		dazeus.spell
		dazeus.commands.spell
		dazeus.commands.woordenboek
		dazeus.commands.hylke.quote
		dazeus.commands.eval.perl
		dazeus.commands.eval.ruby
		dazeus.commands.eval.java
		dazeus.commands.eval.python
		dazeus.factoids.reply
		dazeus.factoids.forward
	)],
	# identified: things everyone who is identified may do
	identified => [qw(
	)],
	# channels_msg: things everyone may do in private message
	'channels_msg' => [qw(
		dazeus.games.mastermind
	)],
	'admin' => [qw(
		dazeus.commands.makeadmin
		dazeus.commands.hasperm
		dazeus.commands.giveperm
		dazeus.commands.dumpperms
		dazeus.commands.join
		dazeus.commands.leave
		dazeus.commands.cycle
		dazeus.commands.reautojoin
		dazeus.commands.clearchannels
		dazeus.commands.autojoin.add
		dazeus.commands.autojoin.del
		dazeus.commands.autojoin.get
		dazeus.commands.dumpchannels
		dazeus.commands.reload
		dazeus.commands.load
		dazeus.commands.unload
		dazeus.commands.nickserv.identify
		dazeus.commands.nickserv.setpass
		dazeus.commands.shutdown
		dazeus.commands.poepost
		dazeus.commands.lastfm.shutdown
		dazeus.commands.lispcafe.loadlisp
		dazeus.commands.dosay
		dazeus.commands.dosay.setchannel
		dazeus.commands.op
		dazeus.commands.deop
		dazeus.commands.csop
		dazeus.commands.voice
		dazeus.commands.devoice
		dazeus.commands.kick
		dazeus.commands.phpref
		dazeus.commands.loadpokedatabase
		dazeus.commands.loadquiz
		dazeus.commands.dumpquiz
		dazeus.factoids.reply
	)],
);

sub new {
	# new creates an empty permissions object, with
	# everything set to 'no'
	my $self = {
		perms => {},
	};
	bless $self, "Permissions";
	return $self;
}

sub new_from_default {
	my ($self, $type) = @_;
	my $perms = Permissions->new();
	my $defaults = $defaults{$type};
	foreach(@$defaults) {
		$perms->set($_);
	}
	return $perms;
}

sub set {
	my ($self, $what) = @_;

	my @what = split /\./, $what;
	my $tree = $self->getLastTree(@what);
	$tree->{this} = 1;
}

sub unset {
	my ($self, $what) = @_;
	
	my @what = split /\./, $what;
	my $tree = $self->getLastTree(@what);
	$tree->{this} = 0;
}

sub force_unset {
	my ($self, $what) = @_;
	my @what = split /\./, $what;
	my $tree = $self->getLastTree(@what);
	$tree->{this} = -1;
}

sub has {
	my ($self, $what) = @_;
	# Every piece (e.g. dazeus.commands.hello) in the hierarchy can be
	# 1 being 'yes' or 0 being 'no' (or undef which is the same as 0).
	# alternatively there is -1, being 'force-no' - if global 'dazeus.flood'
	# is 1, and channel #flood has 'dazeus.flood' set to -1, the merge will be
	# -1 too, so we return 0 here.

	return 0 if(!defined($what));
	my @what = split /\./, $what;
	my $tree = $self->getLastTree(@what);
	$tree->{this} = 0 if(!defined($tree->{this}));
	return $tree->{this} == 1 ? 1 : 0; # can be -1 too, etc.
}

sub getLastTree {
	my ($self, @hier) = @_;
	my $cur = $self->{perms};
	foreach(@hier) {
		if(!defined($cur->{$_})) {
			$cur->{$_} = {};
		}
		$cur = $cur->{$_};
	}
	return $cur;
}

sub merge {
	my ($self, @perms) = @_;
	# Merge: Create one permissions object that contains everything
	# the other permissions have.
	my $perms = $self->new();

	foreach(@perms) {
		$self->merge_foreach($_->{perms}, $perms->{perms});
	}
	return $perms;
}

sub merge_foreach {
	my ($self, $tree, $newtree) = @_;
	foreach my $node (keys %$tree) {
		if(ref($tree->{$node}) eq "HASH") {
			# another tree
			$newtree->{$node} = {} if(!defined($newtree->{$node}));
			$self->merge_foreach($tree->{$node}, $newtree->{$node});
		} else {
			# an item
			# if it is positive, we give it to the new tree too
			if(defined($tree->{$node}) and $tree->{$node}) {
				if(defined($newtree->{$node}) and $newtree->{$node} == -1) {
					# must keep it at -1.
				} else {
					if($tree->{$node} == -1) {
						$newtree->{$node} = -1;
					} else {
						$newtree->{$node} = 1;
					}
				}
			}
		}
	}
}

package DaZeus2Module::DazAuth;
use warnings;
no warnings 'redefine';
use strict;
use base qw(DaZeus2Module);

sub init {
	my $self = shift;

	# $perms = {
	#     nicks => {
	#         dazjorz => Permissions,
	#         somebody => Permissions,
	#     },
	#     channels => {
	#         #codeyard => Permissions,
	#         #dazjorz  => Permissions,
	#         msg       => Permissions,
	#     },
	#     modes   => {
	#         identified => Permissions,
	#         global     => Permissions,
	#     },
	# },
	
	if(!defined($self->get("permissions"))) {
		$self->set("permissions", {
			nicks => {},
			channels => {
				msg => Permissions->new_from_default('channels_msg'),
			},
			modes => {
				global => Permissions->new_from_default('global'),
				identified => Permissions->new_from_default('identified'),
			},
		});
	}
}

sub seen
{
	my ($self, $mess) = @_;

	if(!defined($mess->{perms})) {
		my $perms = $self->getPermissions($mess);
		$mess->{perms} = $perms;
	}
}

sub givePermission {
	my ($self, $what, $which, $perm) = @_;
	my $perms = $self->get("permissions");
	my $object;
	if($what eq "channel") {
		$object = $perms->{channels}{$which};
		if(!defined($object)) {
			$object = $perms->{channels}{$which} = Permissions->new();
		}
	} elsif($what eq "nick") {
		$object = $perms->{nicks}{$which};
		if(!defined($object)) {
			$object = $perms->{nicks}{$which} = Permissions->new();
		}
	} elsif($what eq "mode") {
		$object = $perms->{modes}{$which};
		if(!defined($object)) {
			warn "Unknown mode $which in givePermissions.";
			return -1;
		}
	} else {
		warn("Unknown object type in givePermissions: $what");
		return 0;
	}
	$object->set($perm);
	$self->set("permissions", $perms);
	return 1;
}
sub takePermission {
	my ($self, $what, $which, $perm) = @_;
	my $perms = $self->get("permissions");
	my $object;
	if($what eq "channel") {
		$object = $perms->{channels}{$which};
		if(!defined($object)) {
			$object = $perms->{channels}{$which} = Permissions->new();
		}
	} elsif($what eq "nick") {
		$object = $perms->{nicks}{$which};
		if(!defined($object)) {
			$object = $perms->{nicks}{$which} = Permissions->new();
		}
	} elsif($what eq "mode") {
		$object = $perms->{modes}{$which};
		if(!defined($object)) {
			warn "Unknown mode $which in takePermissions.";
			return -1;
		}
	} else {
		warn("Unknown object type in takePermissions: $what");
		return 0;
	}
	$object->unset($perm);
	$self->set("permissions", $perms);
	return 1;
}

sub getPermissions {
	my ($self, $mess) = @_;

	my $perms = $self->get("permissions");

	my $channel = $self->bot->module("DazChannel");
	if(!defined($channel)) {
		warn "WARNING: DazChannel not loaded, giving back empty permissions set.\n";
		return Permissions->new();
	}

	# GLOBAL PERMISSIONS
	my $global = $perms->{modes}{global};

	# NICKNAME PERMISSIONS
	# only if identified!
	my ($nickperms, $identperms);

	if($mess->{checkidentcmd} or $channel->is_identified($mess->{who})) {
		$nickperms = $perms->{nicks}{ $mess->{who} };
		$nickperms = Permissions->new() if(!defined($nickperms));

		$identperms = $perms->{modes}{identified};
	} else {
		$nickperms = Permissions->new(); # Not identified, empty permissions set
		$identperms = Permissions->new();
	}

	# CHANNEL PERMISSIONS
	my $chanperms = $perms->{channels}{ $mess->{channel} };
	$chanperms = Permissions->new() if(!defined($chanperms));
	
	my $new_perms = Permissions->merge($global, $nickperms, $identperms, $chanperms);
	return $new_perms;
}

sub interestingPeople {
	my $self = shift;
	# Advanced auth system stub subroutine.
	# This function will return the "interesting people": The people I know
	# and are worth whoising for their identification-ness.

	# Permissions are saved in the database.
	my $perms = $self->get("permissions");
	return (keys %{ $perms->{nicks} });
}

sub told
{
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	return if !defined $command;
	if($command eq "makeadmin") {
        	return "You don't have dazeus.commands.makeadmin permissions."
           		if(!$p->has("dazeus.commands.makeadmin"));
		# there must be no nicks yet
		if($self->interestingPeople()) {
			return "There already are admins on this bot.";
		}
	
		my $channel = $self->bot->module("DazChannel");
		return "DazChannel can't be accessed, cannot make you an admin.\n" if(!defined($channel));
		
		$self->{makingadmin} = $mess->{who};
		$self->sendWhois($mess->{who});
		return "Going to check if you are identified before I make you an admin.";
	} elsif($command eq "checkident" or $command eq "sudo") {
		# can't check for permissions, since nobody has them ATM.
		#return "You don't have dazeus.commands.checkident permissions."
		#	if(!$p->has("dazeus.commands.checkident"));
		if(!$rest) {
			return "Usage: checkident [command] - your whois will be checked, and command will be executed";
		}
		$self->{checkidentcmd} = [$rest, $mess];
		$self->sendWhois($mess->{who});
		return "Going to check if you are identified before running this command.";
	} elsif($command eq "hasperm") {
		return "You don't have dazeus.commands.hasperm permissions."
			if(!$p->has("dazeus.commands.hasperm"));
		my ($perm) = @rest;
		return "Usage: hasperm <permission>" if(!defined($perm));
		if($mess->{perms}->has($perm)) {
			return "You do have '$perm'.";
		} else {
			return "You do not have '$perm'.";
		}
	} elsif($command eq "giveperm") {
		if(!$p->has("dazeus.commands.giveperm")) {
			return "You don't have dazeus.commands.giveperm permissions.";
		}

		my ($what, $which, $perm) = @rest;

		if(!defined($what) or !defined($which) or !defined($perm)
		or ($what ne "nick" and $what ne "channel" and $what ne "mode")) {
			return "Syntax: giveperm 'nick'|'channel'|'mode' <nick|channel|mode> <permission>\n".
				"Example: giveperm nick someguy dazeus.commands.shutdown\n".
				"Example: giveperm channel #perl dazeus.commands.leave\n";
		}
		my $return = $self->givePermission($what, $which, $perm);
		if(!defined($return) or $return == 0) {
			return "I could not set '$perm' for $what $which.";			
		} elsif($return == 1) {
			return "Ok, I set '$perm' for $what $which.";
		} elsif($return == -1) {
			return "The mode $which is unknown.";
		}
	} elsif($command eq "takeperm") {
		if(!$p->has("dazeus.commands.takeperm")) {
			return "You don't have dazeus.commands.takeperm permissions.";
		}

		my ($what, $which, $perm) = @rest;

		if(!defined($what) or !defined($which) or !defined($perm)
		or ($what ne "nick" and $what ne "channel" and $what ne "mode")) {
			return "Syntax: takeperm 'nick'|'channel'|'mode' <nick|channel|mode> <permission>\n".
				"Example: takeperm nick someguy dazeus.commands.shutdown\n".
				"Example: takeperm channel #perl dazeus.commands.leave\n";
		}
		my $return = $self->takePermission($what, $which, $perm);
		if(!defined($return) or $return == 0) {
			return "I could not take '$perm' for $what $which.";			
		} elsif($return == 1) {
			return "Ok, I took '$perm' for $what $which.";
		} elsif($return == -1) {
			return "The mode $which is unknown.";
		}
	} elsif($command eq "dumpperms") {
		if(!$p->has("dazeus.commands.dumpperms")) {
			return "You don't have dazeus.commands.dumpperms permissions.";
		}		
		use Data::Dumper;
		warn Dumper($self->get("permissions"));
		return "OK, dumped.";
	}
	return undef;
}

sub whois {
	my ($self, $whois) = @_;
	my $admin = delete $self->{makingadmin};
	if(defined $admin and $admin and $whois->{nick} eq $admin) {
		if(defined $whois->{identified} and $whois->{identified}) {
			my $perms = $self->get("permissions");
			
			$perms->{nicks}{ $admin } = Permissions->new_from_default("admin");
		
			$self->set("permissions", $perms);
			$self->say(
				who => $admin,
				body => "OK, you are now the admin on this bot, $admin.",
				channel => 'msg',
			);
		} else {
			$self->say(
				who => $admin,
				body => "Could not make you an admin: You are not identified ".
					"(try '/msg NickServ HELP REGISTER')",
				channel => 'msg',
			);
		}
	} elsif(defined($self->{checkidentcmd})) {
		my ($cmd, $oldmess) = @{ delete $self->{checkidentcmd} };
		if(defined $whois->{identified} and $whois->{identified}) {
			# now we should act as if the command was
			# sent from this person in an identified state
			# we generate a $mess with 'perms' already set, so 
			# Auth doesn't overwrite it with a new one in seen()
			my $mess = {
				checkidentcmd => 1,
				'raw_body' => $cmd,
				who => $oldmess->{who},
				raw_nick => $oldmess->{raw_nick},
				channel => $oldmess->{channel},
				'original_mess' => $oldmess,
				body => $cmd,
			};
			$mess->{perms} = $self->getPermissions($mess);
			# now give it on to BasicBot::Pluggable
			$self->bot->said($mess);
		} else {
                        $self->say(
                                who => $oldmess->{who},
                                body => "Could not run that command: You are not identified ".
                                        "(try '/msg NickServ HELP REGISTER')",
                                channel => $oldmess->{channel},
                        );
		}
	}
}

sub parseMsg {
        my ($self, $mess) = @_;
        my $body = $mess->{body};

        # { argh, stupid vim editing
        if($body =~ /^}/ or $mess->{channel} eq 'msg' or $mess->{raw_body} =~ /^DaZeus(?::|,|;)/i) {
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
                } elsif($mess->{raw_body} =~ /^DaZeus(?::|,|;)\s+(\w+)$/i) {
                        $command = $1;
                } elsif($mess->{raw_body} =~ /^DaZeus(?::|,|;)\s+(\w+)\s+(\w+)$/i) {
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
