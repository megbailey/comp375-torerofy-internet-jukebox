#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

#include <boost/filesystem.hpp>

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <byteswap.h>

#include "ChunkedDataSender.h"
#include "ConnectedClient.h"
namespace fs = boost::filesystem;

using std::cout;
using std::cerr;
using std::vector;
using std::string;

/* Function to respond to the client
 * @param epoll_fd File descriptor for epoll
 * @param data byte array of data from sender
 * @param data_length length of data array
 */
void ConnectedClient::send_response(int epoll_fd, char *data, int data_length) {
	// Create a large array, just to make sure we can send a lot of data in
	// smaller chunks.

	char *data_to_send = new char[data_length];
	memcpy(data_to_send, data, data_length); // 117 is ascii 'u'


	ArraySender *as = new ArraySender(data_to_send, data_length);

	ssize_t num_bytes_sent;
	ssize_t total_bytes_sent = 0;

	// keep sending the next chunk until it says we either didn't send
	// anything (0 return indicates nothing left to send) or until we can't
	// send anymore because of a full socket buffer (-1 return value)
	while((num_bytes_sent = as->send_next_chunk(this->client_fd)) > 0) {
		total_bytes_sent += num_bytes_sent;
	}
	cout << "sent " << total_bytes_sent << " bytes to client\n";

	/*
	 * 1. update our state field to be sending
	 * 2. sent our sender field to be the ArraySender object we created
	 * 3. update epoll so that it also watches for EPOLLOUT for this client
	 *    socket (use epoll_ctl with EPOLL_CTL_MOD).
	 */
	if (num_bytes_sent < 0){
		// Fill this in with the three steps listed in the comment above.
		// Hint: Do NOT delete as here (you'll need it to continue sending
		// 	later).
		struct epoll_event event;
		event.events = EPOLLIN|EPOLLRDHUP|EPOLLOUT;
		//STEP 1:
		this->state = SENDING;
		//STEP 2:
		this->sender = as;
		//STEP 3:
		event.data.fd = this->client_fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, this->client_fd, &event) < 0){
			perror("epoll error in send_dummy\n");
		}
		else {
			// Sent everything with no problem so we are done with our ArraySender
			// object.
		}
	}
}

/* 
 * Function to send the song to the client
 * @param epoll_fd File descriptor for epoll
 * @param pathToSongs vector of song paths
 * @param data_id id of requested song
 */
void ConnectedClient::send_song(int epoll_fd, vector<fs::path> pathToSongs, int data_id){
	try{
		//create a new file sender
		FileSender *fs = new FileSender(pathToSongs.at(data_id));
		//get size of desired file
		uintmax_t file_size = fs::file_size(pathToSongs[data_id]);
		cout << "\nsize of file: " << file_size;

		ssize_t num_bytes_sent;
		ssize_t total_bytes_sent = 0;
		//create an array to store the data to be sent
		char send_data[sizeof(AudioHeader) + file_size];
		//create a new header for our packet
		AudioHeader *hdr_send = (AudioHeader*) send_data;
		//set header details
		hdr_send->type = 1;
		hdr_send->data_length = fs->file_size;
		hdr_send->data_id = data_id;
		//send the header
		this->send_response(epoll_fd, send_data, sizeof(AudioHeader));
		//send the song to the client in chunks
		while ((num_bytes_sent = fs->send_next_chunk(this->client_fd)) > 0){
			total_bytes_sent += num_bytes_sent;
		}

		cout << "IN SEND SONG:\n";
		cout << "send song: " << total_bytes_sent << " bytes to client\n";
		//if we finish sending the song
		if (num_bytes_sent < 0){
			//update event information
			struct epoll_event event;
			event.events = EPOLLIN|EPOLLRDHUP|EPOLLOUT;
			this->state = SENDING;
			this->sender = fs;
			event.data.fd = this->client_fd;
			if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0){
				perror("epoll error in send_song\n");
			}
		}
	}
	catch(const std::out_of_range& e){
		//If the file is not there we send a response to the client
		cout << "Could not find song\n";
		string errorMessage = "Song not available.\n";
		//create array for response message
		char *ret = new char[errorMessage.length() + 1];
		std::strcpy(ret, errorMessage.c_str());
		int len = strlen(ret);
		//create a header for our packet
		char send_data[sizeof(AudioHeader) + len];
		AudioHeader *hdr_send = (AudioHeader*) send_data;
		//set header detail, type is 3 so the client knows it is not the mp3
		hdr_send->type = 3;
		hdr_send->data_length = len;
		hdr_send->data_id = data_id;
		memcpy(hdr_send+1, ret, len);
		//send the message to the client
		this->send_response(epoll_fd, send_data, sizeof(send_data));
	}
}
/*
 * Function to handle the clients request
 * @param epoll_fd File descriptor for epoll
 * @param dir directory of music folder
 * @param pathToMp3 vector of mp3 paths
 * @param pathToInfo vector of info paths
 */
