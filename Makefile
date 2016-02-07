all:	nrfhost
CFLAGS=-ggdb -Wall

nrfprog: host.c util.c 
	gcc ${CFLAGS} -o nrfhost host.c util.c
	sudo setcap cap_net_raw+ep nrfhost

scan:	scanchannel.c util.c
	gcc ${CFLAGS} -o scan scanchannel.c util.c
	sudo setcap cap_net_raw+ep scan 

clean:
	rm -f nrfhost scan
