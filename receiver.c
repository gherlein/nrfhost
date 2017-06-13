#include "protocol.h"
#include "util.h"

static struct sockdata *nrf;
static 	char buf[32+1]={PIPE_ADDRESS,
			 0x5b,0x5b,0x5b,0x5b,0x5b,
			 0x3c,0x6d,0x9e,0x5e}; //mac address
static int len;
			 
#define PERR(x) if((x)<0){perror("Error in "#x);return -1;}
#define PER(x) if((x)<0){perror("Error in "#x);}

static inline int _receive(void){ //received in buf with len
	return len=recvfrom(nrf->sd,buf,sizeof(buf),0,NULL,NULL);
}

int main(int argc, char * argv[]){
	short flags;

	nrf=nrf_socket("nrf0");
	if(!nrf){
		perror("Socket open failed for nrf0");
		return -1;
	}
	//set up nrf24 
	PERR(set_if_down(nrf));
	PERR(setmac(nrf,buf,14));
	PERR(setpipes(nrf,0x1));
	PERR(setchannel(nrf,2));
	PERR(set_if_up(nrf));
	
	PERR(get_if_flags(nrf,&flags))
	printf("Flags: %x\n",flags);

	while(1){
		PER(_receive())
		if(len<0)continue;
		if(len==0)printf("EOF received.\n");
		else{
			printf("Packet len %d: {",len);
			for(int i=0;i<len;++i)printf("%x ",buf[i]);
			printf("}\n");
		}
	}
	
	set_if_down(nrf);
	close_rawsocket(nrf);
}
