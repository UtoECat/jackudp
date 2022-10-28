/*
 * jack_udp_transmission - transfer audio data using udp without any
 * encryption and guaranties :D
 * Copyright (C) UtoECat 2022. All rights reserved!

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <shared.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


int sock = 0;
int port = DEFAULT_PORT;

void process(jack_default_audio_sample_t* data, size_t size) {
	size_t readed = 0;
	size_t cnt = 0;
	while (readed < size) {
		ssize_t v = read(sock, data + readed, (size - readed) * sizeof(float));
		if (v <= 0) break;
		readed += v / sizeof(float);
	}
	if (readed < size) perror("can't get data from client");
}

int main (int argc, const char* argv[]) {
	const char* server = NULL;
	const char* client = "udp_server";
	in_addr_t addr = DEFAULT_ADDRESS;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') switch (argv[i][1]) {
			case '\0' :
			case ' ' : break;
			case 'h' : 
				printf("Help is here! Usage : jackudpserv {-[FLAG] [ARG]}\n");
				printf("f - set samplerate, s - set server name, p - set port,\
						a - set server ip address, c - set jack client name");
				break;
			case 'f' : 
				size_t freq = atol(argv[++i]);
				printf("Setted sample rate %li\n", freq);
				j_transfer_sample_rate(&freq);
			break;
			case 's' :
				server = argv[++i];
				printf("Setted jack server %s\n", server);
			break;
			case 'c' :
				client = argv[++i];
				printf("Setted jack client name %s\n", server);
			break;
			case 'p' :
				port = htons(atoi(argv[++i]));
				printf("Setted port (BIG ENDIAN) %i\n", port);
				break;
			case 'a' :
				i++;
				printf("To change server ip, change your provider xD\n");
				break;
		} else {
			printf("print jackudpsrv -h to see help\n");
			printf("Bad argument #%i : %s\n", i, argv[i]);
			return -1;
		}
	}
	sock = udp_open_server(htons(port));
	char test[5] = {0};
	fprintf(stderr, "readed %li\n", read(sock, test, 4));
	j_connect(client, server, 0);
	while (j_active()) {sleep(1);}
	perror("Jack server stopped!");
	close(sock);
}
