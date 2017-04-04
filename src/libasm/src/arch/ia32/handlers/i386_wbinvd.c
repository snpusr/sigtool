/*
** $Id: i386_wbinvd.c 1311 2009-01-14 20:36:48Z may $
**
*/
#include <libasm.h>
#include <libasm-int.h>

int     i386_wbinvd(asm_instr *new, u_char *opcode, u_int len,
		    asm_processor *proc)
{
  new->len += 1;
  new->instr = ASM_WBINVD;
  return (new->len);

}
