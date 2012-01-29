package DaZeus;

use strict;
use warnings;
use JSON;
use POSIX qw(:errno_h);
use Storable qw(thaw freeze);
use MIME::Base64 qw(encode_base64 decode_base64);

my ($HAS_INET, $HAS_UNIX);
BEGIN {
	$HAS_INET = eval 'use IO::Socket::INET; 1';
	$HAS_UNIX = eval 'use IO::Socket::UNIX; 1';
};

our $VERSION = '1.00';

sub connect {
	my ($pkg, $socket) = @_;

	my $self = {
		handlers => {},
		events => []
	};
	bless $self, $pkg;

	if(ref($socket)) {
		$self->{sock} = $socket;
	} else {
		$self->{socketfile} = $socket;
	}

	return $self->_connect();
}

sub socket {
	my ($self) = @_;
	$self->_connect();
	return $self->{sock};
}

sub _connect {
	my ($self) = @_;
	if($self->{sock}) {
		return $self;
	}

	if($self->{socketfile} =~ /^tcp:(.+):(\d+)$/ || $self->{socketfile} =~ /^tcp:(.+)$/) {
		if(!$HAS_INET) {
			die "TCP connection requested, but IO::Socket::INET couldn't be loaded";
		}
		my $host = $1;
		my $port = $2;
		$self->{sock} = IO::Socket::INET->new(
			PeerAddr => $host,
			PeerPort => $port,
			Proto    => 'tcp',
			Type     => SOCK_STREAM,
			Blocking => 0,
		) or die $!;
	} elsif($self->{socketfile} =~ /^unix:(.+)$/) {
		if(!$HAS_UNIX) {
			die "UNIX connection requested, but IO::Socket::UNIX couldn't be loaded";
		}
		my $file = $1;
		$self->{sock} = IO::Socket::UNIX->new(
			Peer => $file,
			Type => SOCK_STREAM,
			Blocking => 0,
		) or die "Error opening UNIX socket $file: $!\n";
	} else {
		die "Didn't understand format of socketfile: " . $self->{socketfile} . " -- does it begin with unix: or tcp:?\n";
	}

	return $self;
}