void ConnectedClient::handle_input(int epoll_fd, char *dir, vector<fs::path> pathToMP3, vector<fs::path> pathToInfo) {
	cout << "Ready to read from client " << this->client_fd << "\n";
	//parse header from the client
	char data[12];
	ssize_t bytes_received = recv(this->client_fd, data, 12, 0);
	if (bytes_received < 0) {
		perror("client_read recv");
		exit(EXIT_FAILURE);
	}
	//collect data from the client
	cout << "Received data: ";
	for (int i = 0; i < bytes_received; i++)
		cout << data[i];

	cout << "\n";
	//calling function to handle client's request
	parse_input_and_send(epoll_fd, data, dir, pathToMP3, pathToInfo);
}
/*
 * Function to call other functions to handle the clients request
 * @param epoll_fd File descriptor for epoll
 * @param input array of clients request data
 * @param dir path to music directory
 * @param pathToMP3 vector of mp3 paths
 * @param pathToInfo vector of info paths
 */
void ConnectedClient::parse_input_and_send (int epoll_fd, char *input, char *dir, vector<fs::path> pathToMP3, vector<fs::path> pathToInfo) {
	//create a header object from the byte stream
	AudioHeader* hdr_receive = (AudioHeader*) input;
	//set header object variables into local variables
	int type = hdr_receive->type;
	int data_length = hdr_receive->data_length;
	int data_id = hdr_receive->data_id;
	//notification prints
	cout << "\n";
	cout << "RECV TYPE: " << type << "\n";
	cout << "RECV LENGTH: " << data_length << "\n";
	cout << "RECV ID: " << data_id << "\n";
	cout << "\n";
	//if request is for the song
	if (type == 1) {
		//call send song
		send_song(epoll_fd, pathToMP3, data_id);
	}
	//if request is to stop the song
	else if (type == 2) {
		//call stop song
		stop_song(epoll_fd);
	}
	//if request is for the list
	else if (type == 3) {
		//send the list
		send_list(epoll_fd, dir);
	}
	//if the request is for the info
	else if (type == 4) {
		//send the info
		send_info(epoll_fd, pathToInfo, data_id);
	}
	//handle a non-existant request
	else {
		cout << "Errr.. something went wrong with grabbing the type from input so im gonna quit";
		exit(1);
	}
}
/*
 * Function to respond to the client to acknowledge they ended the song
 * @param epoll_fd File descriptor for epoll
 */
void ConnectedClient::stop_song(int epoll_fd){
	//create a new array to send to the client
	char *rot = new char[sizeof(AudioHeader)+1];
	//cast that array into a header object
	AudioHeader *hdr_send = (AudioHeader*) rot;
	//set the header object
	hdr_send->type = 2;
	hdr_send->data_length = 0;
	hdr_send->data_id = 0;
	//copy the header object to the char array
	memcpy(hdr_send+1, rot, sizeof(AudioHeader));
	//send the ack
	this->send_response(epoll_fd, rot, sizeof(AudioHeader));
}

// You likely should not need to modify this function.
void ConnectedClient::handle_close(int epoll_fd) {
	cout << "Closing connection to client " << this->client_fd << "\n";

	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, this->client_fd, NULL) == -1) {
		perror("handle_close epoll_ctl");
		exit(EXIT_FAILURE);
	}

	close(this->client_fd);
}

/*
 * Function to send the info to the client
 * @param epoll_fd File descriptor for epoll
 * @param pathToInfo vector of info paths
 * @param id id of song to send info about
 */
