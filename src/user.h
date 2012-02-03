/**
 * Copyright (c) Sjors Gielen, 2010
 * See LICENSE for license.
 */

#ifndef USER_H
#define USER_H

#include <QtCore/QString>
#include <QtCore/QDebug>

class User;
class Network;

QDebug operator<<(QDebug, const User &);

class User {
  public:
            User( const QString &fullName, Network *n );
            User( const QString &nick, const QString &ident, const QString &host, Network *n );
    void    setFullName( const QString &fullName );
    void    setNick( const QString &nick );
    void    setIdent( const QString &ident );
    void    setHost( const QString &host );
    void    setNetwork( Network *n );
    const QString &fullName() const;
    const QString &nickName() const;
    const QString &nick() const { return nickName(); }
    const QString &ident() const;
    const QString &host() const;
    Network *network() const;

    bool    isMe() const;
    bool    isMe( const QString &network ) const;
    bool    isSameUser( const User &other ) const;

    static User *getMe( const QString &network );
    static bool isSameUser( const User &one, const User &two );
    static bool isMe( const User &which, const QString &network );

  private:
    QString nick_;
    QString ident_;
    QString host_;
    User   *me;
    Network *network_;
};

#endif
