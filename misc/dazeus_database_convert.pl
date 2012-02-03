#!/usr/bin/perl
use strict;
use warnings;
use DBI;
use Storable qw( freeze thaw );
use MIME::Base64 qw(decode_base64 encode_base64);
use Data::Dumper;

help() if( @ARGV == 0 );

my $SAVE_FROM = $ENV{"IMPORT_TABLE"} || "basicbot";
my $SAVE_TO   = $ENV{"EXPORT_TABLE"} || "properties";
my $NS_VAR_TO_VAL = {};

## CONNECT TO OUTPUT
print "Connecting to output...\n";
my $OUTPUT_DSN = shift @ARGV;
my $OUTPUT;
{
  my ($dsn, $user, $pass) = parse_input($OUTPUT_DSN);
  $OUTPUT = DBI->connect($dsn, $user, $pass) or die( "Failed to connect to output ($OUTPUT): $!\n" );
}
my $NETWORKNAME = shift @ARGV;

## CONNECT
print "Connecting to inputs...\n";

my @INPUTS = map {
  my ($dsn, $user, $pass) = parse_input($_);
  DBI->connect($dsn, $user, $pass) or die( "Failed to connect to input ($_): $!\n" );
} @ARGV;

## FETCH ALL OPTIONS
print "Fetching data from " . scalar @INPUTS . " inputs...\n";
for( my $i = 0; $i < @INPUTS; ++$i )
{
  my $INPUT = $INPUTS[$i];
  my $DSN   = $ARGV[$i];

  # This piece was copied from DBI.pm in Bot::BasicBot::Pluggable 0.50
  my $sth = $INPUT->prepare_cached(
    "SELECT DISTINCT namespace FROM $SAVE_FROM"
  );
  $sth->execute() or die($!);
  my @namespaces = map { $_->[0] } @{ $sth->fetchall_arrayref };
  $sth->finish();

  foreach my $NAMESPACE (@namespaces)
  {
    $NS_VAR_TO_VAL->{$NAMESPACE} ||= {};

    $sth = $INPUT->prepare_cached(
      "SELECT DISTINCT store_key FROM $SAVE_FROM WHERE namespace=?"
    );
    $sth->execute($NAMESPACE) or die($!);
    my @variables = map { $_->[0] } @{$sth->fetchall_arrayref };
    $sth->finish();

    foreach my $VARIABLE (@variables)
    {
      $NS_VAR_TO_VAL->{$NAMESPACE}{$VARIABLE} ||= {};

      $sth = $INPUT->prepare_cached(
        "SELECT store_value FROM $SAVE_FROM WHERE namespace=? AND store_key=?"
      );
      $sth->execute($NAMESPACE, $VARIABLE) or die($!);
      my $row = $sth->fetchrow_arrayref;
      $sth->finish;

      if( $row and @$row )
      {
        my $value = dazeus_legacy_thaw( $row->[0] );
        $NS_VAR_TO_VAL->{$NAMESPACE}{$VARIABLE}{$DSN} = $value;
      }
    }
  }
}

## INSERT THE RIGHT OPTION
print "Inserting new data for network $NETWORKNAME...\n";
sleep 2; # so users can cancel

{
  my $idrow = ( $OUTPUT_DSN =~ /SQLite/ )
                ? "INTEGER PRIMARY KEY AUTOINCREMENT"
                : "INT(10) PRIMARY KEY AUTO_INCREMENT";
  my $sth = $OUTPUT->prepare_cached(
     "CREATE TABLE $SAVE_TO ("
    ."  id       $idrow,"
    ."  variable VARCHAR(150),"
    ."  network VARCHAR(50),"
    ."  receiver VARCHAR(50),"
    ."  sender VARCHAR(50),"
    ."  value TEXT"
    .");"
  );
  $sth->execute() or die $!;
  $sth->finish();
}

my $rowschanged = 0;
foreach my $NAMESPACE (keys %$NS_VAR_TO_VAL)
{
  foreach my $VARIABLE (keys %{ $NS_VAR_TO_VAL->{$NAMESPACE} })
  {
    my $values = $NS_VAR_TO_VAL->{$NAMESPACE}{$VARIABLE};
    my $value = (values %$values)[0];
    if( (keys %$values) > 1 )
    {
      $value = choose_opts( $NAMESPACE, $VARIABLE, $values );
    }
    next unless defined($value);

    my $sth = $OUTPUT->prepare_cached(
      "INSERT INTO $SAVE_TO (variable,value,network,receiver,sender)"
        ." VALUES (?, ?, ?, NULL, NULL)"
    );
    $sth->execute( "perl.$NAMESPACE.$VARIABLE", dazeus_freeze($value),
                   $NETWORKNAME ) or die($!);
    $rowschanged += $sth->rows;
    $sth->finish();
  }
}

