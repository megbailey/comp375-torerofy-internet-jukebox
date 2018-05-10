#ifndef CONNECTEDCLIENT_H
#define CONNECTEDCLIENT_H

enum ClientState { RECEIVING, SENDING };
struct AudioHeader {
    int type;
    int data_length;
    int data_id;
};

class ConnectedClient {
  public:
	int client_fd;
	ChunkedDataSender *sender;
	ClientState state;

	/**
	 * Sends a response to the client.
	 * Note that this is just to demonstrate sending to the client: it doesn't
	 * send anything intelligent.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void send_response(int epoll_fd, char *data_to_send, int data_length);

	/**
	 * Handles new input from the client.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void handle_input(int epoll_fd, char *dir, std::vector<fs::path> pathToMP3, std::vector<fs::path> pathToInfo);

	/**
	 * Handles a close request from the client.
	 *
	 * @param epoll_fd File descriptor for epoll.
	 */
	void handle_close(int epoll_fd);
	/*
	 *Function to send multiple chunks.
	 *@param epoll_fd File descriptor for epoll.
	 */
    void continue_response(int epoll_fd);
	/*
	 * Function to take the senders header and respond
	 * @param epolld_fs File descriptor for epoll
	 * @param input pointer to the senders bytestream
	 * @param dir pointer to the location of the music directory
	 * @param pathToMP3 vector of paths of all MP3s
	 * @param pathToInfo vector of paths of all info files
	 */
    void parse_input_and_send(int epoll_fs, char *input, char *dir, std::vector<fs::path> pathToMP3, std::vector<fs::path> pathToInfo);
	/*
	 * Function to send the list to the client
	 * @param epoll_fd File descriptor for epoll
	 * @param pathToInfo vector to the location of all info files
	 * @param id id of song client called
	 */
    void send_list(int epoll_fd, char *dir); 
	/*
	 *Function to send info to client
	 * @param epoll_fd File descriptor for epoll
	 * @param pathToInfo vector of paths to MP3 files
	 * @param id id of song client is requesting info for
	 */
	void send_info(int epoll_fd, std::vector<fs::path> pathToInfo, int id);
	/* Function to send the song to the client
	 * @param epoll_fd file descriptor for epoll
	 * @param id id of song client is requesting
	 */
	void send_song(int epoll_fd, std::vector<fs::path> pathToMP3, int data_id);
	/* Function to end song client is playing
	 * @param epoll_fd File descriptor for epoll
	 */
	void stop_song(int epoll_fd);
	/* 
	 * Function to create vector of songs and info paths
	 * @param dir the root directory of music files
	 * @param info boolean to tell if we return mp3 or info vectors
	 */
    std::vector<fs::path> find_mp3_or_info_files(char *dir, bool info);	 
};
#endif
