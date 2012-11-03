package DaZeus;

use strict;
use warnings;
use JSON;
use POSIX qw(:errno_h);
use Storable qw(thaw freeze);
use MIME::Base64 qw(encode_base64 decode_base64);

=head1 NAME

DaZeus - Perl interface to the DaZeus 2 Socket API

=head1 SYNPOSIS

  use DaZeus;
  my $dazeus = DaZeus->connect("unix:/tmp/dazeus.sock");
  # or:
  my $dazeus = DaZeus->connect("tcp:localhost:1234");
  
  # Get connection status
  my $networks = $dazeus->networks();
  foreach (@$networks) {
    my $channels = $dazeus->channels($n);
    print "$_:\n";
    foreach (@$channels) {
      print "  $_\n";
    }
  }
  
  $dazeus->subscribe("JOINED", sub { warn "JOINED event received!" });
  
  $dazeus->subscribe(qw/PRIVMSG NOTICE/);
  while(my $event = $dazeus->handleEvent()) {
    next if($event->{'event'} eq "JOINED");
    my ($network, $sender, $channel, $message)
      = @{$event->{'params'}};
    print "[$network $channel] <$sender> $message\n";
    my $destination = $channel eq "msg" ? $sender : $channel;
    $dazeus->message($network, $destination, $message);
  }

=head1 DESCRIPTION

This module provides a Perl interface to the DaZeus 2 Socket API, so a Perl
application can act as a DaZeus 2 plugin. The module supports receiving events
and sending messages. See also the "examples" directory that comes with the
Perl bindings.

=head1 METHODS

=cut

my ($HAS_INET, $HAS_UNIX);
BEGIN {
	$HAS_INET = eval 'use IO::Socket::INET; 1';
	$HAS_UNIX = eval 'use IO::Socket::UNIX; 1';
};

our $VERSION = '1.00';

=head2 connect($socket)

Creates a DaZeus object connected to the given socket. Returns the object if
the initial connection succeeded; otherwise, calls die(). If, after this
initial connection, the connection fails, for example because the bot is
restarted, the module will re-connect.

The given socket name must either start with "tcp:" or "unix:"; in the case of
a UNIX socket it must contain a path to the UNIX socket, in the case of TCP it
may end in a port number. IPv6 addresses must have a port number attached, so
it can be distinguished from the rest of the address; in "tcp:2001:db8::1:1234"
1234 is the port number.

If you give an object through $socket, the module will attempt to use it
directly as the target socket.

=cut

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

=head2 socket()

Returns the internal UNIX or TCP socket used for communication. This call is
useful if you want to watch multiple sockets, for example, using the select()
call. Every time the socket can be read, you can call the handleEvents() method
to process any incoming events. Do not call read() or write() on this socket,
or other calls that change internal socket state.

If the given DaZeus object was not connected, a new connection is opened and
a valid socket will still be returned.

=cut

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

=head2 networks()

Returns a list of active networks on this DaZeus instance, or calls
die() if communication failed.

=cut

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

=head2 channels($network)

Returns a list of joined channels on the given network, or calls
die() if communication failed.

=cut

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

=head2 message($network, $channel, $message)

Sends given message to given channel on given network, or calls die()
if communication failed.

=cut

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

=head2 action($network, $channel, $message)

Like message(), but sends the message as a CTCP ACTION (as if "/me" was used).

=cut

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

=head2 sendNames($network, $channel)

Requests a NAMES command being sent for the given channel on the given network.
After this, a NAMES event will be produced using the normal event system
described below, if the IRC server behaves correctly. Calls die() if
communication failed.

=cut

