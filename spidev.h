#ifndef __SPIDEV_H__
#define __SPIDEV_H__
#include <sys/types.h>

extern int spidev_msg(int fd, const char *txbuf, char * rxbuf, size_t len);

static inline int spi_read(int fd, char *buf, size_t len){
	return spidev_msg(fd,buf,buf,len);
}

static inline int spi_write(int fd, char *buf, size_t len){
	return spidev_msg(fd,buf,0,len);
}

#endif
