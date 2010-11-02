use DaZeus2;
use Data::Dumper;
use strict;
use warnings;

my @modules;
my $uniqueid;

sub init {
  ($uniqueid) = @_;
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
        my $message = $_[0];
        my $mess = $_[2][0];
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
      return if( $error_callback->($@, $mod, \@_) == -1 );
      next;
    }
    return if( $message_callback->($message) == -1 );
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
  print "** Join: $channel $who\n";
  dispatch( "chanjoin", 0, 0, {
      who => $who,
      channel => $channel,
  });
}

sub nick {
  my ($who, $new_nick) = @_;
  print "** Nick: $who $new_nick\n";
  dispatch( "nick_change", 0, 0, $who, $new_nick );
}

sub connected {
  print "** Connected\n";
  dispatch( "connected" );
}

sub namesReceived {
  my ($channel, $names ) = @_;
  print "** Names Received: $channel $names\n";
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
}

sub tick {
  print "** Tick\n";
  dispatch( "tick" );
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
  @modules = grep { $_->{Name} != $to_remove } @modules;
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
  eval {
    no warnings 'redefine';
    delete $INC{$file};
    require $file;
  };

  if( $@ )
  {
    warn("Error loading modules: $@");
    return 0;
  }
  $module = "DaZeus2Module::$module";

  push @modules, $module->new( UniqueID => $uniqueid );
  return 1;
}

1;
