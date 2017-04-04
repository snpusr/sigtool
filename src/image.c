#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <math.h>
#include "fastcrc.h"
#include "libasm.h"
#include "elf.h"
#include "list.h"
#include "db.h"

#define ELF_MAGIC "\x7f\x45\x4c\x46"
#define SIG_MAGIC	0xdeadbeef
#define BB_THRESHOLD	2

asm_processor 		proc;
asm_instr			instr;

typedef struct basic_block basic_block_t;

typedef struct basic_block {
	char			fname[64];
	unsigned int	addr, vaddr;
	unsigned int	size, off;
	unsigned int	icnt, ilen;
	unsigned int 	nopcnt;
	unsigned char	*data;
} bb_t;

typedef struct bin_image {
	char				*name;
	unsigned int		size;
	unsigned char		*data;
	unsigned int		crc;
	unsigned int		hashl, 
						hashb;
} bin_image_t;

typedef struct elf_symbol {
	char				name[64];	// symbol name
	unsigned int		addr;		// symbol address
	unsigned int		size;		// symbol size
	unsigned short		index;		// symbol index into strtable
	unsigned char		type;		// symbol type
	unsigned char		binding;	// symbol binding
	unsigned int		crc;		// symbol data checksum
	unsigned int		code;		// mimimi
	double				entropy, 	// symbol bit entropy
						probability;// symbol probability value when searching similar symbols
	bb_list_t			*blist;		// basic block list

	struct elf_symbol	*next, 
						*prev;
} elf_sym_t;

typedef struct elf_image {
	char			*name;			// image filename
	unsigned int	size;
	unsigned char	*data;			

	Elf32_Ehdr		*ehdr;			// pointer to image elf header

	unsigned int	nsections;		// total number of elf sections

	Elf32_Shdr		*shstrtabhdr;	// pointer to section string table section
	unsigned char	*shstrtab;		// pointer to section string table data
	unsigned int	shstrtablen;	// length of section string table data

	Elf32_Shdr		*strtabhdr;		// pointer to string table header
	unsigned char	*strtab;		// pointer to string table data
	unsigned int 	strtablen;		// length of string table data

	Elf32_Shdr		*symtabhdr;		// pointer to section symbol table header
	unsigned char	*symtab;		// pointer to section symbol table data
	unsigned int 	symtablen;		// length of section symbol table data

	Elf32_Shdr		*dyntabhdr;		// pointer to section dynamic symbol table header
	unsigned char	*dyntab;		// pointer to section dynamic symbol table data
	unsigned int	dyntablen;		// lenght of section dynamic symbol table data

	Elf32_Sym		*lastsym;		
	elf_sym_t		*symlist;

	unsigned int	crc;
} elf_image_t;

static int ELF32_ENDIAN = 0;

void elf_disassemble_symbol(elf_image_t *, elf_sym_t *);

unsigned char h_table[256];

void hexdump(unsigned int start_addr, unsigned char *data, unsigned int dlen)
{
	int     i, c;
	char    asc[18];

	memset(asc, 0x00, sizeof(asc)-1);
	printf("%08X: ", start_addr);
	for(c = i = 0; i < dlen; i++) {
		if(i && !(i % 16)) {
			printf("| %s\n%08X: ", asc, start_addr);
			c = 0;
		}
		if((data[i] > 0x20 && data[i] <= 0x7F))
			asc[c++] = data[i];
		else asc[c++] = '.';
		printf("%02x ", data[i]);
	}
	for(asc[c] = 0, c = 0; (i%16) && c < ((16 - (i%16))); c++)
		printf("   ");
	if(c) printf("| %-16s\n", asc);
	putc(0x0A, stdout);
}

/* 8 bit */
double bit_entropy(unsigned char *data, unsigned int len, double *prb)
{
	int 	i;
	int		total = 0;
	double 	prob, entropy = 0, p = 0;
	memset(h_table, 0x00, sizeof(h_table));
    /* build histogram table */
	for(i = 0; i < len; h_table[data[i++] & 0xFF]++);
	for(i = 0; i < 256; i++)
		if(h_table[i])
			total += h_table[i];
    /* calculate byte probability 
     * and entropy */
	for(i = 0; i < 256; i++) {
		if(h_table[i]) {
			prob = 1.0 * h_table[i] / total;
			p += prob;
			entropy += (prob * log2(1.0 / prob));
		}
	}
	p /= (1.0 * total);
	*prb = p;
	return (entropy);
}

void elf_print_ehdr(elf_image_t *image)
{
	int i;

	printf("MAGIC: ");
	for(i = 0; i < 16; i++)
		printf("%02x ", image->ehdr->e_ident[i]);
	printf("\n");

	printf("e_type:\t%08x\n", image->ehdr->e_type);
	printf("byte order:\t%s\n", image->ehdr->e_ident[EI_DATA] == ELFDATA2MSB ? "BIG ENDIAN" : "LITTLE ENDIAN"); 
	printf("e_machine:\t%08x\n", image->ehdr->e_machine);
	printf("e_version:\t%08x\n", image->ehdr->e_version);
	printf("e_entry:\t %08x\n", image->ehdr->e_entry);
	printf("e_phoff:\t %08x (%d)\n", image->ehdr->e_phoff, image->ehdr->e_phoff);
	printf("e_shoff:\t %08x (%d)\n", (image->ehdr->e_shoff), (image->ehdr->e_shoff));
	printf("e_flags:\t %08x\n", image->ehdr->e_flags);
	printf("e_shnum:\t %08x\n", image->ehdr->e_shnum);
	printf("e_phnum:\t %08x\n", image->ehdr->e_phnum);
}

char *elf_section_name(unsigned char *data, unsigned int datalen, unsigned int sh_name)
{
	if(sh_name == 0)
		return ("NULL");
	if(sh_name > datalen)
		return ("NULL");
	//printf("%p %x\n", data, sh_name);
	return ((char *)(data + sh_name));
}

