#ifndef CHUNKEDDATASENDER_H
#define CHUNKEDDATASENDER_H
#include <cstddef>
#include <boost/filesystem.hpp>
#include <fstream>

namespace fs = boost::filesystem;

const size_t CHUNK_SIZE = 4096;

/**
 * An interface for sending data in fixed-sized chunks over a network socket.
 * This interface contains one function, send_next_chunk, which should send
 * the next chunk of data.
 */
class ChunkedDataSender {
	public:
		virtual ~ChunkedDataSender() {}

		virtual ssize_t send_next_chunk(int sock_fd) = 0;
};

/**
 * Class that allows sending an array of over a network socket.
 */
class ArraySender : public virtual ChunkedDataSender {
	private:
		char *array;
		size_t array_length;
		size_t curr_loc;

	public:
		/**
		 * Constructor for ArraySender class.
		 */
		ArraySender(const char *array_to_send, size_t length);

		/**
		 * Destructor for ArraySender class.
		 */
		~ArraySender() {
			delete[] array;
		}

		/**
		 * Sends the next chunk of data, starting at the spot in the array right
		 * after the last chunk we sent.
		 *
		 * @param sock_fd Socket which to send the data over.
		 * @return -1 if we couldn't send because of a full socket buffer,
		 * 	otherwise the number of bytes actually sent over the socket.
		 */
		virtual ssize_t send_next_chunk(int sock_fd);
};

/*
 * Class that allows sending a file over a network socket. It does so by
 * sending the large file in chunks.
 */

class FileSender : public virtual ChunkedDataSender{
	public: 
		fs::path song;
        std::ifstream stream;
        char *array;
		size_t file_size;
		size_t array_length;
		size_t curr_loc;

		FileSender(fs::path song);
		~FileSender(){
        }
		virtual ssize_t send_next_chunk(int sock_fd);
};
#endif // CHUNKEDDATASENDER_H
