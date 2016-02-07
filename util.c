#include "util.h"

int setmac(struct sockdata * sk, unsigned char *mac, size_t len){
	sk->req.ifr_hwaddr.sa_family=AF_UNSPEC;
	if(len>14)len=14; //max length limited
	memcpy(sk->req.ifr_hwaddr.sa_data, mac, len);
	if(ioctl(sk->sd,SIOCSIFHWADDR,&sk->req)<0)return -errno;
	else return 0;
}

int __set_channel_or_pipes(struct sockdata * sk, int cmd, unsigned char val){
	sk->req.ifr_flags=val;
	if(ioctl(sk->sd,cmd,&sk->req)<0)return -errno;
	else return 0;
}

int __get_channel_or_pipes_or_rpd(struct sockdata * sk, int cmd, unsigned char *valp){
	if(ioctl(sk->sd,cmd,&sk->req)<0)return -errno;
	*valp=(unsigned char)sk->req.ifr_flags;
	return 0;
}
//create a raw socket on devicename using protocol, and set ifr_name of the ifreq
struct sockdata * create_rawsocket(char *devicename, int protocol)
{
	struct sockaddr_ll sll={
		.sll_family = AF_PACKET,
		.sll_protocol = htons(protocol),
	};
	struct sockdata *sk=malloc(sizeof(struct sockdata));
	if(!sk)return sk;
	sk->sd = socket(PF_PACKET, SOCK_RAW,htons(protocol));
	if(sk->sd<0)goto serr;
	/* First Get the Interface Index */	
	strncpy((char *)sk->req.ifr_name, devicename, IFNAMSIZ);
	if((ioctl(sk->sd, SIOCGIFINDEX, &sk->req)) < 0)goto err;
	/* Bind our raw socket to this interface */
	sll.sll_ifindex = sk->req.ifr_ifindex;
	if((bind(sk->sd, (struct sockaddr *)&sll, sizeof(sll))) < 0)goto err;
	return sk;
err:
	close(sk->sd);
serr:
	perror("Error in create_rawsocket: ");
	free(sk);
	return NULL;
}

int set_if_flags(struct sockdata * sk, short flags)
{
	if(ioctl(sk->sd, SIOCSIFFLAGS, &sk->req)<0)return -errno;
	else return 0;
}

int get_if_flags(struct sockdata * sk, short * flags)
{
	if(ioctl(sk->sd, SIOCGIFFLAGS, &sk->req)<0)return -errno;
	*flags=sk->req.ifr_flags;
	return 0;
}

int set_if_up(struct sockdata * sk){
	short flags;
	if(get_if_flags(sk,&flags)<0)return -errno;
    return set_if_flags(sk, flags | IFF_UP);
}

int set_if_down(struct sockdata * sk){
	short flags;
	if(get_if_flags(sk,&flags)<0)return -errno;
    return set_if_flags(sk, flags & ~IFF_UP);
}