unsigned int elf_byteswap(int bits, unsigned int value)
{
	if(!ELF32_ENDIAN)
		return (value);

	switch(bits) {
		case 16:
			return (bswap_16(value));
		case 32:
			return (bswap_32(value));
		default:
			return (0);
	}
}

int elf_load_strtab(elf_image_t *image)
{
	Elf32_Shdr 		*shdr;
	char			*section_name;
	unsigned char	*data;
	long            datalen = 0;
	int				i, loaded = 0;

	for(i = 0; i < image->nsections; i++) {
		shdr = (Elf32_Shdr *)(image->data + image->ehdr->e_shoff + (i * image->ehdr->e_shentsize));

		shdr->sh_type 		= elf_byteswap(32, shdr->sh_type);
		shdr->sh_addralign 	= elf_byteswap(32, shdr->sh_addralign);
		shdr->sh_link 		= elf_byteswap(32, shdr->sh_link);
		shdr->sh_addr 		= elf_byteswap(32, shdr->sh_addr);
		shdr->sh_size 		= elf_byteswap(32, shdr->sh_size);
		shdr->sh_offset 	= elf_byteswap(32, shdr->sh_offset);
		shdr->sh_name 		= elf_byteswap(32, shdr->sh_name);
		shdr->sh_entsize 	= elf_byteswap(32, shdr->sh_entsize);
		shdr->sh_flags		= elf_byteswap(32, shdr->sh_flags);

		if(shdr->sh_type != SHT_STRTAB && shdr->sh_type != SHT_PROGBITS)
			continue;

		data = (image->data + shdr->sh_offset);
		datalen = shdr->sh_size * (shdr->sh_entsize ? shdr->sh_entsize : 1);

		if(datalen < 0)
			continue;

		section_name = elf_section_name(image->shstrtab ? image->shstrtab : data, datalen, shdr->sh_name);

		if(!strcmp(section_name, ".strtab")) {
			image->strtab = (data);
			image->strtablen = datalen;
			image->strtabhdr = shdr;
			loaded++;
			printf("[+] loaded '%s' @ %08x (%ld bytes/%d entities)\n", section_name, shdr->sh_offset, datalen, shdr->sh_entsize);
		} else if(!strcmp(section_name, ".shstrtab")) {
			image->shstrtab 	= (data);
			image->shstrtablen 	= datalen;
			image->shstrtabhdr 	= shdr;
			section_name 		= elf_section_name(data, datalen, shdr->sh_name);
			loaded++;
			printf("[+] loaded '%s' @ %08x (%ld bytes/%d entities)\n", section_name, shdr->sh_offset, datalen, shdr->sh_entsize);
		}
	}
	return (loaded);
}

int elf_load_symtab(elf_image_t *image)
{
	Elf32_Shdr	*shdr;
	int			i;

	for(i = 0; i < image->nsections; i++) {
		shdr = (Elf32_Shdr *)(image->data + image->ehdr->e_shoff + (i * image->ehdr->e_shentsize));

		if(shdr->sh_size > 0x80000000) {
			shdr->sh_type 		= elf_byteswap(32, shdr->sh_type);
			shdr->sh_type 		= elf_byteswap(32, shdr->sh_type);
			shdr->sh_addralign 	= elf_byteswap(32, shdr->sh_addralign);
			shdr->sh_link 		= elf_byteswap(32, shdr->sh_link);
			shdr->sh_addr 		= elf_byteswap(32, shdr->sh_addr);
			shdr->sh_size 		= elf_byteswap(32, shdr->sh_size);
			shdr->sh_offset 	= elf_byteswap(32, shdr->sh_offset);
			shdr->sh_name 		= elf_byteswap(32, shdr->sh_name);
			shdr->sh_entsize 	= elf_byteswap(32, shdr->sh_entsize);
			shdr->sh_flags		= elf_byteswap(32, shdr->sh_flags);
		}
		if(shdr->sh_type != SHT_SYMTAB)
			continue;

		image->symtab = (image->data + shdr->sh_offset);
		image->symtablen = shdr->sh_size;
		image->symtabhdr = shdr;

		printf("[+] loaded '%s' @ %08x (%d bytes/%d entities)\n", elf_section_name(image->shstrtab, image->shstrtablen, shdr->sh_name), shdr->sh_offset, image->symtablen, shdr->sh_entsize);
		return (1);
	}
	return (0);
}

char *elf_symbol_name(unsigned char *data, unsigned int datalen, unsigned int st_name)
{
	if(st_name == 0)
		return ("NULL");
	if(st_name > datalen)
		return ("NULL");
	return ((char *)data + st_name);
}

char *elf_type_str(elf_sym_t *sym)
{
	int type = sym->type;
	int bind = sym->binding;

	switch(type) {
		case STT_FUNC:
			if(bind == STB_LOCAL)
				return ("FUNCTION LOCAL");
			else return ("FUNCTION GLOBAL");
			break;
		case STT_OBJECT:
			if(bind == STB_LOCAL)
				return ("OBJECT LOCAL");
			else 
				return ("OBJECT GLOBAL");
			break;
		case STT_NOTYPE:
			if(bind == STB_LOCAL)
				return ("NOTYPE LOCAL");
			else
				return ("NOTYPE GLOBAL");
			break;
		case STT_SECTION:
			if(bind == STB_LOCAL)
				return ("SECTION LOCAL");
			else 
				return ("SECTION GLOBAL");
			break;
		case STT_FILE:
			if(bind == STB_LOCAL)
				return ("FILE LOCAL");
			else
				return ("FILE GLOBAL");
			break;
		default:
			break;
	}
	return ("NULL");
}

char *instr_type(int type)
{
	switch(type) {
		case ASM_TYPE_BRANCH:
		case ASM_TYPE_CALLPROC:
		case ASM_TYPE_CONDCONTROL:
		case ASM_TYPE_INDCONTROL:
			return ("flow change");
		case ASM_TYPE_RETPROC:
			return ("ret");
		case ASM_TYPE_INT:
			return ("int");
		default:
			return ("");
	}
	return ("NULL");
}

