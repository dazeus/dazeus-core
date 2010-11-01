use DaZeus2;
use Data::Dumper;
use strict;
use warnings;

my @modules;
my $uniqueid;

sub init {
  ($uniqueid) = @_;
}

sub message {
  my ($sender, $receiver, $body, $raw_body) = @_;
  my $mess = {
    channel => $receiver,
    body    => $body,
    raw_body => $raw_body,
    who     => $sender,
  };

  for my $pri (0..3)
  {
    NEXTMOD: for my $mod (@modules)
    {
      my $message;
      eval { $message = $mod->said($mess,$pri) };
      if( $@ )
      {
        warn("Error executing $mod ->said(): $@\n" );
        $mod->say({channel => $receiver, body => "Error executing $mod ->said(): $@\n" });
        next NEXTMOD;
      }
      elsif( $message && $message ne "1" )
      {
        $mod->say({channel => $receiver, body => $message});
        return;
      }
    }
  }

  return;
}

sub __module {
  my $name = @_;
  foreach(@modules)
  {
    if( $_->{Name} eq $name )
    {
      return $_;
    }
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

  push @modules, $module->new();
  $module->uniqueid( $uniqueid );
  return 1;
}

1;
