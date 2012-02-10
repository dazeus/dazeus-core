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
	if(net == NULL) {
		fprintf(stderr, "Failed reading DaZeus networks: %s\n", d->error);
		libdazeus_close(d);
		return 4;
	}
	int i = 0;
	dazeus_network *neti = net;
	while(neti != 0) {
		printf("Network %d: %s\n", ++i, neti->network_name);
		neti = neti->next;
	}
	libdazeus_networks_free(net);
	libdazeus_close(d);
	return 0;
}