/* tosco crc */
unsigned int elf_symbol_opcode_crc(elf_image_t *image, elf_sym_t *sym)
{
	int			 	i;
	unsigned int 	crc = 0, opcode, offset;
	unsigned char 	*ptr;

	Elf32_Shdr		*shdr;

	if((shdr = (Elf32_Shdr *)(image->data + (image->ehdr->e_shoff + (image->ehdr->e_shentsize * sym->index)))) != NULL) {
		offset = (shdr->sh_offset + sym->addr);
		ptr = (image->data + offset);
	}
	for(i = 0; i < sym->size; i++) {
		opcode = *(unsigned int *)(ptr + i);
		crc += ((opcode >> 16) & 0xFFFF);
	}
	return (crc);
}

int basic_block_count(bb_list_t **blist)
{
	bb_list_node_t	*p;
	int				count = 0;

	if((*blist) == NULL)
		return (0);

	for(count = 0, p = (*blist)->head; p; p = p->next, count++);

	return (count);
}

#define ASM_BLOCK_STOP(x) \
	((x & ASM_TYPE_BRANCH) || (x & ASM_TYPE_CALLPROC) || (x & ASM_TYPE_CONDCONTROL) || \
	 (x & ASM_TYPE_INT) || (x & ASM_TYPE_RETPROC) || (x & ASM_TYPE_COMPARISON) || (x & ASM_TYPE_INDCONTROL))

/* build basic block list from elf image */
int elf_build_basicblock_list(bb_list_t **bblist, elf_image_t *image, int sig, char *sigfile)
{
	unsigned int 	curr, len, vaddr = 0;
	char 		*att_dump;
	unsigned char 	*ptr, *bdata;
	unsigned int 	offset;

	elf_sym_t		*sym;
	Elf32_Shdr		*shdr;
	basic_block_t	*bb;
	bb_list_node_t	*bnode;
	int 			bstart, bstop, bsize, binst, binstlen = 0, nopcnt = 0;
	db_t			db;

	int bcount = 0;
	for(sym = image->symlist; sym; sym = sym->next) {
		sym->blist = NULL;
		shdr = (Elf32_Shdr *)(image->data + image->ehdr->e_shoff + (image->ehdr->e_shentsize * sym->index));
		offset = (shdr->sh_offset + sym->addr);
		if(offset > image->size)
			break;
		ptr = (image->data + offset);
		vaddr = sym->addr;
		len = (sym->size == 0) ? 65535 : sym->size;
		curr = 0;
		bstart = bstop = bsize = binst = 0;
		bdata = NULL;
		bcount = 0;

		while(curr < len) {
			if (asm_read_instr(&instr, ptr + curr, len - curr, &proc) > 0) {
				if(ASM_BLOCK_STOP(instr.type)) {
					bstop = 1;
					//printf("STOPING %s at %d\n", sym->name, curr);
				} else bstart=1;

				att_dump = asm_display_instr_att(&instr, (int)vaddr + curr);
				//if(bstop) printf("%s\n", att_dump);
				if (att_dump && (strcmp(att_dump,"int_err"))) {
					if(!strncmp(att_dump, "nop", 3))
						nopcnt++;
					curr += asm_instr_len(&instr);	
					binst++;
					binstlen += asm_instr_len(&instr);
					if(bstop) {
						bdata = (unsigned char *)realloc(bdata, bsize + strlen(att_dump) + 2);
						sprintf((char *)(bdata + bsize), "%s\n", att_dump);
						bsize += strlen(att_dump) + 1;

						if((bb = (basic_block_t *)malloc(sizeof(basic_block_t))) == NULL)
							exit(0);

						bb->addr 	= (int)(vaddr + curr);
						bb->vaddr 	= (int)(vaddr + offset + curr) - binstlen;
						bb->data 	= (unsigned char *)strdup((char *)bdata);
						bb->size 	= curr; //sym->size;
						bb->icnt 	= binst;
						bb->nopcnt 	= nopcnt;
						bb->ilen 	= binstlen;
						bb->off		= offset + curr;
						snprintf(bb->fname, sizeof(bb->fname), "%s", sym->name);
		//				printf("adding %s...\n", sym->name);

						bcount++;

						bb_list_add(&sym->blist, bb_list_node_alloc(bb));

						free(bdata);
						bstop = 0;
						bstart = 1;
						bsize = 0;
						nopcnt = binstlen = binst = 0;
						bdata = NULL;
						continue;
					}
					if(bstart) {
						if(bdata == NULL) {
							if((bdata = (unsigned char *)malloc(strlen(att_dump) + 2)) == NULL)
								exit(0);
							memset(bdata, 0x00, strlen(att_dump));
						} else if((bdata = (unsigned char *)realloc(bdata, bsize + strlen(att_dump) + 2)) == NULL)
							exit(0);

						sprintf((char *)(bdata + bsize), "%s\n", att_dump);
						bsize += strlen((char *)att_dump) + 1;
					}
				}
				else
				{
					//dump_opcodes("int_err", ptr, curr, vaddr);
					curr++;
				}
			} else {
				//dump_opcodes("asm_read_instr",ptr,curr,vaddr);
				curr+=4;
			}
		}
		if(bcount == 0) {
			bdata = (unsigned char *)realloc(bdata, bsize + strlen(att_dump) + 2);
			sprintf((char *)(bdata + bsize), "%s\n", att_dump);
			bsize += strlen(att_dump) + 1;
			bb = (basic_block_t *)malloc(sizeof(basic_block_t));
			bb->addr = (int)vaddr;// + curr;
			bb->vaddr = (int)(vaddr + offset + curr) - binstlen;
			bb->data 	= bdata; //(unsigned char *)strdup((char *)bdata);
			bb->size 	= curr;
			bb->icnt 	= binst;
			bb->nopcnt 	= nopcnt;
			bb->ilen 	= binstlen;
			snprintf(bb->fname, sizeof(bb->fname), "%s", sym->name);

			bb_list_add(&sym->blist, bb_list_node_alloc(bb));
			//free(bdata);
		}
	}
	if(sig) {
		if(db_open(&db, sigfile, DB_OVERWRITE) < 0) {
			perror("db_open");
			exit(-1);
		}
		int root = 0;
		for(sym = image->symlist; sym; sym = sym->next) {
			printf("[ ROUTINE INFORMATION sym=%s ]\n", sym->name);
			//db_item_add(&db, sym->name, (void *)sym, sizeof(sym_t) + (sizeof(basic_block_t) + bb->size + strlen(bb->fname)) - 4);
			if((bcount = basic_block_count(&sym->blist))) {
				root = 0;
				//printf("%s %d\n", sym->name, bcount);
				for(vaddr = 0, bnode = sym->blist->head; bnode; bnode = bnode->next, vaddr++) {
					bb = (basic_block_t *)bnode->dptr;
					//printf("\tblock (id=%03d, size=%08d, ncnt=%03d, icnt=%03d, ilen=%05d)\n",
					//		vaddr, bb->size, bb->nopcnt, bb->icnt, bb->ilen);
					db_item_add(&db, sym->name, root++, (void *)bb, (sizeof(basic_block_t) + strlen(bb->fname)) - 8);
				}
			} else {
			}
			printf("\n");
		}
		db_close(&db, 1);
	}

	return (1);
}

