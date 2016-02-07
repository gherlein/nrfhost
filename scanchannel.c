#include <unistd.h>
#include <stdio.h>
#include "util.h"

#define CHECK(x,r,o) if((r=x)<0)goto o

static unsigned char rpd[127];

int scan(char * devname){
	unsigned char mac[14]={0x7a,0x7a,0x7a,0x7a,0x7a,
			 0x5b,0x5b,0x5b,0x5b,0x5b,
			 0x3c,0x6d,0x9e,0x5e};
	int ch,res,sd=nrf_socket(devname);
	if(sd<0)return -errno;
	CHECK(set_if_down(devname,sd),res,err);
	CHECK(setmac(sd,mac,sizeof(mac)),res,err);
	CHECK(setpipes(sd,0x1),res, err);
	for(ch=0;ch<127;++ch){
		CHECK(setchannel(sd,ch),res,err);
		CHECK(set_if_up(devname,sd),res,err);
		sleep(2);
		CHECK(getrpd(sd,rpd+ch),res,err);
		CHECK(set_if_down(devname,sd),res,err);
	}
	return res;
err:
	close(sd);
	return res;
}

int main(void){
	int i;
	printf("Start scanning...\n");
	scan("nrf0");
	for(i=0;i<127;++i){
		if(i%16==0)printf("\n%x: ",i);
		printf("%i",rpd[i]);
	}
	printf("\n");
	return 0;
}
	
		
