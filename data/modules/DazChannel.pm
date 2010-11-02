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
package DaZeus2Module::DazChannel;
use warnings;
no warnings 'redefine';
use strict;
use base qw(DaZeus2Module);

sub init {}

sub reload {
	# needs dazjorz version of Pluggable
	my ($self, $oldmodule) = @_;
	
	$self->{connected} = 1;
	$self->{nicks} = $oldmodule->{nicks};
	$self->{channels} = $oldmodule->{channels};
	foreach(keys %$oldmodule) {
		delete $oldmodule->{$_};
	}
	return undef;
}

sub chanjoin {
        my ($self, $mess) = @_;
        my $who = $mess->{who};
        my $chan = $mess->{channel};
	return if(lc($who) eq lc($self->bot->getNick()));
	warn "$who joins $chan\n";
        my $perms = $self->get("permissions");

	# get the channel lists
	my $channel = $self->{channels}{$chan};
	$channel = $self->{channels}{$chan} = {} if(!defined($channel));

	# get the nick lists
	my $nicks = $self->{nicks};

	# get interesting people
	my $module = $self->bot->module("DazAuth");
	my @interesting = $module->interestingPeople();

	# add nick if it's not there yet
	if(!defined($nicks->{$who})) {
		$nicks->{$who} = {
			nick => $who,
			channels => {},
		};
	}
	# add channel to the nick
	$nicks->{$who}{channels}{$chan} = {};
	
	# add nick to channel
	$channel->{$who} = $nicks->{$who};

	# if it's an interesting guy
	foreach my $in (@interesting) {
		if(lc($in) eq lc($who)) {
			# whois him
			warn("Asking for whois for $who (I'm in \@interesting loop in chanjoin)");
			$self->sendWhois($who);
			last;
		}
	}
}

sub chanpart {
        my ($self, $mess) = @_;
        my $who = $mess->{who};
        my $chan = $mess->{channel};

	# get the nick lists
	my $nicks = $self->{nicks};

	if(lc($who) eq lc($self->bot->getNick)) {
		warn "I left $chan\n";
		# I left a channel, now I need to clean it up
	        my $channel = $self->{channels}{$chan};
	        return if(!defined($channel));

	        foreach my $user(keys %$channel) {
			warn "Cleaning up for $user.\n";
			my %chanlist = %{ $nicks->{$user}{channels} };
			my %chanlist_new;
			foreach(keys %chanlist) {
				if(lc($_) ne lc($chan)) {
					$chanlist_new{$_} = $chanlist{$_};
				}
			}
		        if(%chanlist_new) {
				warn "Links left: " . join(", ", keys(%chanlist_new)) . ".\n";
		                # he's still in channels, don't remove him.
		                $nicks->{$user}{channels} = \%chanlist_new;
		        } else {
				warn "(No link anymore)\n";
		                # we don't have any link with him anymore, remove him.
		                delete $nicks->{$user};
		        }
	        }
		delete $self->{channels}{$chan};
		return;
	}

	warn "$who parts $chan\n";
        my $perms = $self->get("permissions");

	# remove channel from nick
	my %channellist = %{ $nicks->{$who}{channels} };
	my %channellist_new;
	foreach(keys %channellist) {
		if(lc($_) ne lc($chan)) {
			$channellist_new{$_} = $channellist{$_};
		}
	}

	if(%channellist_new) {
		# he's still in channels, don't remove him.
		$nicks->{$who}{channels} = \%channellist_new;
	} else {
		# we don't have any link with him anymore, remove him.
		delete $nicks->{$who};
	}
	
	# get the channel lists
	my $channel = $self->{channels}{$chan};
	return if(!defined($channel));

	# remove nick from channel
	my %channel_new;
	foreach(keys %$channel) {
		if(lc($_) ne lc($who)) {
			$channel_new{$_} = $channel->{$_};
		}
	}
	$self->{channels}{$chan} = \%channel_new;
}

