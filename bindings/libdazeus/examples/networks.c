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
	dazeus_stringlist *net = libdazeus_networks(d);
	if(net == NULL) {
		fprintf(stderr, "Failed reading DaZeus networks: %s\n", d->error);
		libdazeus_close(d);
		return 4;
	}
	int i = 0;
	dazeus_stringlist *neti = net;
	while(neti != 0) {
		printf("Network %d: %s\n", ++i, neti->value);
		dazeus_stringlist *channels = libdazeus_channels(d, neti->value);
		if(channels == NULL) {
			fprintf(stderr, "Failed reading DaZeus channels for %s: %s\n", neti->value, d->error);
			libdazeus_close(d);
			return 5;
		}
		dazeus_stringlist *chani = channels;
		while(chani != 0) {
			printf("  Channel: %s\n", chani->value);
			chani = chani->next;
		}
		libdazeus_stringlist_free(channels);
		neti = neti->next;
	}
	libdazeus_stringlist_free(net);
	libdazeus_close(d);
	return 0;
}
