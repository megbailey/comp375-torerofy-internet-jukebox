package edu.sandiego.comp375.jukebox;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.net.Socket;
import java.util.Scanner;
import java.nio.ByteBuffer;

/**
 * Class representing a client to our Jukebox.
 */
public class AudioClient {
	//java versions of #defines to make
	//the code more legible
	private static final int PLAY = 1;
	private static final int STOP = 2;
	private static final int LIST = 3;
	private static final int INFO = 4;


	/*
	 * Main method the client uses to communicate with the server
	 * @param String[] args, the domain and socket for connection
	 */
	public static void main(String[] args) throws Exception {
		if(args.length < 2){
			System.out.println("Please see usage: ");
			System.exit(1);
		}
		//setting the host name
		String ipAddr = args[0];
		//setting the port number
		int portNum = Integer.parseInt(args[1]);
		//setting up our scanner for commands
		Scanner s = new Scanner(System.in);
		//initializing the buffered in and out streams
		BufferedInputStream in = null;
		BufferedInputStream musicIn = null; 
		BufferedOutputStream out = null;
		//initializing the thread to hold the player
		Thread player = null;
		//initializing our multiple sockets
		Socket socket = null;
		Socket musicSock = null;
		//creating an blank header to write too
		AudioHeader inputHeader = null;
		//letting the client know what's happening
		System.out.println("Client: Connecting to" + ipAddr + " port " + portNum);
		//setting up the connection
		try {
			socket = new Socket(ipAddr, portNum);
			in = new BufferedInputStream(socket.getInputStream(), 2048);
			out = new BufferedOutputStream(socket.getOutputStream(), 1024);
		}
		catch (Exception e){
			System.exit(1);
		}
		//our main loop to catch input from the user
		while (true) {
			System.out.print(">> ");
			String commandLine = s.nextLine();
			String command[] = commandLine.split(" ");

			int option;
			//setting the users commands to an int for the switch
			if (command[0].equals("play")) {
				option = 1;
			}
			else if (command[0].equals("list")) {
				option = 2;
			}
			else if (command[0].equals("info")) {
				option = 3;
			}
			else if (command[0].equals("stop")) {
				option = 4;
			}
			else if (command[0].equals("exit")) {
				option = 5;
			}
			else {
				option = 0;
			}
			//executing the user's choice
			switch (option) {
				//play
				case 1:
					//if the user forgets to include the desired song
					if (command.length < 2){
						System.out.println("Please provide a track to play.");
					}
					else{
						//creating the request header to send to the server
						AudioHeader playHDR = new AudioHeader("PLAY", Integer.parseInt(command[1]));
						if (socket.isConnected()) {

							//closing the current socket to prevent duplicate
							//songs playing
							socket.close();
							socket = new Socket(ipAddr, portNum);
							in = new BufferedInputStream(socket.getInputStream(), 2048);
							out = new BufferedOutputStream(socket.getOutputStream(), 1024);

							//sending the header out using our output function
							playHDR.toOutStream(out);
							//counting and receiving the response from the
							//server
							int count = 0;
							byte header[] = new byte[12];
							for (int i = 0; i < 12; i+=count) {
								count = in.read(header, 0, 12);
							}
							//placing the bytestream information into an
							//AudioHeader object
							inputHeader = AudioHeader.parseInputHeader(header);
							//getting the data_length to see how much data the
							//client should expect
							int data_length = inputHeader.getDataLength(); 
							//If the song isn't available, we receive a list
							//with debug information
							if(inputHeader.getSegType() == LIST){
								byte input[] = new byte[data_length];
								while(in.available() > 0){
									count = in.read(input, 0, data_length);
								}
								//printing the debug information
								String output = new String(input);
								System.out.println(output);
							}
							else if(inputHeader.getSegType() == PLAY){
								//checking if the client is currently playing
								//and closing all of the players
								if (player != null) {
									player.interrupt();
									player.join();
									player = new Thread(new AudioPlayerThread(in));
									player.start();
								}
								else{
									//sending the audio information to the new
									//thread
									player = new Thread(new AudioPlayerThread(in));
									player.start();
								}
							}
						}
					}
					break;
				//If the user is requesting a list
				case 2:
					//creating a new header to send to the server
					AudioHeader listHDR = new AudioHeader("LIST", 0);
					//if we are currently playing we close
					if(player != null){
						socket.close();
						socket = new Socket(ipAddr, portNum);
						in = new BufferedInputStream(socket.getInputStream(), 2048);
						out = new BufferedOutputStream(socket.getOutputStream(), 1024);
						player.interrupt();
						player.join();
						player = null;
					}

					if (socket.isConnected()) {
						//sending the request to the server
						listHDR.toOutStream(out);
						//receiving and parsing the servers response
						int count = 0;
						byte header[] = new byte[12];
						for (int i = 0; i < 12; i+=count) {
							count = in.read(header, 0, 12);
						}
						//placing the servers response into an object
						inputHeader = AudioHeader.parseInputHeader(header);
						//getting the data length to see how long the 
						//text response is
						int data_length = inputHeader.getDataLength(); 
						//getting the response from the server
						byte input[] = new byte[data_length];
						while (in.available() > 0){
							count = in.read(input, 0, data_length);
						}

						if (inputHeader.getSegType() == LIST) {
							//printing out the response from the server
							String output = new String(input);
							System.out.println("(ID) Song Name"); 
							System.out.println(output);
						}
						else {
							System.out.println("Received wrong data file from server");
						}
					}
					break;
				//If the user requests info
				case 3:
					//the user needs a requested ID
					if (command.length < 2){
						System.out.println("Please enter a track to retreive it's information");
					}
					else {
						//creating a header to request information from the
						//server
						AudioHeader infoHDR = new AudioHeader("INFO", Integer.parseInt(command[1]));
						//closing a currently playing thread
						if(player != null){
							socket.close();
							socket = new Socket(ipAddr, portNum);
							in = new BufferedInputStream(socket.getInputStream(), 2048);
							out = new BufferedOutputStream(socket.getOutputStream(), 1024);
							player.interrupt();
							player.join();
							player = null;
						}
						if (socket.isConnected()) {
							//sending out the request to the server
							infoHDR.toOutStream(out);
							//receiving the header from the server
							int count = 0;
							byte header[] = new byte[12];
							for (int i = 0; i < 12; i+=count){
								count = in.read(header, 0, 12);
							}
							//putting the response into an object
							inputHeader = AudioHeader.parseInputHeader(header);
							//reading in the info from the server
							int data_length = inputHeader.getDataLength();
							byte input[] = new byte[data_length];
							while (in.available() > 0){
								count = in.read(input, 0, data_length);
							}

							if (inputHeader.getSegType() == INFO){
								//printing the servers information
								String output = new String(input);
								System.out.println(output);
							}
						}
						else {
							System.out.println("Received wrong data file from server");
						}
					}
					break;
				//If the client wants to stop playing
				case 4:
					//creating the request header
					AudioHeader stopHDR = new AudioHeader("STOP", 0);
					if (socket.isConnected()){
						//closing the socket and opening up new input output
						//streams
						socket.close();
						socket = new Socket(ipAddr, portNum);
						in = new BufferedInputStream(socket.getInputStream(), 2048);
						out = new BufferedOutputStream(socket.getOutputStream(), 1024);
						//sending the request to the server
						stopHDR.toOutStream(out);
						//parsing the servers response
						byte header[] = new byte[12];
						int count = 0;
						for(int i = 0; i < 12; i+=count){
							count = in.read(header, 0, 12);
						}
						//creating an object from the servers response
						inputHeader = AudioHeader.parseInputHeader(header);
						
						if (inputHeader.getSegType() == STOP) {
							//closing the currently playing stream
							if (player != null) {
								System.out.println("In Stop");
								player.interrupt();
								player.join();
								player = null;
							}
							else {
								System.out.println("No track is currently playing");
							}
						}
					}
					break;
				//the user wants to exit the application
				case 5:
					//closing all of the input and outputs as well as the
					//socket
					if (socket.isConnected()){
						if (player != null) {
							player.interrupt();
							player.join();
							player = null;
						}
					}

					socket.close();
					out.close();
					in.close();

					System.out.println("Goodbye!");
					System.exit(0);
					break;
				//if the user inputs an unavailable command
				default:
					System.err.println("ERROR: unknown command");
					System.out.println("Client: Exiting");
					break;
			}
		}
	}
}
