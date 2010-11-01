package DaZeus2Module;
use strict;
use warnings;
use Data::Dumper;
use Storable qw(thaw freeze);
use MIME::Base64 qw(encode_base64 decode_base64);

my $uniqueid;

sub new {
    my $class = shift;
    my %param = @_;

    my $name = ref($class) || $class;
    $name =~ s/^.*:://;
    $param{Name} ||= $name;

    my $self = \%param;
    bless $self, $class;

    eval {
      $self->init();
    };
    if( $@ )
    {
      warn "Init failed: $@\n";
    }

    return $self;
}

sub uniqueid {
    (undef,$uniqueid) = @_;
}

sub bot {
    # bot == module
    return shift;
}

sub store {
    # store == module
    return shift;
}

sub var {
    my $self = shift;
    my $name = shift;
    if (@_) {
        return $self->set($name, shift);
    } else {
        return $self->get($name);
    }
}

sub getNick {
    my $self = shift;
    my $nick;
    eval {
      $nick = DaZeus2::getNick();
    };
    if( $@ )
    {
      warn "Error executing getNick(): $@";
    }
    return $nick;
}

sub set {
    my $self = shift;
    my $qualifier = "perl." . $self->{Name} . "." . $_[0];
    my $value     = encode_base64(freeze($_[1]));
    eval {
      DaZeus2::setProperty($uniqueid, $qualifier, $value);
    };
    if( $@ )
    {
      warn "Error executing setProperty: $@\n";
    }
}

sub get {
    my $self = shift;
    my $qualifier = "perl." . $self->{Name} . "." . $_[0];
    my $value;
    eval {
        $value = DaZeus2::getProperty($uniqueid, $qualifier);
    };
    if( $@ )
    {
      warn "Error executing getProperty: $@\n";
    }
    $value = thaw(decode_base64($value)) if( defined($value) );
    return $value;
}

sub unset {
    my $self = shift;
    my $qualifier = "perl." . $self->{Name} . "." . $_[0];
    eval {
      DaZeus2::unsetProperty($uniqueid, $qualifier);
    };
    if( $@ )
    {
      warn "Error executing unsetProperty: $@\n";
    }
}

sub store_keys {
    my $self = shift;
    $self->store->keys($self->{Name}, @_);
    die( "In " . $self->{Name} . ", store keys: " . Dumper(\@_ ) );
}

sub say {
  my $self = shift;
  my $args = $_[0];
  my $channel = $args->{channel} eq "msg" ? $args->{who} : $args->{channel};
  eval {
    DaZeus2::privmsg($uniqueid, $channel, $args->{body});
  };
  if( $@ )
  {
    warn "Error executing privmsg(): $@\n";
  }
}

sub emote {
  my $self = shift;
  my %args = @_;
  eval {
    DaZeus2::emote($uniqueid, $args{channel}, $args{body});
  };
  if( $@ )
  {
    warn "Error executing emote(): $@\n";
  }
}

sub sendWhois {
  my $self = shift;
  die( "Send whois: " . Dumper(\@_) );
  return $self->{Bot}->sendWhois(@_);
}

sub reply {
  my $self = shift;
  my $mess = $_[0];
  $self->say( {
    channel => $mess->{channel},
    $mess->{channel} eq "msg" ? (who => $mess->{who}) : (),
    body    => $_[1],
  } );
}

sub tell {
  my $self = shift;
  my $target = shift;
  my $body = shift;
  if ($target =~ /^#/) {
    $self->say({ channel => $target, body => $body });
  } else {
    $self->say({ channel => 'msg', body => $body, who => $target });
  }
}

sub said {
  my ($self, $mess, $pri) = @_;
  $mess->{body} =~ s/\s+$//;
  $mess->{body} =~ s/^\s+//;

  if ($pri == 0) {
    return $self->seen($mess);
  } elsif ($pri == 1) {
    return $self->admin($mess);
  } elsif ($pri == 2) {
    return $self->told($mess);
  } elsif ($pri == 3) {
    return $self->fallback($mess);
  }
  return undef;
}

sub whois { undef }
sub got_names { undef }
sub seen { undef }
sub admin { undef }
sub told { undef }
sub fallback { undef }
sub reload { undef }
sub connected { undef }
sub init { undef }

sub emoted { undef }
sub tick { undef }
sub chanjoin { undef }
sub chanpart { undef }
sub nick_change { undef }

1;