sub networks {
	my ($self) = @_;
	$self->_send({get => "networks"});
	my $response = $self->_read();
	if($response->{success}) {
		return $response->{networks};
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub channels {
	my ($self, $network) = @_;
	$self->_send({get => "channels", params => [$network]});
	my $response = $self->_read();
	if($response->{success}) {
		return $response->{channels};
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub message {
	my ($self, $network, $channel, $message) = @_;
	$self->_send({do => "message", params => [$network, $channel, $message]});
	my $response = $self->_read();
	if($response->{success}) {
		return 1;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub action {
	my ($self, $network, $channel, $message) = @_;
	$self->_send({do => "action", params => [$network, $channel, $message]});
	my $response = $self->_read();
	if($response->{success}) {
		return 1;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub sendWhois {
	my ($self, $network, $nick) = @_;
	$self->_send({do => "whois", params => [$network, $nick]});
	my $response = $self->_read();
	if($response->{success}) {
		return 1;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub join {
	my ($self, $network, $channel) = @_;
	$self->_send({do => "join", params => [$network, $channel]});
	my $response = $self->_read();
	if($response->{success}) {
		return 1;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub part {
	my ($self, $network, $channel) = @_;
	$self->_send({do => "part", params => [$network, $channel]});
	my $response = $self->_read();
	if($response->{success}) {
		return 1;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub getNick {
	my ($self, $network) = @_;
	$self->_send({get => "nick", params => [$network]});
	my $response = $self->_read();
	if($response->{success}) {
		return $response->{'nick'};
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub getProperty {
	my ($self, $name) = @_;
	$self->_send({do => "property", params => ["get", $name]});
	my $response = $self->_read();
	if($response->{success}) {
		my $value = $response->{'value'};
		$value = eval { thaw(decode_base64($value)) } || $value;
		return $value;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub setProperty {
	my ($self, $name, $value) = @_;
	$value = encode_base64(freeze($value)) if ref($value);
	$self->_send({do => "property", params => ["set", $name, "global", $value]});
	my $response = $self->_read();
	if($response->{success}) {
		return 1;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub unsetProperty {
	my ($self, $name) = @_;
	$self->_send({do => "property", params => ["unset", $name]});
	my $response = $self->_read();
	if($response->{success}) {
		return 1;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub getPropertyKeys {
	my ($self, $name, $value) = @_;
	$self->_send({do => "property", params => ["keys", $name, $value]});
	my $response = $self->_read();
	if($response->{success}) {
		return $response->{keys};
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

sub subscribe {
	my ($self, @events) = @_;
	if(ref($events[$#events]) eq "CODE") {
		my $handler = pop(@events);
		foreach(@events) {
			$self->{handlers}{$_} = $handler;
		}
	}

	$self->_send({do => "subscribe", params => \@events});
	my $response = $self->_read();
	return $response->{added};
}

sub unsubscribe {
	my ($self, @events) = @_;
	$self->_send({do => "unsubscribe", params => \@events});
	my $response = $self->_read();
	return $response->{removed};
}

sub handleEvent {
	my ($self, $timeout) = @_;
	if(@{$self->{events}} == 0) {
		my $packet = $self->_readPacket($timeout);
		if(!$packet) {
			return;
		}
		if(!$packet->{event}) {
			die "Error: Out of bound non-event packet received in handleEvent";
		}
		push @{$self->{events}}, $packet;
	}
	my $event = shift @{$self->{events}};
	my $handler = $self->{handlers}{$event->{event}};
	if($handler) {
		$handler->($self, $event);
	}
	return $event;
}

sub _readPacket {
	use bytes;
	my ($self, $timeout) = @_;
	my $once = $timeout == 0 if(defined $timeout);
	my $stop_time = time() + $timeout if(defined $timeout);
	my $event_part;
	my $last_event_part;
	my $size = 3;
	# A size of 20 bytes is too large, something must be happening
	while($size < 20 && ($once || !defined($stop_time) || time() < $stop_time)) {
		undef $event_part;
		if(!defined($self->{sock}->recv($event_part, $size, MSG_PEEK))) {
			if($! == EAGAIN) {
				# Nothing to read, that's ok
				if(defined $timeout && $timeout == 0) {
					return;
				}
				# TODO: how to set a timeout parameter on sockets after creation?
				next;
			}
			# Error in peek
			delete $self->{sock};
			die $!;
		}
		# TODO: change this to "if there are no more bytes available", more accurate
		if($last_event_part && $last_event_part eq $event_part) {
			$once = 0;
		}
		$last_event_part = $event_part;
		if($event_part =~ /^[\r\n]+$/) {
			# This seems to be necessary on my Linux machine; it refuses
			# to read further than these newline bytes...
			$self->{sock}->recv($event_part, length $event_part);
		}
		# Has enough data been read?
		if(length($event_part) == $size && index($event_part, '{') > 0) {
			# Start of JSON data found
			my $pre_json_size = index($event_part, '{');
			my $json_size = substr($event_part, 0, $pre_json_size);
			# Ignore all non-numerals:
			$json_size =~ s/[^0-9]//g;

			# See if we can pull all the data from the socket now
			if(!defined($self->{sock}->recv($event_part, $pre_json_size + $json_size, MSG_PEEK))) {
				# Error in peek
				delete $self->{sock};
				die $!;
			}
			if(length($event_part) != ($pre_json_size + $json_size)) {
				# Try again later
				next;
			}
			# That worked, so now we can actually take it off the socket
			if(!defined($self->{sock}->recv($event_part, $pre_json_size + $json_size))) {
				# Error in read
				delete $self->{sock};
				die $!;
			}
			my $json = decode_json(substr($event_part, $pre_json_size));
			return $json;
		} elsif(length($event_part) == $size) {
			# We read something, but it's not in this response yet
			$size += 4;
		}
	}
	return;
}

sub handleEvents {
	my ($self) = @_;
	my @events;
	while(my $event = $self->handleEvent(0)) {
		push @events, $event;
	}
	return @events;
}

# Read incoming JSON requests until a non-event comes in (blocking)
sub _read {
	my ($self) = @_;
	while(1) {
		my $packet = $self->_readPacket();
		if($packet->{'event'}) {
			push @{$self->{events}}, $packet;
		} else {
			return $packet;
		}
	}
}

sub _send {
	my ($self, $msg) = @_;
	my $json = encode_json($msg);
	$self->{sock}->write(bytes::length($json) . $json . "\r\n");
}

1;

__END__

=head1 NAME

DaZeus - Perl bindings for DaZeus IRC bot communication

=head1 SYNOPSIS

  use DaZeus;
  my $d = DaZeus->connect("unix:/tmp/dazeus.sock");
  # or:
  my $d = DaZeus->connect("tcp:localhost:1234");
  
  # Get connection status
  my $networks = $d->networks();
  foreach (@$networks) {
    my $channels = $d->channels($n);
    print "$_:\n";
    foreach (@$channels) {
      print "  $_\n";
    }
  }
  
  # Subscribe to events
  $d->subscribe("JOINED", sub { warn "JOINED event received!" });
  while(1) {
    $d->handleEvent() or last;
  }

=head1 DESCRIPTION

This module is the Perl version of the DaZeus library bindings. They allow a
Perl application to act as a DaZeus 2 plugin.

=head1 METHODS

=head2 connect($socket)

Creates a DaZeus object connected to the given socket. Returns the object if
the initial connection succeeded; undef otherwise. If, after this initial
connection, the connection fails, for example because the bot is restarted, the
module will re-connect.

The given socket name must either start with "tcp:" or "unix:"; in the case of
a UNIX socket it must contain a path to the UNIX socket, in the case of TCP it
may end in a port number. IPv6 addresses must have a port number attached, so
it can be distinguished from the rest of the address; in "tcp:2001:db8::1:1234"
1234 is the port number.

If you give an object through $socket, the module will attempt to use it
directly as the target socket.

=head2 socket()

Returns the socket internally used for communication. It should act as an
IO::Socket. You can use this method in select() loops, but make sure not to
read or write from it.

=head2 networks()

Returns a list of all networks known in this DaZeus instance.

=head2 channels($network)

Given a network name, returns a list of all channels on this network. If the
bot does not know about the network, this method calls die().

=head2 message($network, $channel, $message)

Says $message on given channel and network. If the channel is not joined on the
network, this method calls die().

=head2 action($network, $channel, $message)

Send a CTCP ACTION (/me) on a given channel and network.

=head2 sendWhois($network, $nick)

Send a WHOIS for the given nick to the network. The reply will come back via
numeric events; this method returns 1 on success.

=head2 join($network, $channel)

Joins given channel on given network.

=head2 part($network, $channel)

Leaves given channel on given network.

=head2 getNick($network)

Returns our own nickname on given network.

=head2 setProperty($name, $value)

Sets the given property $name to $value. (TODO: explain this)

=head2 getProperty($name)

Returns the value of property $name.

=head2 unsetProperty($name)

Unsets the given property $name.

=head2 getPropertyKeys($name)

Returns all property keys beginning with '$name'.

=head2 subscribe(@events, [$handler])

Subscribes to the given events. Call handleEvent() to handle the next incoming
event. If the last parameter is a code reference, it is called for every event
in @events.

=head2 unsubscribe(@events)

Unsubscribes from the given events. Events that already occured before the
unsubscription, but never handled, may still come in.

=head2 handleEvent($timeout)

Return one event, or return undef if no event comes in within the given timeout
parameter (in seconds). If an event was received during the last command, it
will be handled by this method immediately.

If a handler is defined for the incoming event, the handler is called before
this method returns.

=head2 handleEvents()

Handle all events that can be immediately handled, by calling handleEvent until
it returns undef, then returning a list of all events it returned.

=head1 AUTHOR

Sjors Gielen, E<lt>dazeus@sjorsgielen.nlE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012 by Sjors Gielen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=cut
