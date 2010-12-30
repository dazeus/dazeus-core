use DaZeus2;
use Data::Dumper;
use strict;
use warnings;
use POE;

my @modules;
my $uniqueid;
my $dummy_session;

sub poe_queue_count {
  return $poe_kernel->[POE::Kernel::KR_QUEUE]->get_item_count();
}

sub run_timeslice {
  # This method should be called every once in a while to bump events in
  # POE sessions.
  my $counter = 0;
  if($counter < 6 && get_poe_queue_count() > 0)
  {
    $poe_kernel->run_one_timeslice();
    $counter++;
  }
}

sub init {
  ($uniqueid) = @_;

  # Here, we start a dummy POE session. It does nothing at all, but because
  # the reference counter is increased during start, it will never be
  # destroyed. Because this session will never be destroyed, there will always
  # be an active session, keeping POE from destroying other sessions thinking
  # the application is in deadlock.
  $dummy_session = POE::Session->create( inline_states =>
    { _start => sub { $_[KERNEL]->refcount_increment( $_[SESSION]->ID,
                        "keep_me_alive" ) }});
  run_timeslice();
}

sub whois {
  my ($nick, $is_identified) = @_;
  my $whois = {
    nick => $nick,
    identified => $is_identified,
  };

  for my $mod (@modules)
  {
    eval { $mod->whois($whois); };
    if( $@ )
    {
      warn("Error executing $mod ->whois(): $@\n" );
      next;
    }
  }
  run_timeslice();
}

sub message {
  my ($sender, $receiver, $body, $raw_body) = @_;
  my $mess = {
    channel => ($receiver =~ /^(#|&)/) ? $receiver : "msg",
    body    => $body,
    raw_body => $raw_body,
    who     => $sender,
  };

  __said($mess);
  run_timeslice();
}

sub __said {
  my ($mess) = @_;

  for my $pri (0..3)
  {
    dispatch( "said",
      sub {
        my ($error, $mod, $args) = @_;
        my $mess = $args->[0];
        $mod->reply($mess, "Error executing $mod ->said(): $error\n");
      },
      sub {
        my ($message, $mod, $args) = @_;
        my $mess    = $args->[0];
        if( $message && $message ne "1" && !ref($message) )
        {
          $mod->reply($mess, $message);
          return -1;
        }
      }, $mess, $pri );
  }

  return;
}

sub dispatch {
  my $method = shift;
  my $error_callback   = shift || sub {};
  my $message_callback = shift || sub {};

  for my $mod (@modules)
  {
    my $message;
    eval { $message = $mod->$method( @_ ); };
    if( $@ )
    {
      warn("Error executing $mod -> $method: $@\n" );
      my $result = $error_callback->($@, $mod, \@_);
      return if( $result && $result eq "-1" );
      next;
    }
    my $result = $message_callback->($message, $mod, \@_);
    return if( $result && $result eq "-1" );
  }
}

sub getModule {
  foreach(@modules)
  {
    if( $_->{Name} eq $_[0])
    {
      return $_;
    }
  }
  return undef;
}

sub join {
  my ($channel, $who) = @_;
  dispatch( "chanjoin", 0, 0, {
      who => $who,
      channel => $channel,
  });
  run_timeslice();
}

sub nick {
  my ($who, $new_nick) = @_;
  dispatch( "nick_change", 0, 0, $who, $new_nick );
  run_timeslice();
}

sub connected {
  dispatch( "connected" );
  run_timeslice();
}

sub namesReceived {
  my ($channel, $names ) = @_;
  my %names;
  foreach( split /\s+/, $names )
  {
    my $op = /^\+?@/;
    my $voice = /^@?\+/;
    s/^[\+@]+//;
    $names{$_} = {op => $op, voice => $voice};
  }

  dispatch( "got_names", 0, 0, {
    channel => $channel,
    names   => \%names,
  } );
  run_timeslice();
}

my $tick_counter = 0;
sub tick {
  # 'Tick' should be dispatched about once every 5 seconds.
  # Timeslice runs every second.
  if( ++$tick_counter == 5 ) {
    dispatch( "tick" );
    $tick_counter = 0;
  }
  run_timeslice();
}

sub reloadModule {
  my $oldmod = getModule($_[0]);
  unloadModule($_[0]) if($oldmod);
  loadModule($_[0]);
  my $newmod = getModule($_[0]);
  $newmod->reload($oldmod);
  return $newmod;
}

sub unloadModule {
  my $to_remove = $_[0];
  my $module = getModule($to_remove);
  return 1 if(!$module);
  @modules = grep { $_->{Name} ne $to_remove } @modules;
}

sub loadModule {
  my ($module) = @_;

  return 1 if getModule($module);

  my $file = "./modules/$module.pm";

  if( ! -e $file )
  {
    warn "Could not find $file\n";
    return 0;
  }

  # force a reload of the file
  no warnings 'redefine';
  delete $INC{$file};
  require $file;

  $module = "DaZeus2Module::$module";

  push @modules, $module->new( UniqueID => $uniqueid );
  run_timeslice();
  return 1;
}

1;
