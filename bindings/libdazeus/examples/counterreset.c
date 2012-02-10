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

	dazeus_scope *global = libdazeus_scope_global();

	char *cStr = libdazeus_get_property(d, "examples.counter.count", global);
	if(cStr) {
		int c = atoi(cStr);
		free(cStr);
		printf("Current count: %d\n", c);
	} else {
		printf("Current count: unset\n");
	}

	if(!libdazeus_unset_property(d, "examples.counter.count", global)) {
		fprintf(stderr, "Error resetting count variable: %s\n", libdazeus_error(d));
		libdazeus_close(d);
		return 4;
	}

	printf("Count reset!\n");

	libdazeus_scope_free(global);
	libdazeus_close(d);
	return 0;
}
