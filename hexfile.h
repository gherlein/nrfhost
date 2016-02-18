#ifndef __HEXFILE_H__
#define __HEXFILE_H__
#include <stdint.h>
#include "nrf24le1.h" 

#define SPI_SPARE 3

struct firmware{
	uint16_t start,end;
	char reset[3];
	uint8_t number;
	char data[SPI_SPARE+FLASH_SZ]; //except the reset vector, 3 bytes for spi transfer
};

int hexfile_read(struct firmware * fw, char *fn);
int firmware_compare(struct firmware *fw1, struct firmware *fw2);

#endif
