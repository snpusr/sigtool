#ifndef	_MG_CRC_H 
#define _MG_CRC_H 

#include <stdio.h>

#define MG_CRC_24_ARITHMETIC_CCITT	1
#define MG_CRC_24_ARITHMETIC		2
#define MG_CRC_32_ARITHMETIC_CCITT	3
#define MG_CRC_32_ARITHMETIC		4

/* 0xbba1b5 = 1'1011'1011'1010'0001'1011'0101b  
   = x24+x23+x21+x20+x19+x17+x16+x15+x13+x8+x7+x5+x4+x2+1 */   
#define MG_CRC_24_CCITT     (0x00ddd0da)
#define MG_CRC_24           (0x00ad85dd)
/* 0x04c11db7 = 1'0000'0100'1100'0001'0001'1101'1011'0111b  
   = x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1 */   
#define MG_CRC_32_CCITT     (0x04c11db7)  
#define MG_CRC_32           (0xedb88320)  

#define bswap_16(x) (((x) & 0x00ff) << 8 | ((x) & 0xff00) >> 8)
#define bswap_32(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
		(((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

extern unsigned int MG_CRC_Table[256]; 

unsigned int MG_Compute_CRC24(unsigned int crc,unsigned char *bufptr,int len); 
unsigned int MG_Compute_CRC24_CCITT(unsigned int crc,unsigned char *bufptr,int len); 
unsigned int MG_Compute_CRC32(unsigned int crc,unsigned char *bufptr,int len); 
unsigned int MG_Compute_CRC32_CCITT(unsigned int crc,unsigned char *bufptr,int len); 
	 
void MG_Setup_CRC_Table(unsigned char type); 
	 
unsigned int MG_Table_Driven_CRC(register unsigned int crc,register unsigned char *bufptr,register int len, unsigned char type); 
	 
void MG_FCS_Coder(unsigned char *pucInData,int len,unsigned char type); 
int MG_FCS_Decoder(unsigned char *pucInData,int len, unsigned char type); 
	 
#endif