print "Done. Changed $rowschanged rows in the new database.\n";
exit;

## HELPER METHODS;
sub dazeus_freeze {
  my ($value) = @_;
  return ( ref($value) ? encode_base64(freeze($value)) : $value );
}

sub choose_opts {
  my ($namespace, $variable, $values) = @_;
  my @opts = keys %$values;
  warn "\n";
  warn "Multiple options available for $namespace :: $variable.\n";
  warn " v) View dumps\n";
  warn " d) Don't import into new database (choose neither)\n";
  warn " p) Give some Perl code that will be evaled into a new value\n";
  for( my $i = 1; $i <= @opts; ++$i )
  {
    warn " $i) Choose value from DSN: " . $opts[$i-1] . "\n";
  }
  print STDERR "Enter your choice: ";
  my $answ = <STDIN>;
  1 while(chomp $answ);
  if( $answ eq "v" )
  {
    view_dumps($values);
    return choose_opts($namespace, $variable, $values);
  }
  elsif( $answ eq "d" )
  {
    return undef;
  }
  elsif( $answ eq "p" )
  {
    my $perl_code = get_perl_code();
    if( !$perl_code )
    {
      return choose_opts($namespace, $variable, $values);
    }

    my $result = eval $perl_code;
    if( $@ )
    {
      warn "Could not eval that: $@\n";
      return choose_opts($namespace, $variable, $values);
    }
    print "Thanks. Writing into database: \n";
    print Dumper($result);
    print "\n";
    return $result;
  }
  elsif( $answ =~ /^\d+$/ && $answ <= @opts )
  {
    return $values->{$opts[$answ-1]};
  }
  else
  {
    warn "That was not a valid choice. Try again.\n";
    return choose_opts($namespace, $variable, $values);
  }
}

sub view_dumps {
  my ($values) = @_;
  foreach my $DSN (keys %$values)
  {
    print "\n\nStructure for DSN $DSN:\n";
    print Dumper($values->{$DSN});
    print "\n";
  }
}

sub get_perl_code {
  print "Please type some Perl code, ending with a dot (.) when done.\n";
  print "If you've made a mistake, make sure the Perl code does not \n";
  print "compile, you will get another chance.\n";
  my $perl_code = "";
  while(<STDIN>) {
    last if /^\.\s*$/;
    $perl_code .= $_;
  }
  return $perl_code;
}

sub dazeus_legacy_thaw {
  return eval { thaw($_[0]) } || $_[0];
}

sub parse_input {
  my $_ = $_[0];
  if( /^(.+)!([^!]*)!([^!]*)$/ )
  {
    return ($1, $2, $3);
  }
  return ($_, 0, 0);
}

sub help {
  print <<"HELP";
This is the DaZeus legacy database conversion script. It can convert a legacy
database to a new one. When given multiple legacy databases, it can also merge
between them.
Usage:
  $0 dsn_save networkname dsn1 [dsn2 [dsn3 [....]]]

See the Perl DBI module documentation for more information on DSN's.

  dsn_save    The DSN to save to - MAKE SURE the table does not exist
              because it will be recreated!
  networkname The network name to use in the new database (the network
              identifier in your DaZeus 2 configuration file.)
  dsn1        The first DSN to load from
  dsn2        The second DSN to load from
  (etc)

If you need to give a username or password along with the DSN, you can do this
by appending the DSN with a username or password, with exclamation marks in
between. For example:

  $0 \
      "DBI:mysql:database=foo;host=localhost!username!password" \
      "freenode" \
      "DBI:SQLite:bot-basicbot.sqlite" "DBI:SQLite:bot-basicbot2.sqlite"

This command merges the input from the two given SQLite databases, and inserts
it into a MySQL database 'foo', identified by 'username' password 'password'.

Behind the given DSN, the module expects to find a table 'basicbot' - this can
be changed by using the environmental variable IMPORT_TABLE, which defines the
input table for all import DSN's. The table to which all data is saved is
by default 'properties', but this can be changed using EXPORT_TABLE.

HELP
  exit;
}
