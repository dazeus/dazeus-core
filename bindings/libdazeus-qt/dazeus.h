/**
 * LibDazeus-Qt -- a Qt library that implements the DaZeus 2 Plugin Protocol
 * 
 * Copyright (c) Sjors Gielen, 2010-2012
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the DaVinci or DaZeus team nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SJORS GIELEN OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DAZEUS_H
#define DAZEUS_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

#define LIBDAZEUS_QT_VERSION_STR "2.0-beta1"
#define LIBDAZEUS_QT_VERSION 0x010901

struct dazeus_struct;
typedef struct dazeus_struct dazeus;
struct dazeus_scope_struct;
typedef struct dazeus_scope_struct dazeus_scope;

class DaZeus : public QObject
{
Q_OBJECT

public:
	enum ScopeType {
		GlobalScope = 0x00,
		NetworkScope = 0x01,
		ReceiverScope = 0x03,
		SenderScope = 0x07,
	};

	// dazeus_stringlist -> QStringList
	// dazeus_event->_next -> internal
	struct Event {
		QString event;
		QStringList parameters;
	};

	struct Scope {
		Scope(); // global
		Scope(QString network); // network
		Scope(QString network, QString receiver); // receiver
		Scope(QString network, QString receiver, QString sender); // sender
		~Scope();
		ScopeType type() const;
		QString network() const;
		QString receiver() const;
		QString sender() const;
	private:
		friend class DaZeus;
		dazeus_scope *s_;
	};

	DaZeus();
	~DaZeus();

	/**
	 * Retrieve the last error generated or NULL if there was no error.
	 */
	QString error() const;

	/**
	 * Connect this dazeus instance. Takes socket location as its only parameter.
	 * Socket can be either of the form "unix:/path/to/unix/socket" or
	 * "tcp:hostname:port" (including "tcp:127.0.0.1:1234").
	 */
	bool open(const QString &socketfile);

	QStringList networks();
	QStringList channels(const QString &network);
	bool message(const QString &network, const QString &receiver, const QString &message);
	QString getProperty(const QString &variable, const Scope &scope);
	bool setProperty(const QString &variable, const QString &value, const Scope &scope);
	bool unsetProperty(const QString &variable, const Scope &scope);

	bool subscribe(const QString &event);
	bool subscribe(const QStringList &event);
	Event *handleEvent();

private:
	dazeus *d_;
};

#endif
