/*
** Author  : <sk at devhell dot org>
** Started : Mon Jun 10 01:49:20 2002
** Updated : Thu Dec  4 02:46:23 2003
**
** $Id: mydisasm.c 1311 2009-01-14 20:36:48Z may $
**
*/

/* 
 * this tool is designed to disassemble binary with the same output as
 * objdump (except symbols.)
 * 
 * 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libasm.h>


int	usage(char *p) 
{
  printf("Usage: %s <binary> <sym/vaddr> <[len]>\n", 
	  p);
  return (-1);
}

void	dump_opcodes(char *why, u_char *ptr, u_int curr, u_int vaddr)
{
  printf("0x%08x (%s): .byte 0x%02x\n", (int) vaddr + curr, why, *(ptr + curr));
  printf(";; error reading instruction at %p\n", ptr + curr);
  printf(";; dumping opcodes: %02x %02x %02x %02x\n", 
	 *(ptr + curr), *(ptr + curr + 1), 
	 *(ptr + curr + 2), *(ptr + curr + 3));
}

int	main(int ac, char **argv) {
  u_char	*ptr;
  unsigned long	start;
  unsigned long	end;
  unsigned long	len;
  u_int		curr;
  unsigned long	vaddr;
  char		*att_dump;
  int		i;
  asm_instr	instr;
  asm_processor	proc;
  u_int		arch;
  FILE *fp;

  printf("opening %s\n", argv[1]);
  fp = fopen(argv[1], "rb");
  fseek(fp, 0L, SEEK_END);
  len = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  start = strtoul(argv[2], NULL, 16);
  vaddr = 0;
  fseek(fp, start, SEEK_SET);

  printf("offset: %x\n", start);
  len = atoi(argv[3]);
  ptr = malloc(len + 1);
  memset(ptr, 0, len + 1);
  start = 0;
  
  curr = fread(ptr, 1, len, fp);
  if (curr != len)
    {
      printf("error reading %li bytes at %li -> read %i bytes\n", len, start, curr);
      return (-1);
    }
 
  /* disassembling loop */

  curr = 0;
  unsigned char ch = 0;
  
  asm_init_mips(&proc);

  asm_config_set_endian(CONFIG_ASM_BIG_ENDIAN);

  while(curr < len) {
    if (asm_read_instr(&instr, ptr + curr, len - curr, &proc) > 0) {
      att_dump = asm_display_instr_att(&instr, (int) vaddr + curr);
      if (att_dump && (strcmp(att_dump,"int_err"))) 
	{
	  printf("0x%08x:\t", (int) vaddr + curr);
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
	  
	  //asm_instruction_debug(&instr, stdout);
	  curr += asm_instr_len(&instr);	
	}
      else
	{
	  dump_opcodes("int_err", ptr, curr, vaddr);
	  curr++;
	}
    } else {
      dump_opcodes("asm_read_instr",ptr,curr,vaddr);
      curr++;
    }
  }
  	
  fclose(fp);
  free(ptr);
  return (0);
}
  




