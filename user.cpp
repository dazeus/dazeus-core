/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "user.h"
#include "network.h"

QDebug operator<<(QDebug dbg, const User &u)
{
  dbg.nospace() << "User[" << u.nickName() << "]";

  return dbg.space();
}

User::User( const QString &fullName, Network *n )
{
  setFullName( fullName );
  setNetwork( n );
}

User::User( const QString &nick, const QString &ident, const QString &host, Network *n )
{
  setNick( nick );
  setIdent( ident );
  setHost( host );
  setNetwork( n );
}


/**
 * @brief Get the User object pointing to ourselves.
 *
 * This static method can be called from anywhere to indicate ourselves.
 * Since our user information can differ per network, a "User" object exists
 * for every network, so you have to give the network name for which you want
 * to get the User object.
 */
User *User::getMe( const QString &network )
{
  Network *n = Network::getNetwork( network );
  return n->user();
}


const QString &User::host() const
{
  return host_;
}


const QString &User::ident() const
{
  return ident_;
}


bool User::isMe() const
{
  return network()->user()->nick() == nick();
}


Network *User::network() const
{
  return network_;
}


const QString &User::nickName() const
{
  return nick_;
}

void User::setHost( const QString &host )
{
  host_ = host;
}


void User::setFullName( const QString &fullName )
{
  int pos1 = fullName.indexOf( '!' );
  int pos2 = fullName.indexOf( '@', pos1 == -1 ? 0 : pos1 );

  // left(): The entire string is returned if n is [...] less than zero.
  setNick( fullName.left( pos1 != -1 ? pos1 : pos2 ) );
  setIdent( pos1 == -1 ? "" : fullName.mid( pos1 + 1, pos2 - pos1 - 1 ) );
  setHost(  pos2 == -1 ? "" : fullName.mid( pos2 + 1 ) );
  //setIdent( fullName.mid( pos1 + 1 ) );
  //setIdent( fullName.mid( pos1 + 1 ), pos2 - pos1 - 1 );
}


void User::setIdent( const QString &ident )
{
  ident_ = ident;
}


void User::setNetwork( Network *n )
{
  network_ = n;
}


void User::setNick( const QString &nick )
{
  nick_ = nick;
}
