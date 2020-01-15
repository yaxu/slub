all: start example_client

start : start.o audio.o server.o effects.o Makefile
	cc -lasound -O3 -Wall -pedantic -g -ljack -llo -lsndfile -o start start.o audio.o server.o

example_client : example_client.c
	cc -O3 -g -Wall -pedantic -llo -o example_client example_client.c

start.o : start.c externs.h
	cc -O3 -g -Wall -pedantic -c start.c

audio.o : audio.c audio.h
	cc -O3 -g -Wall -pedantic -c audio.c

server.o : server.c server.h
	cc -O3 -g -Wall -pedantic -c server.c

effects.o : effects.c
	cc -O3 -g -Wall -pedantic -c effects.c

queue.o : queue.c
	cc -O3 -g -Wall -pedantic -c queue.c
