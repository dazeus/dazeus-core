#include <libdazeus.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

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

	dazeus_scope *global = libdazeus_scope_global();
	dazeus_scope *oftc   = libdazeus_scope_network("oftc");
	dazeus_scope *q      = libdazeus_scope_network("q");
	dazeus_scope *qmoo   = libdazeus_scope_receiver("q", "#moo");
	dazeus_scope *oftcmoo= libdazeus_scope_receiver("oftc", "#moo");

#define check(a) if(!a) { fprintf(stderr, "Error setting variable: %s\n", \
		libdazeus_error(d)); libdazeus_close(d); return 4; }

	check(libdazeus_set_property(d, "examples.scope.foo", "bar", global));
	printf("Set global scope to bar\n");
	check(libdazeus_set_property(d, "examples.scope.foo", "baz", oftc));
	printf("Set network scope for 'oftc' to baz\n");

	char *cStr = libdazeus_get_property(d, "examples.scope.foo", qmoo);
	if(cStr == NULL) {
		cStr = strdup("__unset__");
	}
	// Should be 'bar':
	printf("Value for network scope 'q', channel #moo: %s\n", cStr);
	free(cStr);
	cStr = libdazeus_get_property(d, "examples.scope.foo", oftcmoo);
	if(cStr == NULL) {
		cStr = strdup("__unset__");
	}
	// Should be 'baz':
	printf("Value for network scope 'oftc', channel #moo: %s\n", cStr);
	free(cStr);

	check(libdazeus_unset_property(d, "examples.scope.foo", oftc));
	printf("Unset network scope for 'oftc'\n");
	cStr = libdazeus_get_property(d, "examples.scope.foo", oftcmoo);
	if(cStr == NULL) {
		cStr = strdup("__unset__");
	}
	// Should be 'bar':
	printf("Value for network scope 'oftc', channel #moo: %s\n", cStr);
	free(cStr);
	check(libdazeus_unset_property(d, "examples.scope.foo", global));
	printf("Unset global scope\n");

	check(libdazeus_set_property(d, "examples.scope.foo", "quux", oftc));
	printf("Set network scope 'oftc' to quux\n");
	cStr = libdazeus_get_property(d, "examples.scope.foo", q);
	if(cStr == NULL) {
		cStr = strdup("__unset__");
	}
	// Should be __unset__:
	printf("Value for network 'q': %s\n", cStr);
	free(cStr);

	libdazeus_scope_free(global);
	libdazeus_scope_free(oftc);
	libdazeus_scope_free(q);
	libdazeus_scope_free(qmoo);
	libdazeus_scope_free(oftcmoo);
	libdazeus_close(d);
	return 0;
}
