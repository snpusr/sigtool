/**
 * @brief Handler for instruction push cs opcode 0x0e
 *
 * @ingroup IA32_instrs
** $Id: op_push_cs.c 1396 2009-09-12 11:25:11Z may $
**
*/
#include <libasm.h>
#include <libasm-int.h>


/**
 * @brief Handler for instruction push cs opcode 0x0e
 */
int op_push_cs(asm_instr *new, u_char *opcode, u_int len,
               asm_processor *proc) {
  new->len += 1;
  new->ptr_instr = opcode;
  new->instr = ASM_PUSH;
  new->type = ASM_TYPE_TOUCHSP | ASM_TYPE_STORE;
  new->spdiff = -4;

#if WIP
  new->len += asm_operand_fetch(&new->op[0], opcode, ASM_OTYPE_FIXED, new,
				asm_fixed_pack(0, ASM_OP_BASE, ASM_REG_CS,
					       ASM_REGSET_SREG));

#else
  new->len += asm_operand_fetch(&new->op[0], opcode, ASM_OTYPE_FIXED, new);
  new->op[0].content = ASM_OP_BASE | ASM_OP_FIXED;
  new->op[0].regset = ASM_REGSET_SREG;
  new->op[0].baser = ASM_REG_CS;
#endif

  return (new->len);
}
