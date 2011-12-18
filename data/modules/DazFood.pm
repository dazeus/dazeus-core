# DaZeus - A highly functional IRC bot
# Copyright (C) 2007  Sjors Gielen <sjorsgielen@gmail.com>
# 
package DaZeus2Module::DazFood;
use strict;
use warnings;
no warnings 'redefine';
use base qw(DaZeus2Module);

my @foodlist;
my ($cook, $cookwhat, $cookedit);
my @months = qw(Jan Feb Mar Apr Mei Jun Jul Aug Sep Okt Nov Dec);
my @week = qw(Zo Ma Di Wo Do Vr Za Zo);

sub init
{
	my ($self) = @_;
	$self->load();
}

sub load
{
	my ($self) = @_;
	my $foodlist= $self->get("foodlist");
	$foodlist ||= [];
	@foodlist   = @$foodlist;
	$cook       = $self->get("cook");
	$cookwhat   = $self->get("cookwhat");
	$cookedit   = $self->get("cookedit");
}

sub save
{
	my ($self) = @_;
	my ($second, $minute, $hour, $dayOfMonth, $month, $yearOffset, $dayOfWeek, $dayOfYear, $daylightSavings) = localtime();
	my $year = 1900+$yearOffset;
	$self->set("foodlist", \@foodlist);
	$self->set("cook", $cook);
	$self->set("cookwhat", $cookwhat);
	$self->set("cookedit", "$week[$dayOfWeek] $dayOfMonth $months[$month] $year $hour:$minute:$second");
}

sub told
{
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	my $who = $mess->{who};
	return if !defined $command;
	if($command eq "food") {
		$who = $rest if($rest);
		if(in_foodlist($who)) {
			return "$who heeft al eten!";
		} else {
			push @foodlist, $who;
			$self->save();
			return "$who krijgt ook eten :)";
		}
	} elsif($command eq "nofood") {
		$who = $rest if($rest);
		if(in_foodlist($who)) {
			my @new_foodlist = ();
			foreach(@foodlist) {
				push @new_foodlist, $_ if(lc($_) ne lc($who));
			}
			@foodlist = @new_foodlist;
			$self->save();
			return "$who krijgt geen eten meer.";
		} else {
			return "$who kreeg al geen eten!";
		}
	} elsif($command eq "foodlist") {
		if(@foodlist == 0) {
			return "Nog niemand krijgt eten!";
		} else {
			$self->bot->reply($mess, "Deze mensen krijgen eten: ". join(", ", @foodlist). ".");
			if(defined($cook)) {
				$self->bot->reply($mess, "$cook kookt namelijk $cookwhat.");
			} else {
				$self->bot->reply($mess, "Er is alleen nog geen eten.");
			}
		}
		if(lc($rest) eq "reset") {
			@foodlist = ();
			$cook = undef;
			$cookwhat = undef;
			$self->save();
			return "Maar nu is alles gereset.";
		}
		$self->bot->reply($mess, "Laatste edit: ".$self->get("cookedit").".");
		return 1;
	} elsif($command eq "cook") {
		if(defined($cook)) {
			return "$cook kookt al $cookwhat!";
		} else {
			$cook = $who;
			$rest ||= "iets";
			$cookwhat = $rest;
			$self->save();
			return "Okee, $cook kookt nu $cookwhat.";
		}
	} elsif($command eq "nocook") {
		if(!defined($cook)) {
			return "Er werd nog niet gekookt.";
		} else {
			my $str = "Okee, $cook kookt niet meer $cookwhat.";
			$cook = undef;
			$cookwhat = undef;
			$self->save();
			return $str;
		}
	} elsif($command eq "let") {
		if(defined($cook)) {
			return "$cook kookt al $cookwhat!";
		}
		if($rest =~ /^(.+?) cook\s?(.*)$/i) {
			$cook = $1;
			$cookwhat = $2;
			$cookwhat ||= "iets";
			$self->save();
			return "OK, $cook kookt nu $cookwhat.";
		}
		return "Syntax: let IEMAND cook IETS";
	} elsif($command eq "foodhelp") {
		return "See http://paster.dazjorz.com/?p=3231 for DazFood help.";
	}
}

sub in_foodlist {
	my ($naam) = @_;
	foreach(@foodlist) {
		return 1 if(lc($_) eq lc($naam));
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
