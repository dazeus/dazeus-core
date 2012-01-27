use strict;
use warnings;

use Test::More tests => 12;
use IO::Socket::UNIX;
use File::Temp qw(tempdir);
use JSON;
BEGIN { use_ok('DaZeus') };

binmode(STDOUT, ':utf8');

my $dir = tempdir(CLEANUP => 1);
my $socketfile = $dir . "/test.sock";
my $socket = IO::Socket::UNIX->new(
	Type => SOCK_STREAM,
	Local => $socketfile,
	Listen => 1,
);

#### Fork to handle the responses
my $pid = fork();
if($pid == 0) {
	# I'm the child process, I'll handle the responses
	sleep 2;
	my $otherside = $socket->accept();
	my $child_dazeus = DaZeus->connect($otherside);
	my $foo_sub = 0;
	my $bar_sub = 0;
	eval {
	while(my $packet = $child_dazeus->_readPacket(5)) {
		my $what = $packet->{'get'} || $packet->{'do'};
		my @params = @{$packet->{'params'}} if($packet->{'params'});
		if($what eq "networks") {
			$child_dazeus->_send({got => "networks", "networks" => ["foo"], success => JSON::true});
		} elsif($what eq "channels") {
			if(@params == 1 && $params[0] eq "foo") {
				$child_dazeus->_send({got => "channels", "channels" => ["#bar"], network => "foo", success => JSON::true});
			} else {
				$child_dazeus->_send({got => "channels", success => JSON::false, error => "(this test is supposed to fail)"});
			}
		} elsif($what eq "message") {
			if(@params == 3 && $params[0] eq "quux" && $params[1] eq "#mumble" && $params[2] eq "¬Fø") {
				$child_dazeus->_send({did => "message", success => JSON::true});
			} else {
				die "Params to message incorrect: $params[0] $params[1] $params[2]\n";
			}
		} elsif($what eq "subscribe") {
			if(@params == 1 && $params[0] eq "FOO") {
				die "Double subscribe" if($foo_sub);
				$foo_sub = 1;
				$child_dazeus->_send({did => "subscribe", added => 1, success => JSON::true});
			} elsif(@params == 1 && $params[0] eq "BAR") {
				die "Double subscribe" if($bar_sub);
				$bar_sub = 1;
				$child_dazeus->_send({did => "subscribe", added => 1, success => JSON::true});
				sleep 2;
				$child_dazeus->_send({event => "FOO", params => ["p1", "p2"]});
				$child_dazeus->_send({event => "BAR", params => []});
			} else {
				die "Params to subscribe incorrect: $params[0]\n";
			}
		}
	}
	};
	exit;
}
####

my $dazeus;
eval {
	$dazeus = DaZeus->connect("unix:" . $socketfile);
};
if($@) {
	fail("connecting");
	BAIL_OUT("DaZeus module cannot connect to the socketfile.");
	exit;
}

pass("connecting");
ok($dazeus, "connect() returned true");
ok(ref($dazeus) eq "DaZeus", "got a DaZeus object");

is_deeply($dazeus->networks(), ["foo"], "got the usable networks");
is_deeply($dazeus->channels("foo"), ["#bar"], "got the usable channels");

ok($dazeus->message("quux", "#mumble", "¬Fø"), "sending a message to IRC");

eval {
	$dazeus->channels("bar");
};
ok($@ =~ /this test is supposed to fail/, "errors are thrown correctly");

my $eventhandled = 0;
$dazeus->subscribe("FOO", sub { $eventhandled = 1 });
$dazeus->subscribe("BAR");
is_deeply($dazeus->handleEvent(), {event => "FOO", params => ["p1", "p2"]}, "Event received correctly");
ok($eventhandled, "Event handler called correctly");
$eventhandled = 0;
is_deeply($dazeus->handleEvent(), {event => "BAR", params => []}, "Event 2 received correctly");
ok(!$eventhandled, "Event handled not called twice");

diag "Waiting for child to quit...\n";
close $dazeus->{sock};
waitpid($pid, 0);
