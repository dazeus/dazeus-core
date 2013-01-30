#!/usr/bin/perl

open( VERSION, "<version" );
my %version;
while( <VERSION> ) {
    chomp;
    if( /^(.*?)=(.*)/ ) {
        $version{$1} = $2;
    }
}

my $dir = "tmp/doxygenfilter-$version{VERSION}";
die "run step 01 first, unpacked dir does not exist!" unless( -d $dir );
chdir( $dir );
system( "perlapp --lib lib bin\\doxygenfilter" )
  and die "perl compiling failed";
