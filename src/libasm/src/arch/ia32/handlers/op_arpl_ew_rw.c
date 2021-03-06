/*
** $Id: op_arpl_ew_rw.c 1311 2009-01-14 20:36:48Z may $
**
*/
#include <libasm.h>
#include <libasm-int.h>

/*
  <instruction func="op_arpl_ew_rw" opcode="0x63"/>
*/

int     op_arpl_ew_rw(asm_instr *new, u_char *opcode, u_int len,
		      asm_processor *proc)
{
  new->ptr_instr = opcode;
  new->instr = ASM_ARPL;
  new->len += 1;
  return (new->len);
}
