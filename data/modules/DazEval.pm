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
package DaZeus2Module::DazEval;
use strict;
use warnings;
no warnings 'redefine';
use base qw(DaZeus2Module);
use Data::Dumper;
use Math::Complex; # tan, atan, etc.
use PerlIO;
use PerlIO::scalar;
use Scalar::Util;
use List::Util;
use Tie::Hash;
use Tie::Scalar;
use Tie::Array;
use BSD::Resource;
use Symbol qw/delete_package/;
use POSIX;
use Carp::Heavy;
use POE qw(Wheel::Run Filter::Stream);

# Based very heavily on Buubot's (?:\w+)eval.pm; buu wrote most of this code.

sub init {
	my ($self) = @_;
	POE::Session->create(
		inline_states => {
			_start => \&Start,
			_stop  => \&Stop,
			output => \&Output,
			close  => \&Close,
			shutdown => \&Shutdown,
			request => \&Request,
			tick   => \&Tick,
		},
	);
}

sub told
{
	my ($self, $mess) = @_;
	my $p = $mess->{perms};
	my ($command, $rest, @rest) = $self->parseMsg($mess);
	my $who = $mess->{who};
	return if !defined $command;
	if($command eq "eval") {
		return "You don't have dazeus.commands.eval.perl permissions."
			if(!$p->has("dazeus.commands.eval.perl"));
		my $code = $rest;
		warn("EVAL by $who in $mess->{channel}: $code\n");
		# arguments to evaler: code, channel, target
		# ( target = username who said )
		POE::Kernel->post("evaler" => "request" =>
			["perl", $code, $mess->{channel}, $who],
			sub { $self->bot->say( channel => $mess->{channel}, body    => $_[0], who => $who ) },
		);
	} elsif($command eq "stopeval") {
		return "You don't have dazeus.commands.stopeval permissions."
			if(!$p->has("dazeus.commands.stopeval"));
		POE::Kernel->post("evaler" => "shutdown");
		return "Ok.";
	}
}

sub Start {
	if( POE::Kernel->alias_set("evaler") != 0 )
  {
    warn "There is already an evaler session (or another error occured).\n";
  }
  warn "evaler Started. --\n";
}

sub Stop {
	warn "evaler Stopped. --\n";
}

sub Output {
	my ($self, $body, $wheelid) = @_[HEAP, ARG0, ARG1];
	
	$self->{ wheels }->{$wheelid}->{output} .= $body;
}

sub Close {
	my ($self, $wheel_id) = @_[HEAP, ARG0];
	my $wheel = delete $self->{ wheels }{ $wheel_id };
	return unless $wheel;
	
	my $output = $wheel->{output};
	$output =~ s/\r?\n/ /g;
	$output =~ s/[^\x20-\x7f]/ /g;
	
	if( ! ref $wheel->{callback} ) { warn "Error: No callback, aborting."; return; }
	$wheel->{callback}->(substr( $output, 0, 200) );
}

sub Tick {
	my ($self) = $_[HEAP];
	for(keys %{ $self->{ wheels } }) {
		my $wheel = $self->{ wheels }{$_};
		if(time - $wheel->{start} > 9) {
			print "Killing wheel $_.\n";
			$wheel->{wheel}->kill(9);
			$wheel->{callback}->("$_ killed because of timeout.");
			delete $self->{wheels}->{$_};
		}
	}
	my $time = keys %{$self->{wheels}};
	if($time > 0) {
		$_[KERNEL]->delay_set(tick => 10);
	}
}

sub Shutdown {
	POE::Kernel->alias_remove("evaler");
}

sub Request {
	my ($self, $args, $return) = @_[HEAP, ARG0, ARG1];

	# safe_execute should be run, and then it should run evaler.
	my $wheel = POE::Wheel::Run->new(
		Program      => sub { safe_execute(@$args) },
		StdoutFilter => POE::Filter::Stream->new,
		StderrFilter => POE::Filter::Stream->new,
		StdoutEvent  => "output",
		StderrEvent  => "output",
		CloseEvent   => "close",
	);
	POE::Kernel->sig( CHLD => "close" );
	
	$self->{ wheels }->{ $wheel->ID } = {
		wheel => $wheel,
		start => time,
		callback => $return,
		output => "",
	};

	POE::Kernel->delay_set (tick => 10);

	return $wheel;
}

