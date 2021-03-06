#include <linux/spi/spidev.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/ioctl.h>


//spidev transfer
int spidev_msg(int fd, const char *txbuf, char * rxbuf, size_t len)
{
	struct spi_ioc_transfer	xfer={
		.tx_buf=(unsigned int)txbuf,
		.rx_buf=(unsigned int)rxbuf,
		.len=len,
		};

	return ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
}

