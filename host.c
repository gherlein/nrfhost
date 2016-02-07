#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "protocol.h"
#include "nrf24.h"

#define MODEL_NUM 0x1

#define PERR(x) if((x)<0){perror();return -1;}

typedef enum {
	NOTCON,
	CONNECTED,
	UPDATING,
	UPDATED,
	READING,
	COMPLETE,
	ERROR
} state_t;

struct firmware{
	uint16_t start,end;
	uint8_t reset[3];
	uint8_t number;
};
	
static	struct firmware fw;
static 	char buf[32];
static 	state_t state=NOTCON;

void updatestart(void){
	
}

void sendhex(void){
	
}

void sendread(void){
	
}

int main(int argc, char * argv[]){
	int ret;
	bool boot=true;
	//read in hex file, setup fw
	
	int fd=open("/dev/nrf240.0",O_RDWR);
	PERR(fd)
	//set up nrf24 with ioctl
	
	//send init
	buf[0]=CMD_INIT;
	PERR(ret=write(fd,buf,1))
	while(boot){
		PERR(ret=read(fd,buf, sizeof(buf)))
		if(ret==0){ //EOF
			
		}else{
			if(buf[0]==CMD_PING)sendpong();
			else if(buf[0]==CMD_NACK){
				printf("error %i in state %i\n",buf[1],state);
				sendexit();
				state=ERROR;
			}else switch(state){ //CMD_ACK
				case NOTCON:
					if(buf[1]==MODEL_NUM && buf[2]<fw.number){
						state=CONNECTED;
						updatestart();
					}
					break;
				case CONNECTED:
					state=UPDATING;
					writehex(); //one line of hex file
					break;
				
				case UPDATING:
					writehex(); //one line of hex file, after send update complete set state to UPDATED
					break;
				case UPDATED:
					sendread();
					break;
				case READING:
					//read buf for readed flash
					
					sendread(); //if read complete, sendexit(), set state to COMPLETE 
					break;
				case COMPLETE:
					boot=false;
					break;
				case ERROR:
				default:break;
					boot=false;
					break;
			}
		}
	}
}
