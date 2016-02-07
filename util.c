#include "util.h"

int setmac(int sd, unsigned char *mac, size_t len){
	struct ifreq req;
	req.ifr_hwaddr.sa_family=AF_UNSPEC;
	if(len>14)len=14; //max length limited
	memcpy(req.ifr_hwaddr.sa_data, mac, len);
	if(ioctl(sd,SIOCSIFHWADDR,&req)<0)return -errno;
	else return 0;
}

int __set_channel_or_pipes(int sd, int cmd, unsigned char val){
	struct ifreq req;
	req.ifr_flags=val;
	if(ioctl(sd,cmd,&req)<0)return -errno;
	else return 0;
}

int __get_channel_or_pipes_or_rpd(int sd, int cmd, unsigned char *valp){
	struct ifreq req;
	if(ioctl(sd,cmd,&req)<0)return -errno;
	*valp=(unsigned char)req.ifr_flags;
	return 0;
}
//create a raw socket on devicename using protocol
int create_rawsocket(char *devicename, int protocol)
{
	struct sockaddr_ll sll={
		.sll_family = AF_PACKET,
		.sll_protocol = htons(protocol),
	};
	struct ifreq ifr;
	int sd = socket(PF_PACKET, SOCK_RAW,htons(protocol));
	if(sd<0)return -errno;
	/* First Get the Interface Index */	
	strncpy((char *)ifr.ifr_name, devicename, IFNAMSIZ);
	if((ioctl(sd, SIOCGIFINDEX, &ifr)) < 0)goto err;
	/* Bind our raw socket to this interface */
	sll.sll_ifindex = ifr.ifr_ifindex;
	if((bind(sd, (struct sockaddr *)&sll, sizeof(sll))) < 0)goto err;
	return sd;
err:
	close(sd);
	return -errno;
}

int set_if_flags(char *ifname, short flags,int sd)
{
	struct ifreq ifr={
		.ifr_flags=flags,
		};

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if(ioctl(sd, SIOCSIFFLAGS, &ifr)<0)return -errno;
	else return 0;
}

int get_if_flags(char *ifname, short *flags,int sd)
{
	struct ifreq ifr;

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if(ioctl(sd, SIOCGIFFLAGS, &ifr)<0)return -errno;
	*flags=ifr.ifr_flags;
	return 0;
}

int set_if_up(char *ifname, int sd){
	short flags;
	if(get_if_flags(ifname,&flags,sd)<0)return -errno;
    return set_if_flags(ifname, flags | IFF_UP, sd);
}

int set_if_down(char *ifname, int sd){
	short flags;
	if(get_if_flags(ifname,&flags,sd)<0)return -errno;
    return set_if_flags(ifname, flags & ~IFF_UP, sd);
}


