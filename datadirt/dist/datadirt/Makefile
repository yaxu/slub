CFLAGS = -O3 -g -Wall -pedantic
LDFLAGS = -llo -lsndfile -lasound

all: datadirt

distlive: dist
	scp dist/datadirt-`date +%Y%m%d`.tar.gz yaxu.org:/var/www/yaxu/datadirt && ssh yaxu.org 'rm -f /var/www/yaxu/datadirt/datadirt-current.tar.gz && ln -s /var/www/yaxu/datadirt/datadirt-`date +%Y%m%d`.tar.gz /var/www/yaxu/datadirt/datadirt-current.tar.gz' && scp README yaxu.org:/var/www/yaxu/datadirt/README.txt

dist: clean
	tar --exclude=CVS --exclude=dist -c -v -z -f dist/datadirt-`date +%Y%m%d`.tar.gz ../datadirt 

clean :
	rm -f *.o *.gz *~ datadirt

datadirt: datadirt.o audio.o server.o effects.o Makefile
	cc $(CFLAGS) $(LDFLAGS) -o datadirt datadirt.o audio.o server.o

datadirt.o : datadirt.c externs.h
	cc $(CFLAGS) -c datadirt.c

audio.o : audio.c audio.h config.h
	cc $(CFLAGS) -c audio.c

server.o : server.c server.h
	cc $(CFLAGS) -c server.c

effects.o : effects.c
	cc $(CFLAGS) -c effects.c

queue.o : queue.c
	cc $(CFLAGS) -c queue.c