/* 
 * build basic block list from binary files */
int bin_build_basicblock_list(bb_list_t **bblist, bin_image_t *image, unsigned int offset, int size, char *sigfile)
{
	unsigned int 	curr, len, vaddr = 0;
	char 			*att_dump;
	unsigned char 	*ptr, *bdata;

	basic_block_t	*bb;
	//bb_list_node_t	*bnode;
	int 			bstart, bstop, bsize, binst, binstlen = 0, nopcnt = 0;
	//db_t			db;

	int bcount = 0;

	len = size;
	curr = 0;
	bstart = bstop = bsize = binst = 0;
	bdata = NULL;
	bcount = 0;
	vaddr = offset;

	ptr = (image->data + offset);

	while(curr < len) {
		if (asm_read_instr(&instr, ptr + curr, len - curr, &proc) > 0) {
			if(ASM_BLOCK_STOP(instr.type)) {
				bstop = 1;
			} else bstart=1;

			att_dump = asm_display_instr_att(&instr, (int)vaddr + curr);
			if (att_dump && (strcmp(att_dump,"int_err"))) {
				if(!strncmp(att_dump, "nop", 3))
					nopcnt++;
				curr += asm_instr_len(&instr);	
				binst++;
				binstlen += asm_instr_len(&instr);

				if(bstop) {
					bdata = (unsigned char *)realloc(bdata, bsize + strlen(att_dump) + 2);
					sprintf((char *)(bdata + bsize), "%s\n", att_dump);
					bsize += strlen(att_dump) + 2;

					if((bb = (basic_block_t *)malloc(sizeof(basic_block_t))) == NULL)
						exit(0);

					bb->addr 	= (int)(vaddr + curr);
					bb->vaddr 	= (int)(vaddr + offset + curr) - binstlen;
					bb->data 	= (unsigned char *)strdup((char *)bdata);
					bb->size 	= curr;
					bb->icnt 	= binst;
					bb->nopcnt 	= nopcnt;
					bb->ilen 	= binstlen;
					snprintf(bb->fname, sizeof(bb->fname), "block %d", bcount);

					bcount++;

					bb_list_add(bblist, bb_list_node_alloc(bb));

					free(bdata);
					bstop = 0;
					bstart = 1;
					bsize = 0;
					nopcnt = binstlen = binst = 0;
					bdata = NULL;
					continue;
				}
				if(bstart) {

					if(bdata == NULL) {
						if((bdata = (unsigned char *)malloc(strlen(att_dump) + 2)) == NULL)
							exit(0);
						memset(bdata, 0x00, strlen(att_dump));
					} else if((bdata = (unsigned char *)realloc(bdata, bsize + strlen(att_dump) + 2)) == NULL)
						exit(0);

					sprintf((char *)(bdata + bsize), "%s\n", att_dump);
					bsize += strlen((char *)att_dump) + 1;
				}
			}
			else
			{
				//dump_opcodes("int_err", ptr, curr, vaddr);
				curr++;
			}
		} else {
			//dump_opcodes("asm_read_instr",ptr,curr,vaddr);
			curr+=4;
		}
	}
	if(bcount == 0) {
		bdata = (unsigned char *)realloc(bdata, bsize + strlen(att_dump) + 2);
		sprintf((char *)(bdata + bsize), "%s\n", att_dump);
		bsize += strlen(att_dump) + 1;
		bb = (basic_block_t *)malloc(sizeof(basic_block_t));
		bb->addr = (int)vaddr;// + curr;
		bb->vaddr = (int)(vaddr + offset + curr) - binstlen;
		bb->data 	= (unsigned char *)strdup((char *)bdata);
		bb->size 	= curr;
		bb->icnt 	= binst;
		bb->nopcnt 	= nopcnt;
		bb->ilen 	= binstlen;
		snprintf(bb->fname, sizeof(bb->fname), "block 0");

		bb_list_add(bblist, bb_list_node_alloc(bb));

		free(bdata);
	}
	return (bcount ? bcount : 0);
}

