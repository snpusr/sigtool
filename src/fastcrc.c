/***FastCRC.c*/   
/***Fast CRC code/decode functions*/   
/***Justin Wu*/   
/***2000.7.13*/   
/***NOTE: The type of int must be 32bits weight. ***/   

#include "fastcrc.h"   

#ifdef MG_CRC_32_ARITHMETIC_CCITT   
#define MG_CRC_32_BIT   
#endif   

#ifdef MG_CRC_32_ARITHMETIC   
#define MG_CRC_32_BIT   
#endif   

#ifdef MG_CRC_24_ARITHMETIC_CCITT   
#define MG_CRC_24_BIT   
#endif   

#ifdef MG_CRC_24_ARITHMETIC   
#define MG_CRC_24_BIT   
#endif   

/* 0xbba1b5 = 1'1011'1011'1010'0001'1011'0101b  
   = x24+x23+x21+x20+x19+x17+x16+x15+x13+x8+x7+x5+x4+x2+1 */   
#define MG_CRC_24_CCITT     (0x00ddd0da)
#define MG_CRC_24           (0x00ad85dd)
/* 0x04c11db7 = 1'0000'0100'1100'0001'0001'1101'1011'0111b  
   = x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1 */   
#define MG_CRC_32_CCITT     (0x04c11db7)  
#define MG_CRC_32           (0xedb88320)  

/************************************ Var define ************************************/   
unsigned int MG_CRC_Table[256];   

/********************************** Function define *********************************/   

