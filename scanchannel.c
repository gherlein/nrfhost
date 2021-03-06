#include <unistd.h>
#include "util.h"

#define CHECK(x,r,o) if((r=x)<0)goto o

static unsigned char rpd[127];

int scan(char * devname){
	char mac[14]={0x7a,0x7a,0x7a,0x7a,0x7a,
			 0x5b,0x5b,0x5b,0x5b,0x5b,
			 0x3c,0x6d,0x9e,0x5e};
	int res;
	unsigned char ch;
	struct sockdata *sk=nrf_socket(devname);
	if(!sk)return -1;
	printf("Socket created\n");
	CHECK(set_if_down(sk),res,err);
	printf("Interface downed\n");
	CHECK(setmac(sk,mac,sizeof(mac)),res,err);
	printf("Mac set\n");
	CHECK(setpipes(sk,0x1),res, err);
	printf("Pipe set\n");
	CHECK(getpipes(sk,&ch),res,err);
	printf("Pipe gotten\n");
	if(ch!=0x1)printf("set-get pipes not consistent.");
	printf("Scanning channel ");
	for(ch=0;ch<127;++ch){
		CHECK(setchannel(sk,ch),res,err);
		CHECK(set_if_up(sk),res,err);
		sleep(1);
		CHECK(set_if_down(sk),res,err);
		CHECK(getrpd(sk,rpd+ch),res,err);
		printf("%d ",ch);
	}
	printf("\n");
ret:
	close_rawsocket(sk);
	return res;
err:
	perror("Error in scan");
	goto ret;
}

int main(void){
	int i;
	if((i=scan("nrf0"))<0)return -1;
	for(i=0;i<127;++i){
		if(i%16==0)printf("\n%x: ",i);
		printf("%i",rpd[i]);
	}
	printf("\n");
	return 0;
}
	
		
