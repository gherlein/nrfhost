all:	nrfhost nrfprog
CFLAGS=-ggdb -Wall

nrfprog: gpio.c spidev.c hexfile.c nrfprog.c gpio.h spidev.h hexfile.h nrf24le1.h
	gcc ${CFLAGS} -o nrfprog gpio.c spidev.c hexfile.c nrfprog.c

nrfhost: host.c hexfile.c util.c 
	gcc ${CFLAGS} -o nrfhost host.c hexfile.c util.c
#	sudo setcap cap_net_raw+ep nrfhost

scan:	scanchannel.c util.c
	gcc ${CFLAGS} -o scan scanchannel.c util.c
	sudo setcap cap_net_raw+ep scan 

clean:
	rm -f nrfhost scan nrfprog
