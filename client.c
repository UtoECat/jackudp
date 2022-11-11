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
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

int sock = 0;
int port = DEFAULT_PORT;

bool message = false;

void process(jack_default_audio_sample_t* data, size_t size) {
	buffer_check_size(size * 2);
	size_t readed = buffer_remove(data, size);
	if (readed == 0) {
			if (!message)
			perror("can't get data from server");
			message = true;
			memset(data, 0, sizeof(float) * size);
	} else message = false;
}

int main (int argc, const char* argv[]) {
	const char* server = NULL;
	const char* client = "udp_client";
	in_addr_t addr = DEFAULT_ADDRESS;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') switch (argv[i][1]) {
			case '\0' :
			case ' ' : break;
			case 'h' : 
				printf("Help is here! Usage : jackudpcli {-[FLAG] [ARG]}\n");
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
				printf("Setted jack client name %s\n", client);
			break;
			case 'p' :
				port = htons(atoi(argv[++i]));
				printf("Setted port %i\n", ntohs(port));
				break;
			case 'a' :
				addr = inet_addr(argv[++i]);
				printf("Setted server address : %i\n", addr);
				break;
		} else {
			printf("print jackudpcli -h to see help\n");
			printf("Bad argument #%i : %s\n", i, argv[i]);
			return -1;
		}
	}
	
	sock = udp_open_client(addr, htons(port));
	j_connect(client, server, 0); // we recieve data

	// server part
	char test[5] = {0};
	int cnt = 0; // count of buffer owerflows

	while (j_active()) {
			size_t v = buffer_read(sock, 1024);
			if (v == 0) {
				usleep(AWAIT_MICROSEC);
				cnt++; // buffer owerflow, or no data
			}
			else cnt = 0;
			if (cnt > 5) read(sock, test, 5); // or socket will be closed :(
	};

	perror("Jack server stopped!");
	close(sock);
}
