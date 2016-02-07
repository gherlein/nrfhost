#ifndef _NRF24_UTIL_H_
#define _NRF24_UTIL_H_

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "nrf24.h"

int setmac(int sd, unsigned char *mac, size_t len);
int __set_channel_or_pipes(int sd, int cmd, unsigned char val);
int __get_channel_or_pipes_or_rpd(int sd, int cmd, unsigned char *valp);

#define setchannel(sd,ch) __set_channel_or_pipes(sd,SET_CHANNEL,ch)
#define setpipes(sd,pipes) __set_channel_or_pipes(sd,SET_PIPES,pipes)
#define getchannel(sd,chp) __get_channel_or_pipes_or_rpd(sd,GET_CHANNEL,chp)
#define getpipes(sd,pipesp) __get_channel_or_pipes_or_rpd(sd,GET_PIPES,pipesp)
#define getrpd(sd,rp) __get_channel_or_pipes_or_rpd(sd,GET_RPD,rp)

int create_rawsocket(char *devicename, int protocol);
static inline int nrf_socket(char *devicename){
	return create_rawsocket(devicename,ETH_P_NRF24);
}

int set_if_flags(char *ifname, short flags,int sd);
int get_if_flags(char *ifname, short *flags,int sd);
int set_if_up(char *ifname, int sd);
int set_if_down(char *ifname, int sd);

#endif
