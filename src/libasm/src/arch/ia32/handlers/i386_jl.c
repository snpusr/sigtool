/*
** $Id: i386_jl.c 1413 2010-05-21 03:41:25Z figueredo $
**
*/
#include <libasm.h>
#include <libasm-int.h>

/*
  <i386 func="i386_jl" opcode="0x8c"/>
*/


int i386_jl(asm_instr *new, u_char *opcode, u_int len, asm_processor *proc)
{
  new->type = ASM_TYPE_BRANCH | ASM_TYPE_CONDCONTROL;
  new->instr = ASM_BRANCH_S_LESS;
  new->len += 1;

#if LIBASM_USE_OPERAND_VECTOR
#if WIP
  new->len += asm_operand_fetch(&new->op[0], opcode + 1, ASM_OTYPE_JUMP, new, 0);
#else
  new->len += asm_operand_fetch(&new->op[0], opcode + 1, ASM_OTYPE_JUMP, new);
#endif
#else
  new->op[0].type = ASM_OTYPE_JUMP;
  new->op[0].content = ASM_OP_VALUE | ASM_OP_ADDRESS;
  new->op[0].ptr = opcode + 1;
  new->op[0].len = 4;
  memcpy(&new->op[0].imm, opcode + 1, 4);
  new->len += 4;
#endif
  return (new->len);
}
