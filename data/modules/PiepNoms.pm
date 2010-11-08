# DaZeus - A highly functional IRC bot
# Copyright (C) 2007  Sjors Gielen <sjorsgielen@gmail.com>
#
## Refter module
# Copyright (C) 2010  Gerdriaan Mulder <har@mrngm.com>
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
package DaZeus2Module::PiepNoms;
#use strict;
use warnings;
no warnings 'redefine';
use base qw(DaZeus2Module);
use MIME::Base64;
use XML::DOM::XPath;
use LWP::Simple;
use Encode;

my @week = qw(Zo Ma Di Wo Do Vr Za Zo);
my @menus = ("Basis 1", "Basis 2", "Vega");

sub nomget {
	my $compare = "";
	my @dag = @_;
	my ($second, $minute, $hour, $dayOfMonth, $month, $yearOffset, $dayOfWeek, $dayOfYear, $daylightSavings) = localtime();
	
	my $menulink = "http://www.ru.nl/facilitairbedrijf/eten_en_drinken/weekmenu_de_refter/menu-week/?rss=true";
	my $menucontent = get($menulink);
	my $tree = XML::DOM::Parser->new();
	my $doc = $tree->parse($menucontent);
    my @items = $doc->findnodes('//item');
	my @noms;
    foreach (@items) {
        my $title = $_->getElementsByTagName('title')->item(0)->getFirstChild()->getNodeValue();
        my $url = $_->getElementsByTagName('link')->item(0)->getFirstChild()->getNodeValue();
        my $desc = $_->getElementsByTagName('description')->item(0)->getFirstChild()->getNodeValue();
        $desc =~ s/\<p\>//;
        $desc =~ s/\<\/p\>//;
        if ($dag[1]) {
            $compare = lc(substr($dag[1], 0, 2));
            if ($compare eq "mo") {
                $compare = lc(substr($week[$dayOfWeek+1], 0, 2));
            } elsif ($compare eq "ov") {
                $compare = lc(substr($week[$dayOfWeek+2], 0, 2));
            }
        }

        if ($compare eq "") {
            $compare =  lc($week[$dayOfWeek]); 
        }
        if ($compare eq lc(substr($title, 0, 2))) {
            push(@noms, $desc);
        }
        $compare = "";
	}
	return @noms;
}

sub told
{
	my %dagen = ("ma", "maandag",
		"di", "dinsdag",
		"wo", "woensdag",
		"do", "donderdag",
		"vr", "vrijdag",
		"za", "zaterdag",
		"zo", "zondag",
		"mo", "morgen",
		"ov", "overmorgen");

	my @dagkeys = keys %dagen;
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	my $who = $mess->{who};
	return if !defined $command;
	if($command eq "noms") {
		my ($second, $minute, $hour, $dayOfMonth, $month, $yearOffset, $dayOfWeek, $dayOfYear, $daylightSavings) = localtime();

		my @noms;
		if ($rest[0]) {
			my $dag = lc(substr($rest[0], 0, 2));
			if (grep $_ eq $dag, @dagkeys) {
				@noms = $self->nomget($dag);
				$self->bot->reply($mess, "Eten bij de Refter op ".$dagen{$dag});
			}
		} else {
			if ($hour >= 19) {
				my $morgen = lc $week[$dayOfWeek+1];
				$self->bot->reply($mess, "Na 19 uur zijn er geen refternoms meer. Daarom krijg je de refternoms van morgen (".$dagen{$morgen}.") te zien ;-)");
				@noms = $self->nomget($morgen);
			} else {
				@noms = $self->nomget();
				$self->bot->reply($mess, "Wat heeft de Refter vandaag in de aanbieding...");
			}
		}
		my $i = 0;
		foreach(@noms) {
			# Newlines weghalen, want die verneuken de output
			s/[\r\n]+/ /g;
			$self->bot->reply($mess, $menus[$i++].": ".$_);
		}
		if( @noms == 0 ) {
			$self->bot->reply($mess, "Helaas, er is niks :'-(");
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