void ConnectedClient::send_info(int epoll_fd, vector<fs::path> pathToInfo, int id){
	cout << "get and send info rec id: " << id << "\n";
	//try to catch non-existant info request
	try{
		//get the desired path
		fs::path desPath = pathToInfo.at(id);
		//if statement to tell the client they forgot the id for the song
		if ((int) pathToInfo.size() < id) {
			//create the error message
			string errorMessage = "Please enter a valid song id.\n";
			//create an array to store the message
			char *ret = new char[errorMessage.length()+1];
			//copy the message into the array
			std::strcpy(ret, errorMessage.c_str());

			int len = strlen(ret);
			//create an array to send the entire message
			char send_data[sizeof(AudioHeader) + len];
			//create a header
			AudioHeader *hdr_send = (AudioHeader*) send_data;
			//setup the header
			hdr_send->type = 4;
			hdr_send->data_length = len;
			hdr_send->data_id = id;
			memcpy(hdr_send+1, ret, len);
			//send the message
			this->send_response(epoll_fd, send_data, sizeof(send_data));
		}
		else{
			//create a stream to read the info
			std::ifstream inp(desPath.string());
			std::stringstream str;
			//read the info into the string stream
			while (inp >> str.rdbuf());
			//create a string of the info
			string fin = str.str();
			//copy the info into a new array
			char *list = new char[fin.length() + 1];
			std::strcpy(list, fin.c_str());

			int len = fin.length();
			char send_data[sizeof(AudioHeader) + len];
			//setup a header for the info response
			AudioHeader *hdr_send = (AudioHeader*) send_data;
			hdr_send->type = 4;
			hdr_send->data_length = len;
			hdr_send->data_id = id;
			memcpy(hdr_send+1, list, len);

			cout << "SEND TYPE: " << hdr_send->type << "\n";
			cout << "SEND LENGTH: " << hdr_send->data_length << "\n";
			cout << "SEND ID: " << hdr_send->data_id << "\n";
			//send the header and info
			this->send_response(epoll_fd, send_data, sizeof(send_data));
		}
	}
	catch(const std::out_of_range& e){
		//catch non-existant info request
		cout << "Could not find info\n";
		//create an error message
		string errorMessage = "Info not available.\n";
		//copy the message into a char array
		char *ret = new char[errorMessage.length() + 1];
		std::strcpy(ret, errorMessage.c_str());
		int len = strlen(ret);
		//create a new header
		char send_data[sizeof(AudioHeader) + len];
		AudioHeader *hdr_send = (AudioHeader*) send_data;
		//setup the header
		hdr_send->type = 4;
		hdr_send->data_length = len;
		hdr_send->data_id = id;
		memcpy(hdr_send+1, ret, len);
		//send the header
		this->send_response(epoll_fd, send_data, sizeof(send_data));
	}

}
/*
 * Function to send the list of all songs to the client
 * @param epoll_fd File descriptor for epoll
 * @param dir directory of music
 */
void ConnectedClient::send_list(int epoll_fd, char *dir){
	char *list = NULL;
	int counter = 0;
	string filename = "";
	int num_mp3_files = 0;
	int num_info_files = 0;
	//go through all files in dir
	for (fs::directory_iterator entry(dir); entry != fs::directory_iterator(); ++entry, counter++) {
		filename += "(";
		//add mp3 files to response
		if (entry->path().extension() == ".mp3") {
			filename += std::to_string(num_mp3_files);
			num_mp3_files++;
		}
		//add info files to response
		else {
			filename += std::to_string(num_info_files);
			num_info_files++;
		}
		//finish response message
		filename += ") ";
		filename += entry->path().filename().string();
		filename += "\n";
	}

	int list_length = filename.length();
	//convert to char*
	list = new char[filename.length()+1];	
	std::strcpy(list, filename.c_str());
	//create new header
	char send_data[sizeof(AudioHeader) + list_length];
	AudioHeader *hdr_send = (AudioHeader*) send_data;
	//setup header
	hdr_send->type = 3;
	hdr_send->data_length = list_length;
	hdr_send->data_id = 0;

	memcpy(hdr_send+1, list, list_length);

	cout << "SEND TYPE: " << hdr_send->type << "\n"; 
	cout << "SEND LENGTH: " << hdr_send->data_length << "\n"; 
	cout << "SEND ID: " << hdr_send->data_id << "\n";
	//send header and response to client
	this->send_response(epoll_fd, send_data ,sizeof(send_data));
}

/*
 * Function to continue sending data to client in chunks
 * @param epoll_fd File descriptor for epoll
 */
void ConnectedClient::continue_response(int epoll_fd) {
	ssize_t num_bytes_sent = 0;
	ssize_t total_bytes_sent = 0;
	//continue to send data until all bytes are sent
	while ((num_bytes_sent = this->sender->send_next_chunk(this->client_fd)) > 0){
		total_bytes_sent += num_bytes_sent;
	}
	// reset epoll info after sending
	if (num_bytes_sent < 0){
		struct epoll_event event;
		event.events = EPOLLIN | EPOLLRDHUP;
		this->state = RECEIVING;
		event.data.fd = this->client_fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0){
			perror("epoll error in send_song\n");
		}
	}

}
