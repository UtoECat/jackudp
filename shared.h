#pragma once
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <jack/jack.h>
#include <stdbool.h>

#define DEFAULT_PORT 5000
#define DEFAULT_ADDRESS htonl(INADDR_LOOPBACK)

/*
 * resample audio data :)
 */
uint64_t Resample_f32(const float *in, float *out, int inrate, int outrate, 
			uint64_t size, uint32_t channels);

/*
 * Connects to jack server in pc.
 * type => where is this program works : on reciever(server)(0) or sender(1) pc
 * this function can no return in case of error :)
 * type also specifies port type : input(1) or output(0)
 */
void j_connect(const char* cliname, const char* server, int type);

/*
 * returns true untill JACK is active
 */
bool j_active();

/*
 * returns JACK server samplerate
 */
size_t j_data_sample_rate();

/*
 * returns transfer samplerate (or sets, if ptr is not NULL)
 */
size_t j_transfer_sample_rate(size_t*);

/*
 * function to process data. user must implement this function.
 * it will be called each time when jack recieves or needs new data.
 * You must not use sockets here! Just copy buffer using threadsafe func!
 */
extern void process(jack_default_audio_sample_t* arr, size_t len);

/*
 * This functions is maked to work with internal threadsafe buffer :)
 * len argument = COUNT of samples!
 */
void buffer_check_size(size_t len);
size_t buffer_append(const jack_default_audio_sample_t*, size_t len);
size_t buffer_remove(jack_default_audio_sample_t* dest, size_t len);
size_t buffer_write(int fd, size_t dlen) ;

// to get ip from string use inet_addr("129.5.24.1")
// if ip is getted from int, it must be casted to INET byteorder!
/*
 * opens and setups client socket.
 * @arg ip address of server. If readed from machine int, must be casted!
 * @arg port. Must be casted to INET byteorder too (see htons())
 */
int  udp_open_client(const in_addr_t ip, const short port);
int  udp_open_server(const short port);

/*
 * Sender MUST convert samplerate of audio data to transfer samplerate!
 */