sub nick_change {
	my $self = shift;
	my ($from, $to) = @_;
	# okay, now. We should remove the info on the old nick
	# in the database, and then set up a delay to recheck whois
	# in five seconds or so.
	# That should give 'em time to (auto-)identify.
	$self->{nickchange} = [] if(!defined($self->{nickchange}));

	# make a new nick object based on the old one
	my $channels = $self->{nicks}{$from}{channels};
	$channels ||= [];

	$self->{nicks}{$to} = {
		nick => $to,
		channels => $channels,
	}; # we wait for the whois to update the rest

	# and we remove the old nick
	delete $self->{nicks}{$from};

	# and we update all channels
	foreach my $chan(@$channels) {
		my $channel = $self->{channels}{$chan};
		next if(!defined($channel));
		my @newchannel;
		foreach my $user(@$channel) {
			if(lc($user->{nick}) eq lc($from)) {
				push @newchannel, $self->{nicks}{$to};
			} else {
				push @newchannel, $user;
			}
		}
		$self->{channels}{$chan} = \@newchannel;
	}

	# and we ask for a whois
	push @{ $self->{nickchange} }, {
		from => $from,
		to   => $to,
		time => time,
		wait => 5,
	};
}

sub connected {
	my $self = shift;

	$self->set("channels", []);
	
	warn "Initialising channels...\n";
	$self->{connected} = 1;
	$self->{initialised} = 0;
	
	$self->{nicks} = {};
	$self->{channels} = {};
}

sub tick {
	# We give other modules a chance to initialise, that's why we put this piece in tick() (run after 5 seconds)
	my $self = shift;
	if($self->{'connected'} and !$self->{initialised}) {
		$self->{initialised} = 1;
		$self->autojoin();
	} elsif(defined $self->{nickchange} and @{ $self->{nickchange} }) {
		# handle them
		my @new_nickchange;
		foreach(@{ $self->{nickchange} }) {
			if($_->{time} + $_->{wait} >= time) {
				warn("Asking for whois for ".$_->{to}." (in tick())");
				$self->sendWhois($_->{to});
			} else {
				push @new_nickchange, $_; # handle later (give 'em time to ident)
			}
		}
		$self->{nickchange} = \@new_nickchange;
	}
}

sub is_identified {
	my ($self, $nick) = @_;
	my $user = $self->{nicks}{$nick};
	return 0 if(!defined($user));
	my $i = $user->{identified};
	return 0 if(!defined($i));
	return $i;
}

sub got_names {
	my ($self, $names) = @_;
	my $channelname = $names->{channel};
	$names = $names->{names};

	warn "Got names for $channelname.\n";

	my $channel = $self->{channels}{$channelname} = {};

	my $nicks = $self->{nicks};

	my $module = $self->bot->module("DazAuth");
	my @interesting = $module->interestingPeople();
	my @interesting_left;

	foreach my $nick (keys %$names) {
		#warn ("* Found $nick in $channelname.\n");
		my $mode = $names->{$nick};
		if(!defined($nicks->{$nick})) {
			$nicks->{$nick} = {
				nick => $nick,
				channels => {},
			};
		}
		$nicks->{$nick}{channels}{$channelname} = {
			mode => ($mode->{op}    ? "o" :
				$mode->{voice} ? "v" : ""),
		};
		$channel->{$nick} = $nicks->{$nick};
		@interesting_left = ();
		foreach my $in (@interesting) {
			#warn("Testing interesting nick $in.\n");
			if(lc($in) eq lc($nick)) {
				warn("Asking for $nick whois in got_names");
				$self->sendWhois($nick);
			} else {
				push @interesting_left, $in;
			}
		}
		@interesting = @interesting_left;
	}
}

sub whois {
	my ($self, $whois) = @_;
	my $nick = $whois->{nick};
	warn "Got whois for $nick\n";
	
	$nick = $self->{nicks}{$nick};
	return if(!defined($nick));
	foreach(keys %$whois) {
		$_ = "whois_channels" if($_ eq "channels");
			# don't want to overwrite our 'channels'.
		$nick->{$_} = $whois->{$_};
	}
}

