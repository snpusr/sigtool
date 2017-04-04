/**
* @file libasm/src/arch/sparc/handlers/asm_sparc_bpr.c
** @ingroup SPARC_instrs
*/
/*
**
** $Id: asm_sparc_bpr.c 1413 2010-05-21 03:41:25Z figueredo $
**
*/
#include "libasm.h"

int
asm_sparc_bpr(asm_instr * ins, u_char * buf, u_int len,
	      asm_processor * proc)
{
  struct s_decode_rbranch opcoder;
  struct s_asm_proc_sparc *inter;
  sparc_convert_rbranch(&opcoder, buf);

  inter = proc->internals;

  ins->instr = inter->brcc_table[opcoder.rcond];
  ins->type = ASM_TYPE_BRANCH | ASM_TYPE_CONDCONTROL;
  ins->nb_op = 2;
  ins->op[0].imm = opcoder.d16;
  ins->op[1].baser = opcoder.rs1;
  ins->annul = opcoder.a;
  ins->prediction = opcoder.p;
  asm_sparc_op_fetch(&ins->op[0], buf, ASM_SP_OTYPE_DISPLACEMENT, ins);
  asm_sparc_op_fetch(&ins->op[1], buf, ASM_SP_OTYPE_REGISTER, ins);

  return 4;
}
