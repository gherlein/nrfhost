#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include "nrf24le1.h"
#include "gpio.h"
#include "hexfile.h"
#include "spidev.h"

// Debug printing
// #define TRACE_MSG	{printf("%s() [%s:%d] \n", __FUNCTION__, __FILE__, __LINE__);}
//#define TRACE_MSG	{}

#define CHECKOUTE(r,x,out) if((r=x)<0){r=-errno;goto out;}
#define CHECKOUT(r,x,out) if((r=x)<0)goto out;

static int prog_begin()
{
	int ret;
	CHECKOUT(ret,gpio_open(PIN_PROG,1),out)
	CHECKOUT(ret,gpio_open(PIN_RESET,0),out)
	usleep(1); // we need only 0.2 us
	CHECKOUT(ret,gpio_set(PIN_RESET,1),out)
	usleep(1500); // we need to wait at least 1.5 ms before send commands
out:
	return ret;
}

static int prog_end()
{
	int ret;
	CHECKOUT(ret,gpio_set(PIN_PROG, 0),out)
	CHECKOUT(ret,gpio_set(PIN_RESET, 0),out)
	usleep(1);
	CHECKOUT(ret,gpio_set(PIN_RESET, 1),out)
out:
	gpio_close(PIN_PROG);
	gpio_close(PIN_RESET);
	return ret;	
}

/* Read from the device and output a bin file
 * ip == 1 if we want to read out the info page
 */
static int read_flash(int fd, char *data, size_t len, int ip){
        // Set up the read operation FSR
        data[0] = WRSR;
        data[1] = ip?FSR_INFEN:0x00;
        spi_write(fd, data,2);

        // Do the Read Operation
        data[0]=READ;
        data[1]=0;
        data[2]=0;
        return spi_read(fd, data,len+3);
}

static int flash_to_file(int fd, int ip, char *fn) {
	char *data;
	int fout,ret,rlen=ip?INFO_SZ:FLASH_SZ;
//	TRACE_MSG;

	// Allocate memory
	data = (char *) malloc(sizeof(char) * (rlen+3));
	if(!data)return -ENOMEM;

	CHECKOUTE(ret,read_flash(fd,data,rlen,ip),outm)

	// Open the output file
	CHECKOUTE(ret,fout = open(fn, O_RDWR | O_CREAT, 0666),outm)

	// Write out the data
	ret=write(fout, data+3, rlen);
	if(ret<0)ret=-errno;
			
	// Close the output file
	close(fout);
outm:
	// Free the memory
	free(data);
	
	return ret;
}

/* Write a hex file to the device */
static int hexfile_to_flash(int fd, char *fn) {
	struct firmware  *fw;
	char *verify;
	int ret;
	unsigned short pos;
//	TRACE_MSG;

	fw=(struct firmware*)malloc(sizeof(struct firmware));
	if(!fw)return -ENOMEM;
	verify=(char*)malloc(SPI_SPARE+FLASH_SZ);
	if(!verify){
		free(fw);
		return -ENOMEM;
	}

	// Load in the hex file
	CHECKOUT(ret,hexfile_read(fw,fn),outm)
	
	// Make sure INFEN is low so we don't erase the IP
	fw->data[0] = WRSR;
	fw->data[1] = 0x00;
	CHECKOUT(ret,spi_write(fd, fw->data,2),outm);

	// Write the data by blocks
	for(pos = (fw->start/BLOCK_SZ)*BLOCK_SZ; pos < fw->end; pos += BLOCK_SZ) {
		// Set WREN to enable writing
		fw->data[0] = WREN;
		CHECKOUT(ret,spi_write(fd, fw->data,1),outm)

		// Next, we erase the block we're overwriting
		fw->data[0] = ERASE_PAGE;
		fw->data[1] = pos / BLOCK_SZ;
		CHECKOUT(ret,spi_write(fd, fw->data,2),outm)
	
		// Check that we've completed the erase command
		fw->data[0] = RDSR;
		do{
			usleep(500);
			CHECKOUT(ret,spi_write(fd, fw->data,2),outm)
		}while(fw->data[1]&FSR_RDYN);		
		
                // Set WREN to enable writing
                fw->data[0] = WREN;
                CHECKOUT(ret,spi_write(fd, fw->data,1),outm);

		// Write the data
		if(pos>2) //temperory save
			memcpy(fw->data,fw->data+pos,3);
		else  //pos==0, copy the reset vector to data for writing
			memcpy(fw->data+SPI_SPARE,fw->reset,3);
		fw->data[pos]=PROGRAM;
		fw->data[pos+1]=pos>>8;
		fw->data[pos+2]=pos&0xff;
		CHECKOUT(ret,spi_write(fd, fw->data+pos, BLOCK_SZ+3),outm)
		if(pos>2) //restore
			memcpy(fw->data+pos,fw->data,3);
	}

	// Read out the data
	CHECKOUT(ret,read_flash(fd,verify,FLASH_SZ,0),outm) 
	
	// Verfiy that the flash programing was performed correctly
	if(memcmp(fw->data+SPI_SPARE+fw->start, verify+SPI_SPARE+fw->start,fw->end-fw->start)){
		printf("Error: verify firmware data failed!");
		ret=-1;
	}else if(fw->start<BLOCK_SZ && memcpy(fw->reset,verify+SPI_SPARE,3)){
		printf("Error: verify reset vector failed!");
		ret=-1;
	} 

outm:
	free(fw);
	free(verify);
	
	return ret;
}

int main(int argc, char **argv) {
	int fd,ret;
	//TRACE_MSG;
	
	printf("Opening the spidev\n");
	fd = open("/dev/spidev0.0",O_RDWR);
	if(fd<0){
		printf("Open spidev device failed!\n");
		goto out;
	}
		
	ret=prog_begin();
	if(ret<0){
		printf("Initialize gpio failed.");
		goto out;
	}

	// Read/Write the Hex File, if requested
	if(argc > 1) {
		// If we can't open the file, we're reading out the memory contents into it
		if(access(argv[1], R_OK)) {	
			printf("Reading from the device to %s\n", argv[1]);
			CHECKOUT(ret, flash_to_file(fd, 0, argv[1]),outf)
		// Otherwise, write it out
		} else {
			printf("Writing %s to the device\n", argv[1]);
			if(hexfile_to_flash(fd, argv[1]) == 0)
				printf("Write succeeded\n");
		}
	}else{
		printf("Backing up the info page\n");
		CHECKOUT(ret,flash_to_file(fd, 1, "info_page.bin"),outf)
	}
		
outf:
	printf("Closing the spidev\n");
	close(fd);
out:
	prog_end();
	return 0;
}