sub autojoin {
	my $self = shift;

	warn "-- Joining AutoJoin channels... --\n";

	my $chans = $self->get("autojoin");
	if(!defined($chans)) {
		$chans = [];
		$self->set("autojoin", $chans);
	}

	if(ref($chans) ne "ARRAY") {
		my @chans = split /\s+/, $chans;
		$chans = \@chans;
		$self->set("autojoin", $chans);
	}

	foreach(@$chans) {
		next if($_ eq '');
		warn "-- Joining $_... --\n";
		$self->joinChannel($_);
	}
}

sub convertChannels {
	my ($self) = @_;
	my $chans = $self->get("channels");
	my @chans = split /\s+/, $chans;
	$self->set("channels", \@chans);
}

sub seen
{
	my ($self, $mess) = @_;

	my $raw = $mess->{raw_body};
	my ($command, $rest, @rest);
	my $nick = $self->bot->getNick();
	

	if($mess->{channel} eq 'msg') {
		$command = $raw;
	} elsif($raw =~ /^}(.+)$/) {
		$command = $1;
	} elsif($raw =~ /^$nick(?::|,|;)(.+)$/i) {
		$command = $1;
	}

	if(!defined($command) or $command !~ /\S/) {
		return;
	}

	# clear spaces at end and start
	$command =~ s/\s*$//gi;
	$command =~ s/^\s*//gi;

	if($command =~ /\s/) {
		($command, $rest) = $command =~ /(.+?)\s+(.+)/i;
		@rest = split /\s+/, $rest;
	} else {
		$rest = undef;
		@rest = ();
	}

        $command = lc($command);
	$mess->{command} = $command;
	$mess->{rest} = $rest;
	$mess->{restarray} = \@rest;
	return;
}

