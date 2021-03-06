/**
* @file libasm/src/arch/ia32/handlers/op_aad.c
 *
 * @ingroup IA32_instrs
 * $Id: op_aad.c 1397 2009-09-13 02:19:08Z may $
 *
 */
#include <libasm.h>
#include <libasm-int.h>

/**
 * Handler for the aad instruction, opcode 0xd5
 * @param new Pointer to instruction structure.
 * @param opcode Pointer to data to disassemble.
 * @param len Length of data to disassemble.
 * @param proc Pointer to processor structure.
 * @return Length of disassembled instruction.
 */
int op_aad(asm_instr *new, u_char *opcode, u_int len, asm_processor *proc)
{
  new->len += 1;
  new->ptr_instr = opcode;
  new->instr = ASM_AAD;
  new->type = ASM_TYPE_WRITEFLAG | ASM_TYPE_ARITH;
  new->flagswritten = ASM_FLAG_SF | ASM_FLAG_ZF | ASM_FLAG_PF;
  return (new->len);
}
