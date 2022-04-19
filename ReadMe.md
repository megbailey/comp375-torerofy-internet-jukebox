# Torerofy - Internet Jukebox
The Torerofy (Torero, USD mascott + Spotify) is a multi-threaded C++ Server and Java Client capable of streaming audio using a [custom protocol](/Internet Jutebox Protocol Spec.pdf) and using epoll and non-blocking sockets for multiplexed, asynchronous I/O

The server serves mp3s located in the /music directory. Clients have the ability to list, display info, and stop songs (even mid stream!) using a simple cmdline interface or options.

list: Retrieve a list of songs that are available on the server, along with their ID numbers.

info [song number]: Retrieve information from the server about the song with the specified ID number.

play [song number]: Begin playing the song with the specified ID number. If another song is already playing, the client should switch immediately to the new one.

stop: Stops playing the current song, if there is one playing.

Since C++ and Java use different byte ordering (Little Endian vs Big Endian), bytes are re-ordered before communication.

## Steps to Run (Linux only)

1. Compile & run the server()

```bash
cd server
make
./jukebox-server
```

2. In a separate terminal, compile and run the client

```bash
cd client
mvn package
java -cp "target/*" edu.sandiego.comp375.jukebox.AudioClient
```