sub told
{
	my ($self, $mess) = @_;
	my $p    = $mess->{perms};
	my $body = $mess->{body};
	my ($command, $rest, $restarr) = ($mess->{command}, $mess->{rest}, $mess->{restarray});
	return if(!defined($command));
	my @rest = @$restarr;
	if($command eq "join") {
		return "You don't have dazeus.commands.join permissions."
			if(!$p->has("dazeus.commands.join"));
		my $channellist = [];

		push @rest, $mess->{channel} if(@rest == 0);

		foreach(@rest) {
			$self->joinChannel($_);
			push @$channellist, $_;
		}

		return "Ok, joined ".join(", ", @$channellist);
	} elsif($command eq "leave") {
		return "You don't have dazeus.commands.leave permissions."
			if(!$p->has("dazeus.commands.leave"));
		my $channellist = [];
		
		push @rest, $mess->{channel} if(@rest == 0);

		foreach(@rest) {
			$self->leaveChannel($_);
			push @$channellist, $_;
		}

		return "Ok, left ".join(", ", @$channellist);
	} elsif($command eq "cycle") {
		return "You don't have dazeus.commands.cycle permissions."
			if(!$p->has("dazeus.commands.cycle"));
		my $channellist = [];

		push @rest, $mess->{channel} if(@rest == 0);

		foreach(@rest) {
			$self->leaveChannel($_);
			$self->joinChannel($_);
			push @$channellist, $_;
		}

		return "Ok, cycled ".join(", ", @$channellist);
	} elsif($command eq "reautojoin") {
		return "You don't have dazeus.commands.reautojoin permissions."
			if(!$p->has("dazeus.commands.reautojoin"));
		$self->autojoin();
		return "Done.";
	} elsif($command eq "clearchannels") {
		return "You don't have dazeus.commands.clearchannels permissions."
			if(!$p->has("dazeus.commands.clearchannels"));
		$self->set("channels", []);
		return "Done.";
	} elsif($command eq "add_autojoin") {
		return "You don't have dazeus.commands.autojoin.add permissions."
			if(!$p->has("dazeus.commands.autojoin.add"));
		my $channels_added = [];

		my $channels_already = [];

		push @rest, $mess->{channel} if(@rest == 0);	

		my $autojoin = $self->get("autojoin");

		NEW: foreach my $new (@rest) {
			# Check if it's already in the autojoin list
			AUTO: foreach my $auto (@$autojoin) {
				if($auto eq $new) {
					# It is, so add this to the "already added" list
					push @$channels_already, $new;
					next NEW;
				}
			}
			# so it's not, add it
			push @$autojoin, $new;
			push @$channels_added, $new;
		}

		$self->set("autojoin", $autojoin);
		my $return;
		if(@$channels_added == 0) {
			$return = "No channels were added";
		} else {
			$return = "These channels were added: " . join (", ", @$channels_added);
		}
		if(@$channels_already == 0) {
			$return .= ".";
		} else {
			$return .= "; these channels were already in the autojoin list: " . join(", ", @$channels_already) . ".";
		}
		return $return;
	} elsif($command eq "del_autojoin") {
		return "You don't have dazeus.commands.autojoin.del permissions."
			if(!$p->has("dazeus.commands.autojoin.del"));
                
		my $channels_del = []; # Deleted channels
                my $channels_notin = []; # Channels that weren't in the autojoin at all

                push @rest, $mess->{channel} if(@rest == 0);
                my $autojoin = $self->get("autojoin");
		
		# Fill a hashref
		my %autojoin;
		$autojoin{lc($_)} = 1 for(@$autojoin);

		# Remove if it's there
                NEW: foreach my $remove (@rest) {
                        # Check if it's in the autojoin list
                        AUTO: foreach my $auto (keys %autojoin) {
                                if(lc($auto) eq lc($remove)) {
					# Yep, remove it!
					delete $autojoin{lc($auto)};
					push @$channels_del, $remove;
					next NEW;
				}
                        }
			# It wasn't even in the list in the first place.
			push @$channels_notin, $remove;
                }

		# build a new $autojoin
		my $autojoin = [];
		push @$autojoin, $_ for(keys %autojoin);
		$self->set("autojoin", $autojoin);

		my $return;
                if(@$channels_del == 0) {
                        $return = "No channels were removed";
                } else {
                        $return = "These channels were removed: " . join (", ", @$channels_del);
                }
                if(@$channels_notin == 0) {
                        $return .= ".";
                } else {
                        $return .= "; these channels were not in the autojoin list: " . join(", ", @$channels_notin) . ".";
                }

		return $return;
	} elsif($command eq "get_autojoin") {
		return "You don't have dazeus.commands.autojoin.get permissions."
			if(!$p->has("dazeus.commands.autojoin.get"));
		my $autojoin = $self->get("autojoin");
		return "Not autojoining any channels." if(@$autojoin == 0);
		$autojoin = $self->removeDuplicates($autojoin);
		$self->set("autojoin", $autojoin);
		return "Auto-joining channels: " . join(", ", @$autojoin);
	} elsif($command eq "dumpchannels") {
		return "You don't have dazeus.commands.dumpchannels permissions."
			if(!$p->has("dazeus.commands.dumpchannels"));
		my $channels = $self->get("channels");
		$channels = $self->removeDuplicates($channels);
		$self->set("channels", $channels);
		return "I'm in these channels: " . join(", ", @$channels);
	} elsif($command eq "ident_recheck") {
		warn("Asking for whois for $mess->{who} - ident_recheck command");
		$self->sendWhois($mess->{who});
		return "OK, rechecking your whois for identificationness.";
	}
}

sub removeDuplicates {
	my ($self, $channels) = @_;
	my $newchannels;
	CHAN: foreach my $chan (@$channels) {
		NEWCHAN: foreach my $newchan (@$newchannels) {
			if($newchan eq $chan) {
				# This channel is already in the list.
				next CHAN;
			}
		}
		push @$newchannels, $chan;
	}
	return $newchannels;
}

sub joinChannel {
	my ($self, $channel) = @_;
	my $channels = $self->get("channels");
	push @$channels, $channel;
	$self->set("channels", $channels);
	$self->bot->join($channel);
}

sub leaveChannel {
	my ($self, $channel) = @_;
	my $channels = $self->get("channels");
	my @newchannels;
	foreach(@$channels) {
		push @newchannels, $_ if $_ ne $channel;
	}
	$self->set("channels", \@newchannels);
	$self->bot->part($channel);
}

sub getChannels {
	my ($self) = @_;
	my $channels = $self->get("channels");
	$channels = [] if(!defined($channels));
	return @$channels if wantarray();
	return $channels;
}

1;
