#ifndef FW_H
#define FW_H

#define FILE_TYPE_RAW		0
#define FILE_TYPE_COFF		1
#define FILE_TYPE_ELF		2
#define FILE_TYPE_PE		3

#define FILE_COMP_LZMA		0x10000000
#define FILE_COMP_GZIP		0x01000000
#define FILE_COMP_BZIP		0x00100000
#define FILE_COMP_ZLIB		0x00010000

#define FILE_ENC_IDEA		0x80000000
#define FILE_ENC_TEA		0x08000000
#define FILE_ENC_XTEA		0x00800000
#define FILE_ENC_DES		0x00080000
#define FILE_ENC_3DES		0x00008000

#define FILE_ENDIAN_LITTLE	0
#define FILE_ENDIAN_BIG		1
#define FILE_ENDIAN_DEFAULT	FILE_ENDIAN_LITTLE

typedef struct data {
	unsigned char		*data;		// view data
	unsigned int		size;		// view size
	unsigned int		flags;
} data_t;

typedef struct file {
	char				*name;		// file name

	unsigned char *		rdata;		// file raw data
	unsigned int		rsize;		// file raw data size

	unsigned int		flags;		// file flags
	unsigned int		type;		// file tyoe
} file_t;

typedef struct asmview {
	unsigned char		align;		// view size (1, 2, 4, 8, 16, 32)
	unsigned char		ascii;		// view ascii data 

	unsigned char		*data;		// view data aligned by valign;
	unsigned int		size;			// view size (if vascii is true, vsize = vsize + valign)
	unsigned int		lines;		
} asmview_t;

#define SYM_TYPE_LOCAL		0
#define SYM_TYPE_GLOBAL		1
#define SYM_TYPE_UNKNOWN	2
typedef struct sym {
	unsigned char		symid;
	unsigned char		*dptr;
	unsigned char		align;
	char				*name;
	unsigned int		paddr, 
						vaddr,
						offset,
						size;
} symbol_t;

#define VAR_TYPE_LOCAL		0
#define VAR_TYPE_GLOBAL		1
#define VAR_TYPE_UNKNOWN	2
typedef struct variable {
	unsigned char		varid;		// variable unique id
	unsigned char		align;		// alignement
	unsigned char		type;		// variable type (int,char,long,double,float,etc)
	unsigned int		owner;		// variable owner
	char				*name;		// variable name
	void				*vptr;		// variable data pointer into file binary image
} variable_t;


#endif
