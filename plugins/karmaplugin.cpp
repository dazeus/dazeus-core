/**
 * Copyright (c) Sjors Gielen, 2011
 * See LICENSE for license.
 */

#include <QtCore/QList>
#include <QtCore/QDebug>
#include <ircclient-qt/IrcBuffer>

#include "karmaplugin.h"

KarmaPlugin::KarmaPlugin( PluginManager *man )
: Plugin( man )
{}

KarmaPlugin::~KarmaPlugin() {}

void KarmaPlugin::init()
{
  qDebug() << "KarmaPlugin initialising.";
}

int KarmaPlugin::modifyKarma(QString object, bool increase) {
	QString qualifiedName = "perl.DazKarma.karma_" + object;
	int current = get(qualifiedName).toInt();
	if(increase)
		++current;
	else	--current;
	
	if(current == 0) {
		// Null QVariant() will unset the variable
		set(Plugin.NetworkScope, qualifiedName, QVariant());
	} else {
		set(Plugin.NetworkScope, qualifiedName, current);
	}

	return current;
}

void KarmaPlugin::messageReceived( Network &net, const QString &origin,
	const QString &message, Irc::Buffer *buffer )
{
	if(message.startsWith("}karma ")) {
		QString object = message.mid(7);
		int current = get("perl.DazKarma.karma_ " + object).toInt();
		buffer->message(object + " has a karma of " + QString::number(current) + ".");
		return;
	}

	// Walk through the message searching for -- and ++; upon finding
	// either, reconstruct what the object was.
	// Start searching from i=1 because a string starting with -- or
	// ++ means nothing anyway, and now we can always ask for b[i-1]
	// End search at one character from the end so we can always ask
	// for b[i+1]
	QList<int> hits;
	int len = message.length();
	for(int i = 1; i < (len - 1); ++i) {
		bool wordEnd = i == len - 2 || message[i+2].isSpace();
		if( message[i] == '-' && message[i+1] == '-' && wordEnd ) {
			hits.append(i);
		}
		else if( message[i] == '+' && message[i+1] == '+' && wordEnd ) {
			hits.append(i);
		}
	}

	QListIterator<int> i(hits);
	while(i.hasNext()) {
		int pos = i.next();
		bool isIncrease = message[pos] == '+';
		QString object;
		int newVal;

		if(message[pos-1].isLetter()) {
			// only alphanumerics between startPos and pos-1
			int startPos = pos - 1;
			for(; startPos >= 0; --startPos) {
				if(!message[startPos].isLetter())
					break;
			}
			object = message.mid(startPos, pos - startPos);
			newVal = modifyKarma(object, isIncrease);
			qDebug() << origin << (isIncrease ? "increased" : "decreased")
			         << "karma of" << object << "to" << QString::number(newVal);
			continue;
		}

		char beginner;
		char ender;
		if(message[pos-1] == ']') {
			beginner = '[';
			ender = ']';
		} else if(message[pos-1] == ')') {
			beginner = '(';
			ender = ')';
		} else {
			continue;
		}

		// find the last $beginner before $ender
		int startPos = message.lastIndexOf(beginner, pos);
		// unless there's already an $ender between them
		if(message.indexOf(ender, startPos) < pos - 1)
			continue;

		newVal = modifyKarma(message.mid(startPos + 1, pos - 2 - startPos), isIncrease);
		QString message = origin + (isIncrease ? " increased" : " decreased")
			          + " karma of " + object + " to " + QString::number(newVal);
		qDebug() << message;
		if(ender == ']') {
			// Verbose mode, print the result
			buffer->message(message);
		}
	}
}
