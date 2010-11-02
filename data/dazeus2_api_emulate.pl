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
    NEXTMOD: for my $mod (@modules)
    {
      my $message;
      eval { $message = $mod->said($mess,$pri) };
      if( $@ )
      {
        warn("Error executing $mod ->said(): $@\n" );
        $mod->say({channel => $mess->{channel},
                   who     => $mess->{who},
                   body    => "Error executing $mod ->said(): $@\n" });
        next NEXTMOD;
      }
      elsif( $message && $message ne "1" && !ref($message) )
      {
        $mod->say({channel => $mess->{channel},
                   who     => $mess->{who},
                   body    => $message});
        return;
      }
    }
  }

  return;
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