int elf_add_sym2list(elf_image_t *image, Elf32_Sym *sym)
{
	unsigned int offset =0;
	unsigned char *p;
	Elf32_Shdr *shdr;
	elf_sym_t *esym;
	elf_sym_t **list = &image->symlist;

	if(elf_byteswap(16, sym->st_shndx) > image->ehdr->e_shnum)
		return (-1);
	if((esym = (elf_sym_t *)malloc(sizeof(elf_sym_t))) == NULL)
		return (-1);

	esym->addr 		= elf_byteswap(32, sym->st_value);
	snprintf(esym->name, sizeof(esym->name), "%s",elf_symbol_name(image->strtab, image->strtablen, elf_byteswap(32, sym->st_name)));
	//printf("adding %s\n", esym->name);
	esym->size 		= elf_byteswap(32, sym->st_size);
	esym->type		= ELF32_ST_TYPE(sym->st_info);
	esym->binding	= ELF32_ST_BIND(sym->st_info);
	esym->index 	= elf_byteswap(16, sym->st_shndx);
	esym->next 		= NULL;
	esym->prev 		= NULL;
	esym->code 		= 0;

	shdr = (Elf32_Shdr *)(image->data + image->ehdr->e_shoff + (image->ehdr->e_shentsize * esym->index));
	offset = (shdr->sh_offset + esym->addr);
	p = (unsigned char *)(image->data + offset);

	esym->crc 		= MG_Table_Driven_CRC(0xFFFFFFFF, p, esym->size, MG_CRC_32_ARITHMETIC_CCITT);
	esym->entropy 	= bit_entropy(p, esym->size, &esym->probability);
	esym->code 		= elf_symbol_opcode_crc(image, esym);

	if((*list) == NULL) {
		(*list) = esym;
	} else {
		esym->next = (*list);
		(*list) = esym;
	}
	//printf("%-32s | %08x | %08d | %08x | %-12s (0x%08X) (%.4f bits per symbol, probability=%.4f)\n", esym->name, esym->addr, esym->size, 0, elf_type_str(esym), esym->crc, esym->entropy, esym->probability);
	return (1);
}

elf_sym_t *elf_add_sym(elf_sym_t **list, elf_sym_t *sym)
{
	elf_sym_t *n = malloc(sizeof(elf_sym_t));

	snprintf(n->name, 64, "%s",sym->name);
	n->addr = sym->addr;
	n->size = sym->size ? sym->size : 32;
	n->type = sym->type;
	n->index = sym->index;
	n->binding = sym->binding;
	n->next = NULL;

	if((*list) == NULL)
		*list = n;
	else {
		n->next = (*list);
		(*list)=n;
	}
	return (NULL);
}

void elf_print_symbols(elf_image_t *image)
{
	elf_sym_t	*p, *symlist = image->symlist;
	Elf32_Shdr 	*shdr;
	char		*section_name;
	unsigned int crc = 0;
	unsigned int offset;
	unsigned char *ptr;

	printf("%-32s | %-8s | %-8s | %-8s | %-10s\n", "NAME", "ADDR", "SIZE", "SECTION", "TYPE");
	for(p = symlist; p != NULL; p = p->next) {
		if((image->ehdr->e_shoff + (image->ehdr->e_shentsize * p->index)) > image->size)
			continue;

		//if(p->type != STT_FUNC)
		//	continue;

		if(p->index > image->ehdr->e_shnum)
			continue;

		shdr = (Elf32_Shdr *)(image->data + (image->ehdr->e_shoff + (image->ehdr->e_shentsize * p->index)));
		offset = (shdr->sh_offset + p->addr);
		ptr = (image->data + offset);

		if(p->size <= 0) {
			crc = 0xFFFFFFFF;
		} else {
			crc = MG_Table_Driven_CRC(0xFFFFFFFF, ptr, p->size, MG_CRC_32_ARITHMETIC_CCITT);
		}
		if((section_name = elf_section_name(image->shstrtab, image->shstrtablen, shdr->sh_name)) == NULL)
			section_name = "NULL";

		if(strcmp(section_name, ".text") && strcmp(section_name, ".data") && strcmp(section_name, ".sbss")) 
			continue;

		//printf("%-32s | %08x | %08d | %-12s (0x%08X) (%.4f bits per symbol, probability=%.4f, %04x, %x)\n", p->name, p->addr, p->size, elf_type_str(p), p->crc, p->entropy, p->probability, code, opcode);
		printf("%-32s | %08x | %08d | %08X | %.4f | %.4f | %08x\n", p->name, p->addr, p->size, p->crc, p->entropy, p->probability, p->code);
		//elf_disassemble_symbol(image, p);
	}
}

int elf_load_symbols(elf_image_t *image)
{
	Elf32_Sym *esym = NULL;
	char *name;

	image->lastsym = (Elf32_Sym *)(image->symtab + image->symtablen);

	for(esym = (Elf32_Sym *)(image->symtab); esym && esym < image->lastsym; esym++) {
		name = elf_symbol_name(image->strtab, image->strtablen, elf_byteswap(32, esym->st_name));
		if(!strcmp(name, "NULL")) continue;
		//printf("esym: %d, %d, %d, %s\n", ELF32_ST_BIND(esym->st_info), ELF32_ST_TYPE(esym->st_info), esym->st_size, name);
		/* will parse only local/global functions and objects */
		if(ELF32_ST_BIND(esym->st_info) != STB_LOCAL && ELF32_ST_BIND(esym->st_info) != STB_GLOBAL)
			continue;

		if(ELF32_ST_TYPE(esym->st_info) != STT_OBJECT && ELF32_ST_TYPE(esym->st_info) != STT_FUNC)
			continue;
		/* discard unintialized data */
		if(ELF32_ST_TYPE(esym->st_info) != STT_FUNC && esym->st_size == 0) 
			continue;

		elf_add_sym2list(image, esym);
	}

	return (0);
}

