/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#include "plugin.h"

Plugin::Plugin()
: QObject()
{}

Plugin::~Plugin()
{}


void Plugin::set( const QString &name, const QVariant &value )
{
}

QVariant Plugin::get( const QString &name ) const
{
  return QVariant();
}

void Plugin::emote( const QString &receiver, const QString &body )
{
  qDebug() << "Plugin emote: " << receiver << body;
}

void Plugin::privmsg( const QString &receiver, const QString &body )
{
  qDebug() << "Plugin privmsg: " << receiver << body;
}