#define _bswap_32(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
		        (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

void MG_Print_Table(void)
{
	int i;
	unsigned int bend;
	printf("LITTLE ENDIAN | BIG ENDIAN\n");
	for(i = 0; i < 256; i++) {
		bend = _bswap_32(MG_CRC_Table[i]);
		printf("[%03d]: 0x%08x | 0x%08x\n", i, MG_CRC_Table[i], bend);
	}
	
}

unsigned int MG_Compute_CRC24_CCITT(unsigned int crc, unsigned char *bufptr, int len)
{
	register int i;   
	while(len--)  /*Length limited*/   
	{   
		crc ^= ((unsigned int)(*bufptr) << 16);   
		bufptr++;   
		for(i = 0; i < 8; i++)   
		{   
			if(crc & 0x00800000)    /*Highest bit procedure*/   
				crc = (crc << 1) ^ MG_CRC_24_CCITT;   
			else   
				crc <<= 1;   
		}   
	}   
	return(crc & 0x00ffffff);  /*Get lower 24 bits FCS*/   
}

unsigned int MG_Compute_CRC24(unsigned int crc, unsigned char *bufptr, int len)
{
	register int i;
	while(len--)  /*Length limited*/   
	{   
		crc ^= (unsigned int)*bufptr;   
		bufptr++;   
		for(i = 0; i < 8; i++)   
		{   
			if(crc & 1)             /*Lowest bit procedure*/   
				crc = (crc >> 1) ^ MG_CRC_24;   
			else    
				crc >>= 1;   
		}   
	}   
	return(crc & 0x00ffffff);  /*Get lower 24 bits FCS*/   
}

unsigned int MG_Compute_CRC32_CCITT(unsigned int crc, unsigned char *bufptr, int len)
{
	register int i;
	while(len--)  /*Length limited*/   
	{   
		crc ^= (unsigned int)(*bufptr) << 24;   
		bufptr++;   
		for(i = 0; i < 8; i++)   
		{   
			if(crc & 0x80000000)    /*Highest bit procedure*/   
				crc = (crc << 1) ^ MG_CRC_32_CCITT;   
			else   
				crc <<= 1;   
		}   
	}   
	return(crc & 0xffffffff);  /*Get lower 32 bits FCS*/   
}

unsigned int MG_Compute_CRC32(unsigned int crc, unsigned char *bufptr, int len)
{
	register int i;
	while(len--)  /*Length limited*/   
	{   
		crc ^= (unsigned int)*bufptr;   
		bufptr++;   
		for(i = 0; i < 8; i++)   
		{   
			if(crc & 1)             /*Lowest bit procedure*/   
				crc = (crc >> 1) ^ MG_CRC_32;   
			else    
				crc >>= 1;   
		}   
	}   
	return(crc & 0xffffffff);  /*Get lower 32 bits FCS*/   
}   

/*Setup fast CRC compute table*/   
void MG_Setup_CRC_Table(unsigned char type)   
{   
	register int count;   
	unsigned char zero=0;   

	for(count = 0; count <= 255; count++) {
		switch(type) {
			case MG_CRC_24_ARITHMETIC_CCITT:
				MG_CRC_Table[count] = (MG_Compute_CRC24_CCITT(count << 16,&zero,1));
				break;
			case MG_CRC_24_ARITHMETIC:
				MG_CRC_Table[count] = (MG_Compute_CRC24(count,&zero,1));
				break;
			case MG_CRC_32_ARITHMETIC_CCITT:
				MG_CRC_Table[count] = (MG_Compute_CRC32_CCITT(count << 24,&zero,1));   
				break;
			case MG_CRC_32_ARITHMETIC:
				MG_CRC_Table[count] = (MG_Compute_CRC32(count,&zero,1));   
				break;
		}
	}
}   

/*Fast CRC compute*/   
unsigned int MG_Table_Driven_CRC(unsigned int crc, unsigned char *bufptr, int len, unsigned char type)   
{   
	register int i;   
	for(i = 0; i < len; i++) {
		switch(type) {
			case MG_CRC_24_ARITHMETIC_CCITT:
				crc=(MG_CRC_Table[((crc >> 16) & 0xff) ^ bufptr[i]] ^ (crc << 8)) & 0x00ffffff;   
				break;
			case MG_CRC_24_ARITHMETIC:
				crc=(MG_CRC_Table[(crc & 0xff) ^ bufptr[i]] ^ (crc >> 8)) & 0x00ffffff;   
				break;
			case MG_CRC_32_ARITHMETIC_CCITT:
				crc=(MG_CRC_Table[((crc >> 24) & 0xff) ^ bufptr[i]] ^ (crc << 8)) & 0xffffffff;
				break;
			case MG_CRC_32_ARITHMETIC:
				crc=(MG_CRC_Table[(crc & 0xff) ^ bufptr[i]] ^ (crc >> 8)) & 0xffffffff;   
				break;
		}
	}
	return(crc);   
}   

void MG_FCS_Coder(unsigned char *pucInData,int len, unsigned char type)   
{   
	unsigned int iFCS;   

	switch(type) {
		case MG_CRC_24_ARITHMETIC_CCITT:
			iFCS = ~MG_Table_Driven_CRC(0x00ffffff,pucInData,len,type);
			pucInData[len + 2] = (unsigned char)iFCS&0xff;
			pucInData[len + 1] = (unsigned char)(iFCS >>  8) & 0xff;
			pucInData[len]     = (unsigned char)(iFCS >> 16) & 0xff;
			break;
		case MG_CRC_24_ARITHMETIC:
			iFCS = ~MG_Table_Driven_CRC(0x00ffffff,pucInData,len,type);
			pucInData[len]     = (unsigned char)iFCS&0xff;
			pucInData[len + 1] = (unsigned char)(iFCS >>  8) & 0xff;
			pucInData[len + 2] = (unsigned char)(iFCS >> 16) & 0xff;
			break;
		case MG_CRC_32_ARITHMETIC_CCITT:
			iFCS = MG_Table_Driven_CRC(0xffffffff,pucInData,len,type);
			pucInData[len + 3] = (unsigned char)iFCS & 0xff;
			pucInData[len + 2] = (unsigned char)(iFCS >>  8) & 0xff;
			pucInData[len + 1] = (unsigned char)(iFCS >> 16) & 0xff;
			pucInData[len]     = (unsigned char)(iFCS >> 24) & 0xff;
			break;
		case MG_CRC_32_ARITHMETIC:
			iFCS = ~MG_Table_Driven_CRC(0xffffffff,pucInData,len,type);
			pucInData[len]     = (unsigned char)iFCS & 0xff;
			pucInData[len + 1] = (unsigned char)(iFCS >>  8) & 0xff;
			pucInData[len + 2] = (unsigned char)(iFCS >> 16) & 0xff;
			pucInData[len + 3] = (unsigned char)(iFCS >> 24) & 0xff;
			break;
	}   
}

int MG_FCS_Decoder(unsigned char *pucInData,int len, unsigned char type)
{   
	unsigned int iFCS;   

	switch(type) {
		case MG_CRC_24_ARITHMETIC_CCITT:
			pucInData[len - 1] = ~pucInData[len - 1];   
			pucInData[len - 2] = ~pucInData[len - 2];   
			pucInData[len - 3] = ~pucInData[len - 3];   
			if ((iFCS = MG_Table_Driven_CRC(0x00ffffff,pucInData,len,type)) != 0)/*Compute FCS*/   
			{   
				return(-1);                         /* CRC check error */   
			}   
			pucInData[len-1]='\0';   
			pucInData[len-2]='\0';   
			pucInData[len-3]='\0';   
			break;
		case MG_CRC_24_ARITHMETIC:
			pucInData[len - 1] = ~pucInData[len - 1];   
			pucInData[len - 2] = ~pucInData[len - 2];   
			pucInData[len - 3] = ~pucInData[len - 3];   
			if ((iFCS = MG_Table_Driven_CRC(0x00ffffff,pucInData,len,type)) != 0)/*Compute FCS*/   
			{   
				return(-1);                         /* CRC check error */   
			}   
			pucInData[len-1]='\0';   
			pucInData[len-2]='\0';   
			pucInData[len-3]='\0';   
			break;
		case MG_CRC_32_ARITHMETIC_CCITT:
			//pucInData[len - 1] = ~pucInData[len - 1];  
			//pucInData[len - 2] = ~pucInData[len - 2];  
			//pucInData[len - 3] = ~pucInData[len - 3];  
			//pucInData[len - 4] = ~pucInData[len - 4];  
			if ((iFCS = MG_Table_Driven_CRC(0xffffffff,pucInData,len,type)) != 0)/*Compute FCS*/   
			{   
				return(-1);                         /* CRC check error */   
			}   
			//pucInData[len - 1] = '\0';  
			//pucInData[len - 2] = '\0';  
			//pucInData[len - 3] = '\0';  
			//pucInData[len - 4] = '\0';  
			break;
		case MG_CRC_32_ARITHMETIC:
			pucInData[len - 1] = ~pucInData[len - 1];   
			pucInData[len - 2] = ~pucInData[len - 2];   
			pucInData[len - 3] = ~pucInData[len - 3];   
			pucInData[len - 4] = ~pucInData[len - 4];   
			if ((iFCS = MG_Table_Driven_CRC(0xffffffff,pucInData,len,type)) != 0)/*Compute FCS*/   
			{   
				return(-1);                         /* CRC check error */   
			}   
			pucInData[len - 1] = '\0';   
			pucInData[len - 2] = '\0';   
			pucInData[len - 3] = '\0';   
			pucInData[len - 4] = '\0';   
			break;
	}

	return(0);                              /* CRC check OK */   
}   
/***************************************** END ***************************************/   