elf_image_t *elf_load_image(const char *filename)
{
	FILE			*fp;
	elf_image_t 	*image;
	struct stat		st;

	if((image = (elf_image_t *)malloc(sizeof(elf_image_t))) == NULL)
		return ((elf_image_t *)NULL);

	if(stat(filename, &st) < 0)
		return ((elf_image_t *)NULL);

	if((fp = fopen(filename, "rb")) == NULL) {
		free(image);
		return ((elf_image_t *)NULL);
	}

	image->size = st.st_size;

	if((image->data = (unsigned char *)malloc(image->size + 1)) == NULL)  {
		free(image);
		return ((elf_image_t *)NULL);
	}

	image->name = strdup(filename);

	image->strtab = NULL;
	image->strtablen = 0;
	image->symtab = NULL;
	image->symtablen = 0;
	image->shstrtab = NULL;
	image->shstrtablen = 0;

	fseek(fp, 0L, SEEK_SET);
	if(fread(image->data, 1, image->size, fp) != image->size) {
		perror("fread");
		free(image);
		fclose(fp);
		return ((elf_image_t *)NULL);
	}
	fclose(fp);

	image->ehdr = (Elf32_Ehdr *)image->data;

	if(image->ehdr->e_ident[EI_DATA] == ELFDATA2MSB)
		ELF32_ENDIAN = 1;

	image->ehdr->e_type 		= elf_byteswap(16, image->ehdr->e_type);
	image->ehdr->e_machine 		= elf_byteswap(16, image->ehdr->e_machine);
	image->ehdr->e_entry		= elf_byteswap(32, image->ehdr->e_entry);
	image->ehdr->e_shoff 		= elf_byteswap(32, image->ehdr->e_shoff);
	image->ehdr->e_phoff 		= elf_byteswap(32, image->ehdr->e_phoff);
	image->ehdr->e_flags 		= elf_byteswap(32, image->ehdr->e_flags);
	image->ehdr->e_shnum 		= elf_byteswap(16, image->ehdr->e_shnum);
	image->ehdr->e_phnum 		= elf_byteswap(16, image->ehdr->e_phnum);
	image->ehdr->e_ehsize		= elf_byteswap(16, image->ehdr->e_ehsize);
	image->ehdr->e_shentsize 	= elf_byteswap(16, image->ehdr->e_shentsize);
	image->ehdr->e_phentsize 	= elf_byteswap(16, image->ehdr->e_phentsize);
	image->ehdr->e_shstrndx		= elf_byteswap(16, image->ehdr->e_shstrndx);
	image->symlist				= NULL;

	if(memcmp(image->ehdr->e_ident, ELF_MAGIC, 4) != 0) {
		fprintf(stderr, "load ehdr");
		free(image);
		return ((elf_image_t *)NULL);
	}

	image->nsections = image->ehdr->e_shnum;

	return (image);
}

elf_sym_t *elf_get_symbol_with_name(elf_image_t *image, const char *name)
{
	elf_sym_t *p;
	for(p = image->symlist; p; p = p->next) {
		if(!strcmp(name, p->name))
			return (p);
	}
	return ((elf_sym_t *)NULL);
}

elf_sym_t *elf_get_symbol_with_addr(elf_image_t *image, unsigned int addr)
{
	elf_sym_t *p;
	for(p = image->symlist; p; p = p->next) {
		if(addr == p->addr)
			return (p);
	}
	return ((elf_sym_t *)NULL);
}

void elf_disassemble_symbol(elf_image_t *image, elf_sym_t *sym)
{
	unsigned int 	curr, len, vaddr = 0;
	unsigned char 	*att_dump;
	unsigned char 	*ptr, ch;
	unsigned int 	offset;

	Elf32_Shdr		*shdr;
	int i;


	printf("<%s>:\n", sym->name);
	shdr = (Elf32_Shdr *)(image->data + image->ehdr->e_shoff + (image->ehdr->e_shentsize * sym->index));
	offset = (shdr->sh_offset + sym->addr);
	if(offset > image->size)
		return;

	ptr = (image->data + offset);
	vaddr = sym->addr;
	len = sym->size;
	curr = 0;
	while(curr < len) {
		if (asm_read_instr(&instr, ptr + curr, len - curr, &proc) > 0) {
			att_dump = (unsigned char *)asm_display_instr_att(&instr, (int)(vaddr + curr));

			if (att_dump && (strcmp((char *)att_dump,"int_err"))) {
				//printf("0x%08x:\t", (int) vaddr + curr);
				printf("%08x : \t",instr.nb_op);
				for (i = instr.len-1; i >= 0; i--)
					printf("%02x", *(ptr + curr + i));
				printf("    ");
				for (i = instr.len-1; i >= 0; i--) {
					ch = *(ptr + curr + i);
					if(ch > 0x20 && ch < 0x7f)
						printf("%c", ch);
					else printf(".");
				}
				printf("\t");
				printf("%-30s", att_dump);
				puts("");

				curr += asm_instr_len(&instr);	
			}
			else
			{
				curr++;
			}
		} else {
			curr+=4;
		}
	}
	/*crc = MG_Table_Driven_CRC(0xFFFFFFFF, ptr, len, MG_CRC_32_ARITHMETIC_CCITT);
	  double entropy;
	  entropy = bit_entropy(ptr, len);
	  printf("CRC32=%08x ENTROPY=%f bits per symbol\n", crc, entropy);*/
}

void bin_disassemble_addr(bin_image_t *image, unsigned int addr, unsigned int len)
{
	unsigned int 	curr, vaddr = 0;
	unsigned char 	*att_dump;
	unsigned char 	*ptr, ch;
	//unsigned int 	offset;

	int i;

	vaddr = addr;
	curr = 0;

	ptr = image->data + addr; 

	while(curr < len) {
		if (asm_read_instr(&instr, ptr + curr, len - curr, &proc) > 0) {
			att_dump = (unsigned char *)asm_display_instr_att(&instr, (int)(vaddr + curr));

			if (att_dump && (strcmp((char *)att_dump,"int_err"))) {
				//printf("0x%08x:\t", (int) vaddr + curr);
				printf("%08x : \t", vaddr + curr);
				for (i = instr.len-1; i >= 0; i--)
					printf("%02x", *(ptr + curr + i));
				printf("    ");
				for (i = instr.len-1; i >= 0; i--) {
					ch = *(ptr + curr + i);
					if(ch > 0x20 && ch < 0x7f)
						printf("%c", ch);
					else printf(".");
				}
				printf("\t");
				printf("%-30s", att_dump);
				puts("");

				curr += asm_instr_len(&instr);	
			}
			else
			{
				curr++;
			}
		} else {
			curr+=4;
		}
	}
	/*crc = MG_Table_Driven_CRC(0xFFFFFFFF, ptr, len, MG_CRC_32_ARITHMETIC_CCITT);
	  double entropy;
	  entropy = bit_entropy(ptr, len);
	  printf("CRC32=%08x ENTROPY=%f bits per symbol\n", crc, entropy);*/
}


