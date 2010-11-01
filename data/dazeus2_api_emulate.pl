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

sub loadModule {
  my ($module) = @_;

  foreach(@modules)
  {
    if( $_ eq $module)
    {
      return 1;
    }
  }

  if( ! -e "$module.pm" )
  {
    warn "Could not find $module.pm\n";
    return 0;
  }

  eval "do \"$module.pm\";";
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
