CFLAGS32 				+= -Wall 
CFLAGS64 				+= -Wall 
STATOPT					= -static
STATOPT2				= -DUSE_STATIC
EXTRACFLAGS 			+= -g3
BUILDOPT				= -m32
LIBASM_ENABLE_IA32 		= 1
LIBASM_ENABLE_SPARC 	= 1
LIBASM_ENABLE_MIPS 		= 1
LIBASM_ENABLE_ARM 		= 1
LIBASM_PACKED_HANDLERS 	= 1
MAKE					= make
BUILDOP 				= 
LDASMOPT				= -lasm
LPTHREAD				= -lpthread
BITS 					= 
SHELL					= /bin/bash