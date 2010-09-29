package DaZeus2Module;
use strict;
use warnings;
use Data::Dumper;

sub new {
    my $class = shift;
    my %param = @_;

    my $name = ref($class) || $class;
    $name =~ s/^.*:://;
    $param{Name} ||= $name;

    my $self = \%param;
    bless $self, $class;

    $self->init();

    return $self;
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

sub set {
    my $self = shift;
    # module: $self->{Name}
    # Var/val: @_
    die( "In " . $self->{Name} . ", set: " . Dumper(\@_) );
}

sub get {
    my $self = shift;
    #$self->store->get($self->{Name}, @_);
    die( "In " . $self->{Name} . ", get: " . Dumper(\@_) );
}

sub unset {
    my $self = shift;
    #$self->store->unset($self->{Name}, @_);
    die( "In " . $self->{Name} . ", unset: " . Dumper(\@_ ) );
}

sub store_keys {
    my $self = shift;
    $self->store->keys($self->{Name}, @_);
    die( "In " . $self->{Name} . ", store keys: " . Dumper(\@_ ) );
}

sub say {
  my $self = shift;
  my $args = $_[0];
  DaZeus2::privmsg($args->{channel}, $args->{body});
}

sub emote {
  my $self = shift;
  my %args = @_;
  DaZeus2::emote($args{channel}, $args{body});
}

sub sendWhois {
  my $self = shift;
  die( "Send whois: " . Dumper(\@_) );
  return $self->{Bot}->sendWhois(@_);
}

sub reply {
  my $self = shift;
  die( "Reply: " . Dumper(\@_) );
  return $self->{Bot}->reply(@_);
}

sub tell {
  my $self = shift;
  my $target = shift;
  my $body = shift;
  die( "Tell target=$target, body: $body" );
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
