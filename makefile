#compiler
CC = g++

CFLAGS = -g -Wall -c --std=c++11

compile : back_server front_server client

back_server: back_server.o
	g++ back_server.o -o back_server -lpthread

front_server: front_server.o
	g++ front_server.o -o front_server -lpthread

client: client.o
	g++ client.o -o client

back_server.o: back_server.cpp
	g++ -c back_server.cpp

front_server.o: front_server.cpp
	g++ -c front_server.cpp

client.o: client.cpp
	g++ -c client.cpp

clean:
	rm -rf *.o compile