bin_image_t *bin_load_image(const char *filename)
{
	FILE			*fp;
	struct stat 	st;

	bin_image_t *image = (bin_image_t *)malloc(sizeof(bin_image_t));
	if(image == NULL)
		return (image);

	if(stat(filename, &st) < 0)
		return (NULL);

	if((image->size = st.st_size) <= 0)
		return (NULL);

	if((fp = fopen(filename, "rb")) == NULL)
		return (NULL);

	if((image->data = malloc(st.st_size + 1)) == NULL) {
		fclose(fp);
		free(image);
		return (NULL);
	}

	if(fread((char *)image->data, 1, st.st_size, fp) != (int)st.st_size) {
		fclose(fp);
		free(image->data);
		free(image);
		return (NULL);
	}
	image->crc 		= MG_Table_Driven_CRC(0xFFFFFFFF, image->data, image->size, MG_CRC_32_ARITHMETIC_CCITT);
	//image->hashl	= hashlittle(image->data, image->size, image->crc);

	return (image);
}

int search_symbol(bin_image_t *bimage, elf_sym_t *sym)
{
	int cur;
	unsigned char *ptr = (unsigned char *)bimage->data;

	if(sym == NULL)
		return (0);

	for(cur = 0; cur < bimage->size;) {
		if(asm_read_instr(&instr, (ptr + cur), sym->size, &proc) > 0) {
			cur += instr.len;
		} else {
			cur += 4;
		}
	}
	return (0);
}

#define MODE_INFO		0x00
#define MODE_SEARCH		0x01
#define MODE_DISASM		0x02
#define MODE_ANALISYS	0x03
#define MODE_CREATE_SIG	0x04
#define MODE_SIGINFO	0x05
#define MODE_BINASM		0x06

void sig_open(db_t *db, const char *filename)
{
	//basic_block_t 	*bb;
	db_item_t		*item;
	unsigned int	icount = 0, ilen = 0, bsize = 0, bcount = 0;

	if(db_open(db, filename, DB_OPEN) < 0) {
		perror("db_open");
		exit(-1);
	}

	printf("signature file '%s' loaded with %d block signatures and %d functions\n\n", filename, db->nitems, db->nroot);
}

void sig_info(const char *filename)
{
	basic_block_t 	*bb;
	db_item_t		*item, *lastitem;
	db_t			db;
	unsigned int	icount = 0, ilen = 0, bsize = 0, bcount = 0;
	double pr, entropy = 0;

	if(db_open(&db, filename, DB_OPEN) < 0) {
		perror("db_open");
		exit(-1);
	}

	printf("block signatures: %d\n", db.nitems);
	printf("function count: %d\n", db.nroot);

	while((item = db_item_get(&db, NULL, 0)) != NULL) {
		bb = (basic_block_t *)item->data;
		if(!item->root && bcount) {
			lastitem->child = bcount;
			printf(" => icount=%d, bcount=%d, ilen=%d, entropy=%f, prob=%f\n", icount, bcount, ilen, entropy, pr);
		}
		if(!item->root) {
			printf("[ FUNCTION %-32s ]", item->name);
			icount = ilen = bsize = bcount = 0;
			fflush(stdout);
		}

		icount += bb->icnt;
		bsize += bb->size;
		ilen += bb->ilen;
		bcount += 1;
		lastitem = item;
		//fflush(stdout);
		//entropy += bit_entropy(bb->data, bb->size, &pr);
		//printf("bb->size: %d\n%x\n", bsize, bb->data);
	}
	printf(" => icount=%d, bcount=%d, ilen=%d\n", icount, bcount, ilen);

	db_close(&db, 0);
}

