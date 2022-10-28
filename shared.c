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

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <jack/jack.h>
#include <shared.h>
#include <string.h>
#include <threads.h>

static jack_client_t* j_client;
static jack_port_t* j_port;
static mtx_t j_lock;
static size_t j_sample_rate = 44800; // must be same on both client/server side!

static jack_default_audio_sample_t* buffer = NULL;
static size_t buff_len = 0;
static size_t buff_pos = 0;
static mtx_t  buff_lock;

extern void process(jack_default_audio_sample_t* arr, size_t len);

static int _process_impl(jack_nframes_t len, void*) {
	if (jack_port_connected(j_port))
	process(jack_port_get_buffer(j_port, len), len);
	return 0;
}

static void _shutdown_impl(void*) {
	perror("Jack Server is shutdowned!");
	mtx_unlock(&j_lock);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// bUFFER
void buffer_check_size(size_t len) {
	mtx_lock(&buff_lock);
	if (buff_len < len) {
		buffer = realloc(buffer, len * sizeof(jack_default_audio_sample_t));
		buff_len = len;
	}
	mtx_unlock(&buff_lock);
}
size_t buffer_append(const jack_default_audio_sample_t* arr, size_t alen) {
	mtx_lock(&buff_lock);
	size_t len = MIN(buff_len - buff_pos, alen);
	memcpy(buffer + buff_pos, arr, len * sizeof(float));
	buff_pos += len;
	mtx_unlock(&buff_lock);
	return len;
}
size_t buffer_remove(jack_default_audio_sample_t* dest, size_t dlen) {
	mtx_lock(&buff_lock);
	ssize_t len = MIN(MIN(buff_pos, dlen), buff_len);
	memcpy(dest, buffer, len * sizeof(float));
	memmove(buffer, buffer + len, (buff_pos - len) * sizeof(float));
	buff_pos -= len;
	mtx_unlock(&buff_lock);
	return len;
}

bool buffer_is_full(void) {
	mtx_lock(&buff_lock);
	bool v = buff_pos >= buff_len - 5;
	mtx_unlock(&buff_lock);
	return v;
}

size_t buffer_write(int fd, size_t dlen) {
	mtx_lock(&buff_lock);
	size_t len = MIN(buff_pos, dlen);
	size_t pos = 0;
	while (pos < len) {
		ssize_t v = write(fd, buffer, (len - pos) * sizeof(float));
		if (v < 0) break;
		pos += v / sizeof(float);
	}
	memmove(buffer, buffer + len, (buff_pos - len) * sizeof(float));
	buff_pos -= len;
	mtx_unlock(&buff_lock);
	return len;
}

size_t buffer_read(int fd, size_t alen) {
	mtx_lock(&buff_lock);
	size_t len = MIN(buff_len - buff_pos, alen);
	size_t pos = 0;
	while (pos < len * sizeof(float)) {
		ssize_t v = read(fd, buffer + buff_pos, len * sizeof(float));
		if (v <= 0) break;
		pos += v / sizeof(float);
	}
	buff_pos = MIN(buff_pos + pos, buff_len);
	mtx_unlock(&buff_lock);
	return len;
}

// JACK INIT AND DESTROY
static void j_release() {
	jack_deactivate(j_client);
	mtx_unlock(&j_lock);
	jack_port_unregister(j_client, j_port);
	mtx_destroy(&j_lock);
	mtx_destroy(&buff_lock);
	free(buffer);
	perror("Goodbye!");
}

void j_connect(const char* cliname, const char* server, int type) {
	jack_status_t stat;
	j_client = jack_client_open(cliname, server ? JackServerName : 0, &stat, server);
	if (!j_client) {
		perror("Can't connect jack server!"); exit(-3);
	};
	mtx_init(&j_lock, 0);
	mtx_init(&buff_lock, 0);
	jack_on_shutdown(j_client, _shutdown_impl, NULL);
	jack_set_process_callback(j_client, _process_impl, NULL);	

	j_port = jack_port_register(j_client, type ? "input" : "output",
			JACK_DEFAULT_AUDIO_TYPE, (type?JackPortIsInput:JackPortIsOutput) |
			JackPortIsTerminal, 0);
	if (!j_port) {
		perror("Can't open JACK port!"); exit(-4);
	}
	atexit(j_release);
	buffer_check_size(jack_get_buffer_size(j_client) * 4);
	jack_activate(j_client);
	mtx_lock(&j_lock);
}

bool j_active() {
	int v = mtx_trylock(&j_lock);
	if (v == 0) mtx_unlock(&j_lock);
	return v != 0;
}

size_t j_data_sample_rate() {
	return jack_get_sample_rate(j_client);
}

size_t j_transfer_sample_rate(size_t* p) {
	if (p) j_sample_rate = *p;
	return j_sample_rate;
}

// UDP
#include <fcntl.h>

int udp_open_server(const short port) {
	struct sockaddr_in info = {0};
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("Can't open socket!");
		exit(-1);
	}
	
	info.sin_family      = AF_INET;
	info.sin_addr.s_addr = DEFAULT_ADDRESS;
	info.sin_port        = port;
	fprintf(stderr, "try bind on address %s\n", inet_ntoa(info.sin_addr));
	if (bind(sock, (const struct sockaddr*)&info, sizeof(info)) != 0) {
		perror("Can't bind socket!"); exit(-2);
	}
	fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
	return sock;
}

int udp_open_client(const in_addr_t ip, const short port) {	
	struct sockaddr_in info;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("Can't open socket!");
		exit(-1);
	}
	
	info.sin_family      = AF_INET;
	info.sin_addr.s_addr = ip;
	info.sin_port        = port;
	if (connect(sock, (struct sockaddr*)&info, sizeof(info)) < 0) {
		perror("Can't connect to server!"); exit(-2);
	}
	fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
	return sock;
}
