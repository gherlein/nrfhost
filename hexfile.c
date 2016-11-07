#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "hexfile.h"

/* Load a hex file into *data, mark start and end addr */
int hexfile_read(struct firmware * fw, char *fn) {
	FILE * f;
	int ret=0, line = 0, i, done = 0, reset=0;
	unsigned char byte_count, rec_type, checksum, c_sum;
	unsigned short address = 0, address_high = 0;
	char buf[512];
	char dat[256];
	unsigned int addr;

	// Open the file
	f = fopen(fn, "r");
	if(!f)return -errno;

	memset(fw->data, 0xFF, sizeof(fw->data));
	fw->start=-1;fw->end=0;

	// Read in the data from the file and store in the array
	while(!done) {
		// Read in the line
		if(fgets(buf, sizeof(buf), f) == 0)	break;

		// Make sure this record starts with :, otherwise, skip it
		if(buf[0] != ':')continue;

		// Get the byte count, address and record type
		sscanf(buf, ":%02hhX%04hX%02hhX", &byte_count, &address, &rec_type);
		

		c_sum = byte_count + rec_type + (address & 0xFF) + (address >> 8);
		// Get the data on this line
		for(i=0; i<byte_count; ++i){
			sscanf(buf + 9 + i*2, "%02hhX", dat+i);
			c_sum += dat[i];
		}
		// Get the checksum
		sscanf(buf + 9 + (i * 2), "%02hhX", &checksum);
		// check the checksum			
		//c_sum = (0x100 - c_sum) & 0xFF;
		if((c_sum+checksum)&0xff){
			printf("Checksum error on line %i of %s \n", line,fn);
			ret=-2;
			goto out;
		}
		
		switch(rec_type) { // Check based on the record type
		// Data record
		case 0:
			if(!address&& byte_count >= sizeof(fw->reset)){
				memcpy(fw->reset,dat,sizeof(fw->reset)); //reset vector
				reset=1;
			}
			addr=(address_high << 16) + address;
			if(addr < fw->start)fw->start=addr;
			if(addr+byte_count>fw->end)fw->end=addr+byte_count;
			if(fw->end>FLASH_SZ){
				printf("Buffer overflow when loading hex file %s\n",fn);
				ret=-1;
				goto out;
			}
			// Copy the data into the buffer, at the right place
			memcpy(fw->data+SPI_SPARE+addr, dat, byte_count);
			break;
		// EOF Record
		case 1:
			// At EOF, we're done
			if(byte_count == 0)
				done = 1;
			break;
		// Extended Segment Address Record
		case 2:
			printf("FIXME: ESA\n");
			break;
		// Start Segment Address Record
		case 3:
			printf("FIXME: SSA\n");
			break;
		// Extended Linear Address Record
		case 4:
			// This is the new high order bits
			address_high = (dat[0] << 8) & dat[1];
			break;
		// Start Linear Address Record
		case 5:
			printf("FIXME: SLA\n");
			break;
		}
		line++;
	}
	if(!done)printf("warning: no EOF record in the hex file: %s",fn);
	if(!reset){
		printf("Fatal: NO RESET VECTOR FOUND!");
		ret=-1;
	}
out:
	// Complete
	fclose(f);
	return ret;
}

int firmware_compare(struct firmware *fw1, struct firmware *fw2){
	int i, flag=0;
	if(fw1->number!=fw2->number){
		printf("firmware numbers differ: %i != %i\n",fw1->number,fw2->number);
	}
	if(fw1->start!=fw2->start){
		printf("firmware starts differ: %i != %i\n",fw1->start,fw2->start);
	}
	if(fw1->end!=fw2->end){
		printf("firmware ends differ: %i != %i\n",fw1->end,fw2->end);
	}
	if(fw1->reset[0]!=fw2->reset[0]||fw1->reset[1]!=fw2->reset[1]||fw1->reset[2]!=fw2->reset[2]){
		printf("firmware reset vectors differ: %#x%x%x != %#x%x%x\n",fw1->reset[0],fw1->reset[1],fw1->reset[2],fw2->reset[0],fw2->reset[1],fw2->reset[2]);
	}
	printf("Comparing firmware body...\n");
	for(i=fw1->start+SPI_SPARE;i<(fw1->end<fw2->end?fw1->end:fw2->end);++i){
		if(fw1->data[i]!=fw2->data[i]){
			printf("%#x:%#x!=%#x; ",i-SPI_SPARE,fw1->data[i],fw2->data[i]);
			++flag;
		}
	}
	printf("Compare complete.\n");
	return flag;
}