sub sendNames {
	my ($self, $network, $channel) = @_;
	$self->_send({do => "names", params => [$network, $channel]});
	my $response = $self->_read();
	if($response->{success}) {
		return 1;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

=head2 sendWhois($network, $nick)

Requests a WHOIS command being sent for the given nick on the given network.
After this, a WHOIS event will be produced using the normal event system
described below, if the IRC server behaves correctly. Calls die() if
communication failed.

=cut

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

=head2 join($network, $channel)

Requests a JOIN command being sent for the given channel on the given network.
After this, a JOIN event will be produced using the normal event system
described below, if the IRC server behaves correctly and the channel was not
already joined. Calls die() if communication failed.

=cut

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

=head2 part($network, $channel)

Requests a PART command being sent for the given channel on the given network.
After this, a PART event will be produced using the normal event system
described below, if the IRC server behaves correctly and the channel was
joined. Calls die() if communication failed.

=cut

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

=head2 getNick($network)

Requests the current nickname on given network, and returns it. Calls die()
if communication failed.

=cut

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

sub _addScope {
	my ($network, $receiver, $sender) = @_;
	return () if(!$network);
	my $scope = [$network];
	if($receiver) {
		push @$scope, $receiver;
		push @$scope, $sender if $sender;
	}
	return scope => \@$scope;
}

=head2 getConfig($name)

Retrieves the given variable from the configuration file and returns
its value. Calls die() if communication failed.

=cut

sub getConfig {
	my ($self, $name) = @_;
	$self->_send({get => "config", params => [$name]});
	my $response = $self->_read();
	if($response->{success}) {
		return $response->{value};
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

=head2 getProperty($name, [$network, [$receiver, [$sender]]])

Retrieves the given variable from the persistent database and returns
its value. Optionally, context can be given for this property request,
so properties stored earlier using a specific scope can be correctly
matched against this request, and the most specific match will be
returned. Calls die() if communication failed.

=cut

sub getProperty {
	my ($self, $name, @scope) = @_;
	$self->_send({do => "property", params => ["get", $name], _addScope(@scope)});
	my $response = $self->_read();
	if($response->{success}) {
		my $value = $response->{'value'};
		$value = eval { thaw(decode_base64($value)) } || $value if defined $value;
		return $value;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

=head2 setProperty($name, $value, [$network, [$receiver, [$sender]]])

Stores the given variable to the persistent database. Optionally, context can
be given for this property, so multiple properties with the same name and
possibly overlapping context can be stored and later returned. This is useful
in situations where you want different settings per network, but also
overriding settings per channel. Calls die() if communication failed.

=cut

sub setProperty {
	my ($self, $name, $value, @scope) = @_;
	$value = encode_base64(freeze($value)) if ref($value);
	$self->_send({do => "property", params => ["set", $name, $value], _addScope(@scope)});
	my $response = $self->_read();
	if($response->{success}) {
		return 1;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

=head2 unsetProperty($name, [$network, [$receiver, [$sender]]])

Unsets the given variable with given context from the persistent database. If
no variable was found with the exact given context, no variables are removed.
Calls die() if communication failed.

=cut

sub unsetProperty {
	my ($self, $name, @scope) = @_;
	$self->_send({do => "property", params => ["unset", $name], _addScope(@scope)});
	my $response = $self->_read();
	if($response->{success}) {
		return 1;
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

=head2 getPropertyKeys($name, [$network, [$receiver, [$sender]]])

Retrieves all keys in a given namespace. I.e. if example.foo and example.bar
were stored earlier, and getPropertyKeys("example") is called, "foo" and "bar"
will be returned from this method. Calls die() if communication failed.

=cut

sub getPropertyKeys {
	my ($self, $name, @scope) = @_;
	$self->_send({do => "property", params => ["keys", $name], _addScope(@scope)});
	my $response = $self->_read();
	if($response->{success}) {
		return $response->{keys};
	} else {
		$response->{error} ||= "Request failed, no error";
		die $response->{error};
	}
}

=head2 subscribe($event, [$event, [$event, ..., [$coderef]]])

Subscribes to the given events. If the last parameter is a code reference, it
will be called automatically every time one of the given events hits, with the
DaZeus object as the first parameter, and the received event as the second.

=cut

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

=head2 unsubscribe($event, [$event, [$event, ...]])

Unsubscribe from the given events. They will no longer be received, until
subscribe() is called again.

=cut

sub unsubscribe {
	my ($self, @events) = @_;
	$self->_send({do => "unsubscribe", params => \@events});
	my $response = $self->_read();
	return $response->{removed};
}

=head2 handleEvent($timeout)

Returns all events that can be returned as soon as possible, but no longer
than the given $timeout. If $timeout is zero, do not block. If timeout is
undefined, wait forever until the first event arrives.

"As soon as possible" means that if there are cached events already read from
the socket earlier, the socket will not be touched. Otherwise, if events are
available for reading, they will be immediately read. Only if no events were
cached, none were available for reading, and $timeout is not zero will this
function block to retrieve events. (See also handleEvents().)

This method only returns a single event, in order of receiving. For every
returned event, the event handler (if given to subscribe()), is called.

=cut

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
		# On OS X, if the other end closes the socket, recv() still returns success
		# values and sets no error; however, connected() will be undef there
		if(!defined($self->{sock}->connected())) {
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

=head2 handleEvents()

Handle and return as much events as possible without blocking. Also calls the
event handler for every returned event.

=cut

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
