/*
** $Id: op_cmp_rmv_rv.c 1311 2009-01-14 20:36:48Z may $
**
*/
#include <libasm.h>
#include <libasm-int.h>

/*
  <instruction func="op_cmp_rmv_rv" opcode="0x39"/>
*/

int op_cmp_rmv_rv(asm_instr *new, u_char *opcode, u_int len,
                  asm_processor *proc)
{
  new->len += 1;
  new->ptr_instr = opcode;
  new->instr = ASM_CMP;

  new->type = ASM_TYPE_COMPARISON | ASM_TYPE_WRITEFLAG;
  new->flagswritten = ASM_FLAG_AF | ASM_FLAG_CF | ASM_FLAG_PF |
                        ASM_FLAG_OF | ASM_FLAG_SF | ASM_FLAG_ZF;

#if WIP
  new->len += asm_operand_fetch(&new->op[0], opcode + 1, ASM_OTYPE_ENCODED,                                new, 0);
#else
  new->len += asm_operand_fetch(&new->op[0], opcode + 1, ASM_OTYPE_ENCODED,                                new);
#endif
#if WIP
  new->len += asm_operand_fetch(&new->op[1], opcode + 1, ASM_OTYPE_GENERAL,                                new, 0);
#else
  new->len += asm_operand_fetch(&new->op[1], opcode + 1, ASM_OTYPE_GENERAL,                                new);
#endif

  return (new->len);
}
