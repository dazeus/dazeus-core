/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <QtCore/QObject>

class Database : public QObject
{
  Q_OBJECT

  public:
                    Database( const QString &network );
                   ~Database();

    const QString  &network() const;
  public slots:


  private:
    QString network_;

};

#endif
