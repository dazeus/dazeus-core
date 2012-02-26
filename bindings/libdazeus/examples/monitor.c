#include <libdazeus.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	if(argc != 2) {
		fprintf(stderr, "Usage: %s socket\n", argv[0]);
		return 1;
	}
	dazeus *d = libdazeus_create();
	if(!d) {
		fprintf(stderr, "Failed to create DaZeus object.\n");
		return 2;
	}
	if(!libdazeus_open(d, argv[1])) {
		fprintf(stderr, "Error connecting to DaZeus: %s\n", libdazeus_error(d));
		libdazeus_close(d);
		return 3;
	}

#define s(a) if(!libdazeus_subscribe(d, a)) { \
		fprintf(stderr, "Error subscribing to event %s: %s\n", a, libdazeus_error(d)); \
		libdazeus_close(d); \
		return 4; \
	}
	s("CONNECT"); s("DISCONNECT"); s("JOIN"); s("PART"); s("QUIT");
	s("NICK"); s("MODE"); s("TOPIC"); s("INVITE"); s("KICK"); s("PRIVMSG");
	s("NOTICE"); s("CTCP"); s("CTCP_REP"); s("ACTION"); s("NUMERIC");
	s("UNKNOWN"); s("WHOIS"); s("NAMES");
#undef s

	dazeus_event *e;
	// Wait indefinitely for an event
	while((e = libdazeus_handle_event(d, -1))) {
		printf("Event: %s\n", e->event);
		printf("Parameters:\n");
		dazeus_stringlist *params = e->parameters;
		int i = 0;
		while(params != 0)
		{
			printf("  %i: %s\n", ++i, params->value);
			params = params->next;
		}
		printf("\n");
		libdazeus_event_free(e);
	}

	libdazeus_close(d);
	return 0;
}
