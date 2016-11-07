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

//PH5 => 7*32+5=224+5
#define PIN_PROG	229
//PH10 => 7*32+10
#define PIN_RESET	234


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
        if(spi_write(fd, data,2)<0)return -EIO;

		printf("Reading block ");
		for(int i=0;i<len;i+=BLOCK_SZ){
			if(i>0){
				data[0]=data[i];
				data[1]=data[i+1];
				data[2]=data[i+2];
			}
			printf("%u, ",i/BLOCK_SZ);
	        // Do the Read Operation
	        data[i]=READ;
	        data[1+i]=i>>8;
	        data[2+i]=i&0xff;
	        if(spi_read(fd, data+i,(len-i>BLOCK_SZ?BLOCK_SZ:len-i)+3)<0)return -EIO;
	        if(i>0){
				data[i]=data[0];
				data[i+1]=data[1];
				data[i+2]=data[2];
			}
		}
		printf("\n");
		return 0;
}

static int flash_to_file(int fd, int ip, char *fn) {
	char *data;
	int fout,ret=0,rlen=ip?INFO_SZ:FLASH_SZ;

	// Allocate memory
	data = (char *) malloc(sizeof(char) * (rlen+3));
	if(!data)return -ENOMEM;

	CHECKOUTE(ret,read_flash(fd,data,rlen,ip),outm)

	// Open the output file
	CHECKOUTE(ret,fout = open(fn, O_RDWR | O_CREAT, 0666),outm)

	// Write out the data
	ret=write(fout, data+3, rlen);
	printf("Written file %s \n",fn); 
	if(ret<0)ret=-errno;
	else ret=0;
	
	// Close the output file
	close(fout);
outm:
	// Free the memory
	free(data);
	
	return ret;
}

int wait_rdy(int fd){
		int i=0,ret;
		char temp[2]={RDSR,0};
		do{
			++i;
			usleep(50000);
			CHECKOUT(ret,spi_read(fd, temp,2),outrdy)
		}while( (temp[1]&FSR_RDYN) && i<10);		
		if(temp[1]&FSR_RDYN){
			printf("Error erasing page: timeout %u\n",i);
			return -ETIMEDOUT;
		}else return 0;
outrdy:
	return -EIO;
}
	

/* Write a hex file to the device */
static int hexfile_to_flash(int fd, char *fn) {
	struct firmware  *fw, *fwr;
	int ret;
	unsigned short pos;
//	TRACE_MSG;

	fw=(struct firmware*)malloc(sizeof(struct firmware));
	if(!fw)return -ENOMEM;
	fwr=(struct firmware*)malloc(sizeof(struct firmware));
	if(!fwr){
		free(fw);
		return -ENOMEM;
	}

	// Read out the code
	CHECKOUT(ret,read_flash(fd,fwr->data,FLASH_SZ,0),outm)
	memcpy(fwr->reset,fwr->data+SPI_SPARE,sizeof(fwr->reset));
	fwr->start=0;
	fwr->end=FLASH_SZ; 
	
	// Load in the hex file
	CHECKOUT(ret,hexfile_read(fw,fn),outm)
	printf("hexfile read, start: %u, end: %u\n", fw->start, fw->end);
	printf("Reset vector is: 0x%02x%02x%02x\n",fw->reset[0],fw->reset[1],fw->reset[2]);

	// Make sure INFEN is low so we don't erase the IP
	fw->data[0] = WRSR;
	fw->data[1] = 0x00;
	CHECKOUT(ret,spi_write(fd, fw->data,2),outm);

	//copy the reset vector to data for writing
	memcpy(fw->data+SPI_SPARE,fw->reset,sizeof(fw->reset));
	// Write the data by blocks
	for(pos = (fw->start/BLOCK_SZ)*BLOCK_SZ; pos < fw->end; pos += BLOCK_SZ) {
		//compare with the old firmware for difference
		if(!memcmp(fw->data+pos+SPI_SPARE, fwr->data+pos+SPI_SPARE, BLOCK_SZ))continue;
		
		// Set WREN to enable writing
		fw->data[0] = WREN;
		CHECKOUT(ret,spi_write(fd, fw->data,1),outm)

		// Next, we erase the block we're overwriting
		fw->data[0] = ERASE_PAGE;
		fw->data[1] = pos / BLOCK_SZ;
		CHECKOUT(ret,spi_write(fd, fw->data,2),outm)
		printf(" block %u: erasing; ", pos/BLOCK_SZ);
		// Check that we've completed the erase command
		CHECKOUTE(ret,wait_rdy(fd),outm);
		
		// Set WREN to enable writing
		fw->data[0] = WREN;
		CHECKOUT(ret,spi_write(fd, fw->data,1),outm);
// Write the data
		if(pos>2) //temperory save
			memcpy(fw->data,fw->data+pos,SPI_SPARE);
		fw->data[pos]=PROGRAM;
		fw->data[pos+1]=pos>>8;
		fw->data[pos+2]=pos&0xff;
		CHECKOUT(ret,spi_write(fd, fw->data+pos, BLOCK_SZ+SPI_SPARE),outm)
		if(pos>2) //restore
			memcpy(fw->data+pos,fw->data,SPI_SPARE);
		// Check that we've completed the write command
		printf("writing for pos %u\n", pos);
		CHECKOUTE(ret,wait_rdy(fd),outm);
	}

	// Read out the code
	CHECKOUT(ret,read_flash(fd,fwr->data,FLASH_SZ,0),outm)
	memcpy(fwr->reset,fwr->data+SPI_SPARE,sizeof(fwr->reset));
	fwr->start=0;
	fwr->end=FLASH_SZ; 
	
	// Verify that the flash programing was performed correctly
	ret=firmware_compare(fw,fwr);
	
outm:
	free(fw);
	free(fwr);
	
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
		char temp[2];
		temp[0]=RDSR;
		CHECKOUT(ret,spi_read(fd, temp,2),outf);
		printf("FSR: endebug%u,STP%u,WEN%u,RDYN%u,INFEN%u,RDISMB%u\n",temp[1]&0x40,
				temp[1]&020,temp[1]&0x10,temp[1]&0x8,temp[1]&0x4,temp[1]&0x2);
		temp[0]=RDFPCR;
		CHECKOUT(ret,spi_read(fd, temp,2),outf);
		printf("FPCR NUPP: %u\n",temp[1]&0x7f);
	
		printf("Backing up the info page\n");
		CHECKOUT(ret,flash_to_file(fd, 1, "info_page.bin"),outf)
	}
		
outf:
	if(ret<0)perror("Error");
	printf("Closing the spidev\n");
	prog_end();
	close(fd);
out:
	return ret;
}
