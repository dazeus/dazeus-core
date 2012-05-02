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

#include "dazeus.h"
#include <libdazeus.h>

///// SCOPE /////
DaZeus::Scope::Scope()
: s_(libdazeus_scope_global()) {}
DaZeus::Scope::Scope(QString network)
: s_(libdazeus_scope_network(network.toUtf8())) {}
DaZeus::Scope::Scope(QString network, QString receiver)
: s_(libdazeus_scope_receiver(network.toUtf8(), receiver.toUtf8())) {}
DaZeus::Scope::Scope(QString network, QString receiver, QString sender)
: s_(libdazeus_scope_sender(network.toUtf8(), receiver.toUtf8(), sender.toUtf8())) {}

DaZeus::Scope::~Scope() {
	libdazeus_scope_free(s_);
}

DaZeus::ScopeType DaZeus::Scope::type() const {
	switch(s_->scope_type) {
	case DAZEUS_GLOBAL_SCOPE:
		return GlobalScope;
	case DAZEUS_NETWORK_SCOPE:
		return NetworkScope;
	case DAZEUS_RECEIVER_SCOPE:
		return ReceiverScope;
	case DAZEUS_SENDER_SCOPE:
		return SenderScope;
	default:
		Q_ASSERT(!"Scope not recognised...");
		return GlobalScope;
	}
}

QString DaZeus::Scope::network() const {
	return QString::fromUtf8(s_->network);
}

QString DaZeus::Scope::receiver() const {
	return QString::fromUtf8(s_->receiver);
}

QString DaZeus::Scope::sender() const {
	return QString::fromUtf8(s_->sender);
}

//////DAZEUS///////

DaZeus::DaZeus() : d_(libdazeus_create()), n_(0) {}
DaZeus::~DaZeus() {
	libdazeus_close(d_);
	delete n_;
}

QString DaZeus::error() const {
	return QString::fromUtf8(libdazeus_error(d_));
}

bool DaZeus::open(const QString &socketfile) {
	int retval = libdazeus_open(d_, socketfile.toUtf8());
	if(retval != 1)
		return false;
	int fd = libdazeus_get_socket(d_);

	delete n_;
	n_ = new QSocketNotifier(fd, QSocketNotifier::Read);
	connect(n_,   SIGNAL(activated(int)),
	        this,   SLOT(activated()));
	return true;
}

bool DaZeus::connected() {
	return libdazeus_get_socket(d_) != 0;
}

QStringList DaZeus::networks() {
	dazeus_stringlist *networks = libdazeus_networks(d_);
	dazeus_stringlist *neti = networks;
	QStringList results;
	while(neti != 0) {
		results << QString::fromUtf8(neti->value);
		neti = neti->next;
	}
	libdazeus_stringlist_free(networks);
	return results;
}

QStringList DaZeus::channels(const QString &network) {
	dazeus_stringlist *channels = libdazeus_channels(d_, network.toUtf8());
	dazeus_stringlist *chani = channels;
	QStringList results;
	while(chani != 0) {
		results << QString::fromUtf8(chani->value);
		chani = chani->next;
	}
	libdazeus_stringlist_free(channels);
	return results;
}

bool DaZeus::message(const QString &network, const QString &receiver, const QString &message)
{
	return 1 == libdazeus_message(d_, network.toUtf8(), receiver.toUtf8(), message.toUtf8());
}

QString DaZeus::getProperty(const QString &variable, const Scope &scope) {
	char *result = libdazeus_get_property(d_, variable.toUtf8(), scope.s_);
	if(result == 0) {
		return QString();
	} else {
		QString res = QString::fromUtf8(result);
		free(result);
		return res;
	}
}

bool DaZeus::setProperty(const QString &variable, const QString &value, const Scope &scope) {
	return 1 == libdazeus_set_property(d_, variable.toUtf8(), value.toUtf8(), scope.s_);
}

bool DaZeus::unsetProperty(const QString &variable, const Scope &scope) {
	return 1 == libdazeus_unset_property(d_, variable.toUtf8(), scope.s_);
}

bool DaZeus::subscribe(const QString &event) {
	return 1 == libdazeus_subscribe(d_, event.toUtf8());
}

bool DaZeus::subscribe(const QStringList &events) {
	bool success = true;
	foreach(const QString &s, events) {
		if(!subscribe(s))
			success = false;
	}
	return success;
}

void DaZeus::activated() {
	bool handled;
	do {
		handled = handleEvent(0);
		if(!connected()) {
			// connection closed while retrieving events
			delete n_;
			n_ = 0;
			emit connectionFailed();
		}
	} while(handled);
}

bool DaZeus::handleEvent(int timeout) {
	dazeus_event *de = libdazeus_handle_event(d_, timeout);
	if(de == NULL) {
		return false;
	}

	DaZeus::Event *e = new DaZeus::Event;
	e->event = QString::fromUtf8(de->event);

	dazeus_stringlist *ei = de->parameters;
	while(ei != 0) {
		e->parameters << QString::fromUtf8(ei->value);
		ei = ei->next;
	}

	libdazeus_event_free(de);
	emit newEvent(e);
	delete e;
	return true;
}
