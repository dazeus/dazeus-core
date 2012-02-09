#include <libdazeus.h>
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
	dazeus_network *net = libdazeus_networks(d);
	int i = 0;
	while(net != 0) {
		printf("Network %d: %s\n", ++i, net->network_name);
		net = net->next;
	}
	libdazeus_close(d);
	return 0;
}
