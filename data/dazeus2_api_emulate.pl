
package Perms;

use Data::Dumper;
use strict;
use warnings;

sub has {
  my ($self, $whatperm) = @_;
  my $res = $self->{$whatperm} || 0;
  warn("Perms::has($whatperm)=$res\n");
  return $res;
}

package main;

use DaZeus2;
use Data::Dumper;
use strict;
use warnings;

my @modules;
my $perms;
my $uniqueid;

sub init {
  ($uniqueid) = @_;
  $perms = {
    Sjors => {"dazeus.commands.fortune" => 1,
              "dazeus.commands.order"   => 1,
              "dazeus.commands.dosay"   => 1,
              "dazeus.commands.dosay.setchannel" => 1},
    any   => {"dazeus.commands.fortune" => 1,
              "dazeus.commands.order"   => 1},
  };
  bless $perms->{Sjors}, "Perms";
  bless $perms->{any}, "Perms";
}

sub message {
  my ($sender, $receiver, $body, $raw_body) = @_;
  my $mess = {
    perms   => ($sender eq "Sjors" ? $perms->{Sjors} : $perms->{any}),
    channel => $receiver,
    body    => $body,
    raw_body => $raw_body,
    who     => $sender,
  };

  NEXTMOD: for my $mod (@modules)
  {
    for my $pri (0..3)
    {
      my $message;
      eval { $message = $mod->said($mess,$pri) };
      if( $@ )
      {
        warn("Error executing $mod ->said(): $@\n" );
        $mod->say({channel => $receiver, body => "Error executing $mod ->said(): $@\n" });
        next NEXTMOD;
      }
      elsif( $message )
      {
        if( $message ne "1" )
        {
          $mod->say({channel => $receiver, body => $message});
        }
        next NEXTMOD;
      }
    }
  }

  return;
}

sub loadModule {
  my ($module) = @_;

  foreach(@modules)
  {
    if( $_ == $module)
    {
      return 1;
    }
  }

  if( ! -e "$module.pm" )
  {
    warn "Could not find $module.pm\n";
    return 0;
  }

  eval "do \"DaZeus2Module.pm\"; do \"$module.pm\";";
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
