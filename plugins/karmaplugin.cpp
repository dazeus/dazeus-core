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

int KarmaPlugin::modifyKarma(const QString &object, bool increase) {
	QString qualifiedName = QLatin1String("perl.DazKarma.karma_") + object;
	int current = get(qualifiedName).toInt();
	if(increase)
		++current;
	else	--current;
	
	if(current == 0) {
		// Null QVariant() will unset the variable
		set(Plugin::NetworkScope, qualifiedName, QVariant());
	} else {
		set(Plugin::NetworkScope, qualifiedName, current);
	}

	return current;
}

void KarmaPlugin::messageReceived( Network &net, const QString &origin,
	const QString &message, Irc::Buffer *buffer )
{
	if(message.startsWith(QLatin1String("}karma "))) {
		QString object = message.mid(7);
		int current = get(QLatin1String("perl.DazKarma.karma_") + object).toInt();
		if(current == 0) {
			buffer->message(object + QLatin1String(" has neutral karma."));
		} else {
			buffer->message(object + QLatin1String(" has a karma of ") + QString::number(current) + QLatin1Char('.'));
		}
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
		if( message[i] == QLatin1Char('-') && message[i+1] == QLatin1Char('-') && wordEnd ) {
			hits.append(i);
		}
		else if( message[i] == QLatin1Char('+') && message[i+1] == QLatin1Char('+') && wordEnd ) {
			hits.append(i);
		}
	}

	QListIterator<int> i(hits);
	while(i.hasNext()) {
		int pos = i.next();
		bool isIncrease = message[pos] == QLatin1Char('+');
		QString object;
		int newVal;

		if(message[pos-1].isLetter()) {
			// only alphanumerics between startPos and pos-1
			int startPos = pos - 1;
			for(; startPos >= 0; --startPos) {
				if(!message[startPos].isLetter())
				{
					// revert the negation
					startPos++;
					break;
				}
			}
			if(startPos > 0 && !message[startPos-1].isSpace()) {
				// non-alphanumerics would be in this object, ignore it
				continue;
			}
			object = message.mid(startPos, pos - startPos);
			newVal = modifyKarma(object, isIncrease);
			qDebug() << origin << (isIncrease ? "increased" : "decreased")
			         << "karma of" << object << "to" << QString::number(newVal);
			continue;
		}

		char beginner;
		char ender;
		if(message[pos-1] == QLatin1Char(']')) {
			beginner = '[';
			ender = ']';
		} else if(message[pos-1] == QLatin1Char(')')) {
			beginner = '(';
			ender = ')';
		} else {
			continue;
		}

		// find the last $beginner before $ender
		int startPos = message.lastIndexOf(QLatin1Char(beginner), pos);
		// unless there's already an $ender between them
		if(message.indexOf(QLatin1Char(ender), startPos) < pos - 1)
			continue;

		object = message.mid(startPos + 1, pos - 2 - startPos);
		newVal = modifyKarma(object, isIncrease);
		QString message = origin + QLatin1String(isIncrease ? " increased" : " decreased")
		                + QLatin1String(" karma of ") + object + QLatin1String(" to ")
		                + QString::number(newVal) + QLatin1Char('.');
		qDebug() << message;
		if(ender == ']') {
			// Verbose mode, print the result
			buffer->message(message);
		}
	}
}
