/**
* @file libasm/src/arch/mips/output_mips.c
** @ingroup mips
*/
/**
* @file libasm/src/arch/mips/output_mips.c
 * @brief This file implements the MIPS ASCII output engine
 *
 * Made by Manuel Martin and Adam Zabrocki
 * $Id: output_mips.c 1397 2009-09-13 02:19:08Z may $
 */
#include <libasm.h>


/**
 * @fn char *asm_mips_display_operand(asm_instr *ins, int num, unsigned int addr)
 * @brief Return ASCII representation of a mips operand
 *
 * @param ins Pointer to asm_instr structure.
 * @param num Now it unused.
 * @parram addr Virtual Address of instruction.
 * @return A pointer to a static buffer or NULL on error.
 */
char		*asm_mips_display_operand(asm_instr *ins, int num, eresi_Addr addr)
{
  unsigned int	i;
  static char	bufer[80];
  char		temp[4][20];
  asm_operand	*op;
  unsigned int converted;

  memset(bufer, 0x0, sizeof(bufer));
  for (i = 0; i < 4; i++) 
    {
      op = &ins->op[i];
      memset(&temp[i][0], 0x0, sizeof(temp[i]));
      switch(op->type) 
	{
        case ASM_MIPS_OTYPE_REGISTER:
			memcpy((char *)&converted, ins->ptr_instr, ins->len);
			if((((converted >> 26)) == MIPS_OPCODE_COP0) && (i == 1) && op->baser < 32) {
				op->baser += 32;
			}
	  		snprintf(temp[i],sizeof(temp[i]),(i) ? ", %s" : "%s",
		   			(op->regset) ? e_mips_registers[op->baser].fpu_mnemonic : e_mips_registers[op->baser].ext_mnemonic);
	  break;
	  
        case ASM_MIPS_OTYPE_IMMEDIATE:
			memcpy((char *)&converted, ins->ptr_instr, ins->len);
			switch((converted >> 0)) {
				case MIPS_OPCODE_ADDIU:
				case MIPS_OPCODE_ADDU:
				case MIPS_OPCODE_SUBU:
				case MIPS_OPCODE_SUB:
	  				snprintf(temp[i],sizeof(temp[i]),(i) ? ", %d" : "%d\t# 0x%x",(unsigned short)op->imm, (unsigned short)op->imm);
					break;
				default:
	  				snprintf(temp[i],sizeof(temp[i]),(i) ? ", 0x%x" : "0x%x",(unsigned short)op->imm);
					break;
			}
	  				//snprintf(temp[i],sizeof(temp[i]),(i) ? ", %d" : "%d",(short)op->imm);
	  break;
	  
        case ASM_MIPS_OTYPE_JUMP:
	  //snprintf(temp[i], sizeof(temp[i]), (i ? ", "XFMT : XFMT),  (op->imm << 2) | ((((addr + 8) >> 28) & 0xF) << 28));
	  snprintf(temp[i], sizeof(temp[i]), (i ? ", 0x%x": "0x%x"),  (op->imm << 2) | ((((addr + 8) >> 28) & 0xF) << 28));
	  break;
	  
        case ASM_MIPS_OTYPE_NONE:
        case ASM_MIPS_OTYPE_NOOP:
	  //        snprintf(bufer,sizeof(bufer)," ");
	  break;
	  
        case ASM_MIPS_OTYPE_BRANCH:
	  snprintf(temp[i], sizeof(temp[i]) , (i) ? ", 0x%x" : "0x%x", (addr + (((short) op->imm + 1) * 4)));
	  break;
	  
        case ASM_MIPS_OTYPE_REGBASE:
	  snprintf(temp[i], sizeof(temp[i]), "(%s)",
		   (op->regset) ? 
		   e_mips_registers[op->baser].fpu_mnemonic : 
		   e_mips_registers[op->baser].ext_mnemonic);
	  break;
	  
	}
    }
  
  for (i = 0; i < 4; i++)
    if (temp[i][0])
      strcat(bufer,temp[i]);
  
  return (bufer[0]) ? bufer : NULL;
  
}

/**
 * @fn char *asm_mips_display_instr(asm_instr *ins,int addr)
 * @brief Return ASCII representation of a mips instruction with operand.
 *
 * @param ins Pointer to instruction structure.
 * @param addr Virtual Address of instruction.
 * @return Pointer to a static buffer or NULL on error.
 */
char		*asm_mips_display_instr(asm_instr *ins, eresi_Addr addr)
{
  static char	buf[120];
  char		*tmp = NULL;
  
  tmp = asm_mips_display_operand(ins, 0, addr);
  bzero(buf, sizeof(buf));
  
  if (tmp)
    snprintf(buf, sizeof(buf), "%-8s %s", 
	     e_mips_instrs[ins->instr].mnemonic,
	     asm_mips_display_operand(ins, 0, addr));
  else
    snprintf(buf, sizeof(buf), "%-8s", e_mips_instrs[ins->instr].mnemonic);
  
  return buf;
}