sub safe_execute {
	my ($language, $code, $channel, $target) = @_;
	# This should be automatically done in the start
	#chdir("/var/dazeus") or die $!;
	if( $< == 0) {
		chroot(".") or die $!;
	}
	$<=$>=65534;
	POSIX::setgid(65534);
	
	setrlimit(RLIMIT_CPU, 10, 10);
	setrlimit(RLIMIT_DATA, 15*1024, 15*1024);
	setrlimit(RLIMIT_STACK, 15*1024, 15*1024);
	setrlimit(RLIMIT_NPROC, 1, 1);
	setrlimit(RLIMIT_NOFILE, 0, 0);
	setrlimit(RLIMIT_OFILE, 0, 0);
	setrlimit(RLIMIT_OPEN_MAX, 0, 0);
	setrlimit(RLIMIT_LOCKS, 0, 0);
	setrlimit(RLIMIT_AS, 15*1024, 15*1024);
	setrlimit(RLIMIT_VMEM, 15*1024, 15*1024);
	setrlimit(RLIMIT_MEMLOCK, 100, 100);
	die "Failed to drop root: $<" if $< == 0;
	close STDIN;
	local $@;
	for( qw/Socket IO::Socket::INET/ ) {
		delete_package( $_ );
	}
	local @INC;

	{
		if($language eq "perl") {
			evaler($code, $channel, $target);
		} else {
			print "Unknown language.";
		}
	}
}

sub pretty {
	my $ch = shift;
	return '\n' if $ch eq "\n";
	return '\t' if $ch eq "\t";
	return '\0' if ord $ch == 0;
	return sprintf '\x%02x', ord $ch if ord $ch < 256;
	return sprintf '\x{%x}', ord $ch;
}

sub evaler {
	# THIS SUB MUST ALWAYS BE RUN IN A DIFFERENT PROCESS!
	# We shift, or people might get them from @_ (not that it matters)
	my ($code, $channel, $target) = (shift, shift, shift);
	$| = 1;
	$! = undef;
	local %ENV=();
	srand;
	my @OS = ('Microsoft Windows', 'Linux', 'NetBSD', 'FreeBSD', 'Solaris', 'OS/2 Warp', 'OSX');
	local $^O = $OS[rand@OS];
	if($channel ne 'msg' and defined $target and $target ne "") {
		print "$target:\n";
	}
	
	my $deny_code = "";
	# Disallow calls that may change the filesystem, shared memory
	# or other processes, or that give away sensitive information.
	for( "msgctl", "msgget", "msgrcv", "msgsnd", "semctl", "semget",
		"semop", "shmctl", "shmget", "shmread", "shmwrite", "unlink",
		"chmod", "chown", "opendir", "link", "mkdir", "stat",
		"rename", "rmdir", "stat", "syscall", "truncate" )
	{
		$deny_code .= "*CORE::GLOBAL::$_ = sub {die}; \n";
	}
	
	my $ret;
	{
		local $\=" ";
		$ret = eval (
			"no strict; no warnings; package main;
			BEGIN{ $deny_code }\n#line 1 eval
			$code"
		);
	}
	no warnings;
	$ret =~ s/\s+$//;
	if( $@ ) {
		print "Error: $@\n";
	} else {
		print " ";
		if( ref $ret ) {
			local $Data::Dumper::Terse = 1;
			local $Data::Dumper::Quotekeys = 0;
			local $Data::Dumper::Indent = 0;
			$ret = Dumper( $ret );
			print $ret;
		} elsif($ret =~ /[^\x20-\x7e]/) {
			$ret =~ s/\\/\\\\/g;
			$ret =~ s/"/\"/g;
			$ret =~ s/([^\x20-\x73])/pretty($1)/eg;
			print qq{$ret};
		} else {
			print $ret;
		}
		print "\n";
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
