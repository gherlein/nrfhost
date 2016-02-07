all:	nrfhost
CFLAGS=-ggdb -Wall

nrfprog: host.c util.c 
	gcc ${CFLAGS} -o nrfhost host.c util.c

scan:	scanchannel.c util.c
	gcc ${CFLAGS} -o scan scanchannel.c util.c

clean:
	rm -f nrfhost scan
