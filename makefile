all: server client server2

server: server.cpp queue.h
	g++ -o server server.cpp -pthread

client: client.cpp
	g++ -o client client.cpp

server2: server2.cpp queue.h
	g++ -o server2 server2.cpp -pthread