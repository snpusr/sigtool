/*
** $Id: i386_cpuid.c 1311 2009-01-14 20:36:48Z may $
**
*/
#include <libasm.h>
#include <libasm-int.h>

/*
  Opcode :              0x0f 0xa2
  Instruction :         CPUID
 */

int i386_cpuid(asm_instr *new, u_char *opcode, u_int len, asm_processor *proc)
{
  new->len += 1;
  new->instr = ASM_CPUID;
  return (new->len);
}
