
#include "embedperl.h"

int main( int argc, char *argv[] )
{
  EmbedPerl ep;
  ep.loadModule( "DazMessages" );
  ep.message( "Sjors", "#ru", "}ping" );
  ep.message( "Sjors", "#ru", "}fortune" );
  ep.message( "Sjors", "#ru", "}order a coffee for everyone" );
  ep.message( "Sjors", "#ru", "}setchannelsay #dazjorz" );
  ep.message( "Sjors", "#ru", "}dosay Foo bar baz!" );
  ep.message( "Sjors", "#ru", "}ispriem 63" );

  return 0;
}
