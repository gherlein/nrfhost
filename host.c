#include <stdint.h>
#include <stdbool.h>

#include "protocol.h"
#include "util.h"
#include "hexfile.h"

#define MODEL_NUM 0x1

static const unsigned char channels[]=CHANNELS;

#define PERR(x) if((x)<0){perror("Error in "#x);return -1;}
#define PER(x) if((x)<0){perror("Error in "#x);}

typedef enum {
	NOTCON,
	CONNECTED,
	UPDATING,
	UPDATED,
	READING,
	ERROR
} state_t;
	
static	struct firmware fw;
static 	state_t state=NOTCON;
static struct sockdata *nrf;
static 	char buf[32+1]={PIPE_ADDRESS,
			 0x5b,0x5b,0x5b,0x5b,0x5b,
			 0x3c,0x6d,0x9e,0x5e}; //mac address
			 
static char len;
static FILE* f;
static	char fbuf[64];
static unsigned short pos;

static inline int _send(void){ //send buf with len
	return write(nrf->sd,buf,len);
}

static inline int _receive(void){ //received in buf with len
	return len=read(nrf->sd,buf,sizeof(buf));
}

static void sendcmd(command_t cmd){
	buf[1]=cmd;
	len=2;
	PER(_send());
}

static void sendupstart(void){
	buf[1]=CMD_UPDATE_START;
	buf[2]= (fw.end-fw.start)>>8;//bytes h
	buf[3]=	(fw.end-fw.start)&0xff;//bytes l
	buf[4]= fw.reset[0];//reset op
	buf[5]= fw.reset[1];//reset addr h
	buf[6]= fw.reset[0];//reset addr l
	buf[7]= fw.number;//firmware number
	buf[8]=~(buf[2]+buf[3]+buf[4]+buf[5]+buf[6]+buf[7])+1; //checksum      sum of buf[2...8] should be 0
	len=9;
	PER(_send());
}

static void sendhex(void){
	unsigned char c_sum,i;
	if(!f){
		printf("Hex file not opened when sending hex.\n");
		exit(1);
	}
	do{
		if(!fgets(fbuf, sizeof(fbuf), f))goto end;
	}while(fbuf[0]!=':'||
			(fbuf[1]=='0'&&fbuf[2]=='3'&&fbuf[3]=='0'&&fbuf[3]=='0'&&fbuf[3]=='0'&&fbuf[3]=='0'&&fbuf[3]=='0'&&fbuf[3]=='0'&&fbuf[3]=='0')); //skip reset vector
	sscanf(fbuf, ":%02hhX%02hhX%02hhX%02hhX", buf+2, buf+3,buf+4,buf+5);
	c_sum = buf[2] + buf[3] + buf[4] + buf[5];
	if(buf[2]>0x10){
		printf("Line in hex file is too long(data bytes > 0x10).\n");
		exit(1);
	}
	for(i=0; i<buf[2]+1; ++i){
		sscanf(fbuf + 9 + i*2, "%02hhX", buf+6+i);
		c_sum += buf[i+6];
	}
	if(c_sum){
		printf("Checksum error when read hex file.\n");
		exit(1);
	}
	if(buf[5]==1 && buf[2]==0)goto end;
		
	buf[1]=CMD_WRITE;
	len=i+6;
	PER(_send());
	return;
end:
	state=UPDATED;
	fclose(f);
	buf[1]=CMD_UPDATE_COMPLETE;
	len=2;
	PER(_send());
}

static void sendread(void){
	buf[1]=CMD_READ;
	buf[2]= fw.end-pos>0x10?0x10:fw.end-pos;//bytes, should <=0x10
	buf[3]= pos>>8;//addr h
	buf[4]= pos&0xff;//addr l
	PER(_send());
}

int main(int argc, char * argv[]){
	int i=0;
	bool boot=true;
	//read in hex file, setup fw
	
	if(argc==3){
		PERR(hexfile_read(&fw,argv[1]));
		f=fopen(argv[1],"r");
		if(!f){
			perror("Reopen hex file failed");
			return -1;
		}
		fw.number=atoi(argv[2]);
	}else{
		printf("Usage: %s hexfilename firmwarenumber/n", argv[0]);
		return 0;
	}
	
	nrf=nrf_socket("nrf0");
	if(!nrf){
		perror("Socket open failed for nrf0");
		return -1;
	}
	//set up nrf24 
	PERR(set_if_down(nrf));
	PERR(setmac(nrf,buf,14));
	PERR(setpipes(nrf,0x1));
	PERR(setchannel(nrf,channels[i]));
	PERR(set_if_up(nrf));
		
	buf[0]=0; //set send pipe nubmer; should be hold all through
	//send init
	sendcmd(CMD_INIT);
	while(boot){
		PERR(_receive());
		if(len==0){ //EOF
			boot=false;
		}else{
			if(buf[1]==CMD_PING)sendcmd(CMD_PONG);
			else if(buf[1]==CMD_NACK){
				printf("error %i in state %i\n",buf[1],state);
				sendcmd(CMD_EXIT);
				state=ERROR;
			}else switch(state){ //CMD_ACK
				case NOTCON:
					if(buf[1]==MODEL_NUM && buf[2]<fw.number){
						state=CONNECTED;
						sendupstart();
					}
					break;
				case CONNECTED:
					state=UPDATING;
					sendhex(); //one line of hex file
					break;
				case UPDATING:
					sendhex(); //one line of hex file, after send update complete set state to UPDATED
					break;
				case UPDATED:
					pos=fw.start;
					sendread();
					break;
				case READING:
					//read buf for flash
					if(memcmp(buf+2,fw.data+SPI_SPARE+pos,fw.end-pos>0x10?0x10:fw.end-pos)){
						printf("Read back verification failed at %#x.\n",pos);
					}
					pos+=fw.end-pos>0x10?0x10:fw.end-pos;
					if(pos>=fw.end){
						sendcmd(CMD_EXIT);
						boot=false;						
					}else sendread(); //if read complete, sendexit(), set state to COMPLETE 
					break;
				case ERROR:
				default:break;
					boot=false;
					break;
			}
		}
	}
	set_if_down(nrf);
	close_rawsocket(nrf);
}