int main(int argc, char **argv)
{
	elf_sym_t 		*sym = NULL;
	bin_image_t 	*bimage = NULL;
	elf_image_t		*eimage = NULL;
	char			output[65535];
	int				outsize = 0;
	int				mode = 0;
	char 			*sigfile;
	db_t			sigdb;
	bb_list_node_t	*node;

	MG_Setup_CRC_Table(MG_CRC_32_ARITHMETIC_CCITT);

	if(argc < 4) {
		printf("sigtool - 0.1");
		printf("copyright (c) 2011, snape\n\n");
		printf("options:\n");
		printf("\t-createsig <elf file> <signature file>:\t create signatures for routines found on <elf file>\n");
        printf("\t-search\n");
        printf("\t-info\n");
        printf("\t-disasm\n");
        printf("\t-anal\n");
        printf("\t-siginfo\n");
        printf("\t-binasm\n");
        printf("\tread source for info\n");
		exit(0);
	}

	asm_init_mips(&proc);
	//asm_init_i386(&proc);
	asm_config_set_endian(!ELF32_ENDIAN);

	if(!strcmp(argv[1], "-search") && argv[2] != NULL && argv[3] != NULL && argv[4] != NULL) {
		mode = MODE_SEARCH;
		bimage = bin_load_image(argv[2]);
		if((eimage = elf_load_image(argv[3])) == NULL)
			exit(0);
	} else if(!strcmp(argv[1], "-info") && argv[2] != NULL) {
		mode = MODE_INFO;
		if((eimage = elf_load_image(argv[2])) == NULL)
			exit(0);
	} else if(!strcmp(argv[1], "-disasm") && argv[2] != NULL && argv[3] != NULL) {
		mode = MODE_DISASM;
		if((eimage = elf_load_image(argv[2])) == NULL)
			exit(0);
	} else if(!strcmp(argv[1], "-anal") && argv[2] != NULL && argv[3] != NULL) {
		mode = MODE_ANALISYS;
		if((eimage = elf_load_image(argv[2])) == NULL)
			exit(0);
	} else if(!strcmp(argv[1], "-createsig") && argv[2] != NULL) {
		mode = MODE_CREATE_SIG;
		if((eimage = elf_load_image(argv[2])) == NULL)
			exit(0);
		sigfile = argv[3] ? argv[3] : "sigs.db";
	} else if(!strcmp(argv[1], "-siginfo") && argv[2] != NULL) {
		mode = MODE_SIGINFO;
		sigfile = argv[2];
	} else if(!strcmp(argv[1], "-binasm") && argv[2] != NULL) {
		mode = MODE_BINASM;
	}

	bb_list_t	*blist = NULL;
	db_item_t 	*item;

	if(mode != MODE_SIGINFO && mode != MODE_BINASM) {
		if(eimage == NULL) 
			exit(0);
		elf_load_strtab(eimage);
		elf_load_symtab(eimage);
		elf_load_symbols(eimage);

	}
	switch(mode) {
		case MODE_SEARCH:
			if((sym = elf_get_symbol_with_name(eimage, argv[4])) != NULL)  {
				elf_disassemble_symbol(eimage, sym);
			} else {
				printf("%s symbol not found\n", argv[4]);
			}
			break;
		case MODE_INFO:
			elf_print_symbols(eimage);
			break;
		case MODE_DISASM:
			if((sym = elf_get_symbol_with_name(eimage, argv[3])) != NULL)  {
				elf_disassemble_symbol(eimage, sym);
			} else {
				printf("%s symbol not found\n", argv[3]);
			}
			break;
		case MODE_ANALISYS:
			elf_build_basicblock_list(&blist, eimage, 0, NULL);
			break;
		case MODE_CREATE_SIG:
			elf_build_basicblock_list(&blist, eimage, 1, sigfile);
			break;
		case MODE_SIGINFO:
			sig_info(argv[2]);
			break;
		case MODE_BINASM:

			sigfile = argv[3];
			printf("loading binary image: %s\n", argv[2]);
			bimage = bin_load_image(argv[2]);
			bb_list_t *blist = NULL;
			basic_block_t *b, *bb;
			printf("loading signature file: %s\n", argv[3]);
			sig_open(&sigdb, argv[3]);
			printf("OK!\n");
			int start = 1, found = 0;
			int ilenok = 0, icntok = 0;
			int check = 0, bcount = 0;
			int startaddr = 0, ll = 0;
			int probability = 0;
			int	bbbsize = 0;
			int lastaddr = 0;
			unsigned char ch[] = "-/|\\";
			double pr = 0;
			char tmpout[1024];
			memset(output, 0x00, sizeof(output));

			for(found = 0; startaddr < (bimage->size);) {
				if(blist != NULL) {
					for(node = blist->head; node; node = node->next) {
						if((bb = (basic_block_t *)(node->dptr)) == NULL)
							continue;
						free(bb->data);
						free(bb);
						free(node);
					}
					blist = NULL;
				}

				start = 1;
				bcount = 0;
				probability = 0;
				if(++found == 4)
					found = 0;
				printf("\rsearching for item %s, %-3d%% ( %c )", argv[4], ((startaddr * 100) / bimage->size), ch[found]);
				fflush(stdout);
				while ((item = db_item_get(&sigdb, argv[4], start)) != NULL) {
					b = (basic_block_t *)(item->data);
					if(start) {
						//printf("loading basic blocks (address=0x%x, size=%d, file=%s)\n", startaddr, b->size, sigfile);
						ll = bin_build_basicblock_list(&blist, bimage, startaddr, b->size, sigfile);
						//printf("loaded %d blocks\n", ll);
						node = blist->head;
						bbbsize = b->size;
						//bin_disassemble_addr(bimage, 0, b->size);
						//node = blist->head;
					}
					if((bb = (basic_block_t *)(node->dptr)) == NULL) {
						printf("exiting, db corrupted\n");
						break;
					}

					ilenok = icntok = 0;

                    /* dumb probability calculation */
					if(bb->ilen == b->ilen) {
						ilenok = 1;
						probability+=3;
					} else 
					if(bb->ilen < b->ilen) {
						if((b->ilen - bb->ilen) < BB_THRESHOLD) {
							ilenok = 1;
							probability+=1;
						}
					} else 
					if(bb->ilen > b->ilen){
						if((bb->ilen - b->ilen) < BB_THRESHOLD) {
							ilenok = 1;
							probability+=1;
						}
					} 
					if(bb->icnt == b->icnt) {
						icntok = 1;
						probability+=3;
					} else 
					if(bb->icnt < b->icnt) {
						if((b->icnt - bb->icnt) < BB_THRESHOLD) {
							icntok = 1;
							probability+=1;
						}
					} else
					if(bb->icnt > b->icnt) {
						if((b->icnt - bb->icnt) < BB_THRESHOLD) {
							icntok = 1; 
							probability+=1;
						}
					}

					if(icntok && ilenok) check++;

					bcount++;
					ilenok = icntok = 0;
					start = 0;
					node=node->next;
					if(node == NULL)
						break;
				}
				
				icntok = 0;
				if(check > bcount)
					if((check - bcount) <= BB_THRESHOLD)
						icntok = 1;
				if(check < bcount)
					if((bcount - check) <= BB_THRESHOLD)
						icntok = 1;

				if(icntok) {
					if((startaddr - lastaddr) > 16) {
						outsize = strlen(output);
						memset(tmpout, 0x00, sizeof(tmpout));
						snprintf(tmpout, sizeof(tmpout), 
                                "sig(fname=%s, probability=%d, foff=0x%x, [check=%d, bcount=%d, startaddr=%d, threshold=%d])\n", 
                                b->fname, probability, 
								startaddr, check, bcount, startaddr, BB_THRESHOLD);
						//printf("size: %d\n", printf(output));
						strcat(output + outsize, tmpout);
						//outsize += strlen(tmpout);
					}
					lastaddr = startaddr;
					if(probability == 36)
						bin_disassemble_addr(bimage, startaddr, bbbsize);
					check = 0; bcount = 0;
					//printf("\n\n\n");
					//found = 1;
					//break;
				}
				startaddr += 4;
			}
			printf("\n%s\n", output);
			break;
		default:
			break;
	}
	free(eimage);
	printf("\nexiting.\n");
	exit(0);
}
