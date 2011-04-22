# DaZeus - A highly functional IRC bot
# Copyright (C) 2010  Sjors Gielen <sjorsgielen@gmail.com>
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
package DaZeus2Module::Twitter;
use strict;
use warnings;
no warnings 'redefine';
use base qw(DaZeus2Module);
use Net::Twitter::Lite;
use LWP::UserAgent::POE;
use HTML::Entities;

my $net_twitter;
my $ticks;

sub init {
	my $self = shift;
	$net_twitter = new Net::Twitter::Lite(
		useragent_class => "LWP::UserAgent::POE",
	);
	$ticks = 0;
}

sub tick {
	my $self = shift;
	my $interval = $self->get( "interval" ) || 120;
	if( ++$ticks > $interval )
	{
		$self->updateTwitter();
		$ticks = 0;
	}
}

sub told
{
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	my $who = $mess->{who};
	return if !defined $command;
	if( $command eq "twitterrate") {
		return "You have no dazeus.commands.twitter.rate permissions."
			if(!$p->has("dazeus.commands.twitter.rate"));
		my $rate_lim = $net_twitter->rate_limit_status();

		return "Rate limit status: " . $rate_lim->{'remaining_hits'} . " remaining; reset at "
			. $rate_lim->{'reset_time'};
	}
	elsif( $command eq "resettwitter") {
    		return "You have no dazeus.commands.twitter.reset permissions."
    			if(!$p->has("dazeus.commands.twitter.reset"));
		my $old_id = $self->get("last_id") || "unset";
		$self->set("last_id", 0);
		return "Just reset twitter id. Count was: $old_id";
	}
	elsif( $command eq "twitter_configure" ) {
    		return "You have no dazeus.commands.twitter.configure permissions."
    			if(!$p->has("dazeus.commands.twitter.configure"));
		my $user = $rest[0];
		my $id   = $rest[1];
		my $chan = $rest[2];
		my $lim  = $rest[3];
		my $intv = $rest[4];
		if( !$user || !$id || !$chan )
		{
			return "Usage: twitter_configure <twitteruser> <listid> <channel> [tweetlimit] [interval]\n"
			     . "(interval is a factor of 5 seconds. i.e. a value of 4 means 20 seconds.)";
		}
		$self->set( "list_user", $user );
		$self->set( "list_id", $id );
		$self->set( "tweet_channel", $chan );
		if( $lim )
		{
			$self->set( "tweet_limit", $lim );
		}
		if( $intv )
		{
			$self->set( "interval", $intv );
		}
		return "OK, configured Twitter for you. Tweets will start dripping in automatically.";
	}
}

sub updateTwitter {
	my ($self) = shift;
	my $last_id = $self->get("last_id") || 1;

	my $list_user = $self->get("list_user") || return;
	my $list_id   = $self->get("list_id") || return;
	my $tweet_limit = $self->get("tweet_limit") || 5;
	my $tweet_channel = $self->get("tweet_channel") || return;

	eval {
		my $statuses = $net_twitter->list_statuses({
			user => $list_user,
			list_id => $list_id,
			since_id => $last_id,
			per_page => $tweet_limit,
		});
		my @statuses = reverse @$statuses;
		for my $status(@statuses) {
			$self->bot->say(
				channel => $tweet_channel,
				body    => "-Twitter- <" . $status->{user}{screen_name}
					   . "> " . decode_entities ($status->{text})
			);
			if( $last_id <= $status->{id} ) {
				$last_id = $status->{id};
			}
		}
	};
	if( $@ )
	{
		warn("Warning: Could not fetch Tweets: $@");
		return;
	}

	$self->set("last_id", $last_id);
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
                $command = lc($command);
                return ($command, $rest, @rest) if wantarray;
                return [$command, $rest, @rest];
        } else {
                return undef;
        }
}


1;
