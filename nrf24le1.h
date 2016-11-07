#ifndef __NRF24LE1_H_
#define __NRF24LE1_H_


#define BLOCK_SZ		(512)
#define FLASH_SZ		(32*BLOCK_SZ)
#define INFO_SZ		(256)

// SPI Flash operations commands
#define WREN 		0x06	// Set flash write enable latch
#define WRDIS		0x04	// Reset flash write enable latch
#define RDSR		0x05	// Read FLASH Status Register (FSR)
#define WRSR		0x01	// Write FLASH Status Register (FSR)
#define READ		0x03	// Read data from FLASH
#define PROGRAM		0x02	// Write data to FLASH
#define ERASE_PAGE	0x52	// Erase addressed page
#define ERASE_ALL	0x62	// Erase all pages in FLASH main block and
				// infopage
#define RDFPCR		0x89	// Read FLASH Protect Configuration Register
				// FPCR
#define RDISMB		0x85	// Enable FLASH readback protection
#define ENDEBUG		0x86	// Enable HW debug features

// Flash Status Register (FSR) bits
#define FSR_ENDEBUG	(1 << 7)	// Initial value read from byte ENDEBUG 
					// in flash IP
#define FSR_STP		(1 << 6)	// Enable code execution start from
					// protected flash area (page address
					// NUPP)
#define FSR_WEN		(1 << 5)	// Flash write enable latch
#define FSR_RDYN	(1 << 4)	// Flash ready flag, active low
#define FSR_INFEN	(1 << 3)	// Flash IP Enable
#define FSR_RDISMB	(1 << 2)	// Flash MB readback protection enabled,
					// active low

#endif
