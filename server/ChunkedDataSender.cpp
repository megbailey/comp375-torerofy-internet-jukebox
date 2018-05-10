#include <algorithm>

#include <cstring>
#include <cerrno>
#include <cstdio>

#include <fstream>
#include <sstream>

#include <boost/filesystem.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "ChunkedDataSender.h"
#include "ConnectedClient.h"
namespace fs = boost::filesystem;

ArraySender::ArraySender(const char *array_to_send, size_t length) {
	this->array = new char[length];
	std::copy(array_to_send, array_to_send+length, this->array);
	this->array_length = length;
	this->curr_loc = 0;
}

/*
 * Constructor for FileSender object. It takes in a path to a song and creates
 * a file sender object by getting the size of the file and initializes the
 * objects data fields.
 */
FileSender::FileSender(fs::path song): stream(song.string()) {
	//setting our song path
    this->song = song;
	//setting the file size
    this->file_size = fs::file_size(song);
    std::cout << "\nsize of file in const: " << this->file_size << "\n";
	//setting the location in the stream
	this->curr_loc = 0;
}

ssize_t ArraySender::send_next_chunk(int sock_fd) {
	size_t num_bytes_remaining = array_length - curr_loc;
	size_t bytes_in_chunk = std::min(num_bytes_remaining, CHUNK_SIZE);
	char chunk[CHUNK_SIZE];
    if (bytes_in_chunk > 0) {
		memcpy(chunk, array+curr_loc, bytes_in_chunk);
		ssize_t num_bytes_sent = send(sock_fd, chunk, bytes_in_chunk, 0);
		if (num_bytes_sent < 0 && errno != EAGAIN) {
			perror("send_next_chunk send");
			exit(EXIT_FAILURE);
		}
		else if (num_bytes_sent > 0) {
			curr_loc += num_bytes_sent;
		}
		return num_bytes_sent;
	}
	else {
        return 0;
	}
}

/*
 *Function called by FileSender to send the song in chunks rather than all at once
 */
ssize_t FileSender::send_next_chunk(int sock_fd) {
    char *chunk = NULL;
	//checking if we've reached the end of the file
    if (!this->stream.eof()) {
		//creating a new chunk
        chunk = new char[CHUNK_SIZE];
		//setting the location of the bytes sent
		this->stream.seekg(curr_loc);
		//getting the data for the next chunk
        this->stream.read(chunk, CHUNK_SIZE);
		ssize_t num_bytes_read = this->stream.gcount();
		//sending the chunk
		ssize_t num_bytes_sent = send(sock_fd, chunk, num_bytes_read, 0);
        //error checking
        if (num_bytes_sent < 0 && errno != EAGAIN) {
			perror("send_next_chunk send");
			exit(EXIT_FAILURE);
		}
		//setting the amount of data sent
		else if (num_bytes_sent > 0) {
		    this->curr_loc += num_bytes_sent;
        }
		//cleaning up
		delete[] chunk;
        return num_bytes_sent;
	}
    return 0;
}
