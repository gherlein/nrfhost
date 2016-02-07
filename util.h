#ifndef _NRF24_UTIL_H_
#define _NRF24_UTIL_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "nrf24.h"

struct sockdata{
	int sd;
	struct ifreq req;
};

int setmac(struct sockdata * sk, unsigned char *mac, size_t len);
int __set_channel_or_pipes(struct sockdata * sk, int cmd, unsigned char val);
int __get_channel_or_pipes_or_rpd(struct sockdata * sk, int cmd, unsigned char *valp);

#define setchannel(sd,ch) __set_channel_or_pipes(sd,SET_CHANNEL,ch)
#define setpipes(sd,pipes) __set_channel_or_pipes(sd,SET_PIPES,pipes)
#define getchannel(sd,chp) __get_channel_or_pipes_or_rpd(sd,GET_CHANNEL,chp)
#define getpipes(sd,pipesp) __get_channel_or_pipes_or_rpd(sd,GET_PIPES,pipesp)
#define getrpd(sd,rp) __get_channel_or_pipes_or_rpd(sd,GET_RPD,rp)

struct sockdata * create_rawsocket(char *devicename, int protocol);
static inline struct sockdata * nrf_socket(char *devicename){
	return create_rawsocket(devicename,ETH_P_NRF24);
}

int set_if_flags(struct sockdata * sk, short flags);
int get_if_flags(struct sockdata * sk, short * flags);
int set_if_up(struct sockdata * sk);
int set_if_down(struct sockdata * sk);

#endif
