##
## $Id: Makefile 1331 2009-02-17 05:02:42Z figueredo $
## Author  : <sk at devhell dot org>
## Started : Xxx Xxx xx xx:xx:xx 2002
## Updated : Sun Mar 21 00:03:07 2004
##

include ../config.h
#
# Architecture source files
#

#CFLAGS += -ggdb
CFLAGS += $(CFLAGS_libasm) $(EXTRACFLAGS) -D__MacOSX__
RANLIB=ranlib

SRCS_libasm 	=				\
		src/generic.c			\
		src/output.c			\
		src/error.c			\
		src/operand.c			\
		src/instruction.c		\
		src/register.c			\
		src/build.c			\
		src/vectors.c

#
# IA32 Support
#

ifeq ($(LIBASM_ENABLE_IA32), 1)
SRCS_libasm +=	src/arch/ia32/init_i386.c	\
		src/arch/ia32/register.c	\
		src/arch/ia32/tables_i386.c	\
		src/arch/ia32/output_ia32.c	\
		src/arch/ia32/operand_ia32.c
ifeq ($(LIBASM_PACKED_HANDLERS), 1)
$(info [IA32] PACKED)
PACKED_ARCH += packed_ia32
SRCS_libasm += 	src/arch/ia32/packed_handlers.c
else
$(info [IA32] UNPACKED)
SRCS_libasm    += $(foreach dir,src/arch/ia32/handlers/,\
	$(wildcard $(dir)/*.c))
SRCS_libasm    += $(foreach dir,src/arch/ia32/operand_handlers/,\
	$(wildcard $(dir)/*.c))
endif
CFLAGS += -DLIBASM_ENABLE_IA32
endif

#
# Sparc Support
#

ifeq ($(LIBASM_ENABLE_SPARC), 1)
PACKED_ARCH += packed_sparc
SRCS_libasm += 	src/arch/sparc/init_sparc.c	\
		src/arch/sparc/register.c	\
		src/arch/sparc/tables_sparc.c	\
		src/arch/sparc/output_sparc.c	\
		src/arch/sparc/sparc_convert.c
ifeq ($(LIBASM_PACKED_HANDLERS),1)
$(info [SPARC] PACKED)
PACKED_ARCH += packed_sparc
SRCS_libasm += src/arch/sparc/packed_handlers.c
else
$(info [SPARC] UNPACKED)
SRCS_libasm    += $(foreach dir,src/arch/sparc/handlers/,\
	$(wildcard $(dir)/*.c))
SRCS_libasm    += $(foreach dir,src/arch/sparc/operand_handlers/,\
	$(wildcard $(dir)/*.c))
endif
CFLAGS += -DLIBASM_ENABLE_SPARC
endif

#
# Mips Support
#

ifeq ($(LIBASM_ENABLE_MIPS), 1)
PACKED_ARCH += packed_mips
SRCS_libasm +=	src/arch/mips/init_mips.c	\
		src/arch/mips/output_mips.c	\
		src/arch/mips/tables_mips.c	\
		src/arch/mips/mips_convert.c	\
		src/arch/mips/register_mips.c
ifeq ($(LIBASM_PACKED_HANDLERS), 1)
$(info [MIPS] PACKED)
PACKED_ARCH += packed_mips
SRCS_libasm += src/arch/mips/packed_handlers.c
else
$(info [MIPS] UNPACKED)
SRCS_libasm    += $(foreach dir,src/arch/mips/handlers/,\
	$(wildcard $(dir)/*.c))
SRCS_libasm    += $(foreach dir,src/arch/mips/operand_handlers/,\
	$(wildcard $(dir)/*.c))
endif
CFLAGS += -DLIBASM_ENABLE_MIPS
endif

#
# ARM Support
#

ifeq ($(LIBASM_ENABLE_ARM), 1)
PACKED_ARCH += packed_arm
SRCS_libasm += 	src/arch/arm/init_arm.c	\
		src/arch/arm/register_arm.c	\
		src/arch/arm/tables_arm.c	\
		src/arch/arm/output_arm.c	\
		src/arch/arm/decode_arm.c	\
		src/arch/arm/arm_convert.c
ifeq ($(LIBASM_PACKED_HANDLERS),1)
$(info [ARM] PACKED)
PACKED_ARCH += packed_arm
SRCS_libasm += src/arch/arm/packed_handlers.c
else
$(info [ARM] UNPACKED)
SRCS_libasm    += $(foreach dir,src/arch/arm/handlers/,\
	$(wildcard $(dir)/*.c))
SRCS_libasm    += $(foreach dir,src/arch/arm/operand_handlers/,\
	$(wildcard $(dir)/*.c))
endif
CFLAGS += -DLIBASM_ENABLE_ARM
endif

OBJS32_libasm 	=	${SRCS_libasm:.c=.32.o} 
OBJS32_libasm	+=	${SRCS_hdl:.c=.32.o}		
OBJS64_libasm 	=	${SRCS_libasm:.c=.64.o} 
OBJS64_libasm	+=	${SRCS_hdl:.c=.64.o}		

NAME32_libasm 	= 	libasm32.a
NAME32_libasm_o	=	libasm32.o
NAME64_libasm 	= 	libasm64.a
NAME64_libasm_o	=	libasm64.o

SRCS_sparc	=	tools/sparc_mydisasm.c
OBJS_sparc	=	${SRCS_sparc:.c=.o}
NAME_sparc	=	test_sparc

#CFLAGS32 	= $(CFLAGS) -Iinclude -Isrc/include -Wall -g3 -fPIC -I../libaspect/include -DERESI32
CFLAGS32 	= $(CFLAGS) -m32 -DERESI32 -Iinclude -fPIC -Isrc/include -Wall -I../libaspect/include
CFLAGS64 	= $(CFLAGS) -Iinclude -Isrc/include -Wall -g3 -I../libaspect/include -DERESI64

RM 		= 	rm -f
ETAGS 		= 	etags
CC 		?= 	gcc -E
LD		?=	ld
CP 		= 	cp
RANLIB  = ranlib


all:	all32
all32: 	libasm32.so
all64: 	libasm64.so

#src/arch/ia32/packed_handlers.c:
#	cat ${SRCS_IA32} > src/arch/ia32/packed_handlers.c

install:
	${CP} ${NAME_libasm} /usr/lib
	${CP} include/libasm.h /usr/include
	${CP} include/libasm-i386.h /usr/include
	${CP} include/libasm-sparc.h /usr/include

libasm32.so: $(PACKED_ARCH) $(OBJS32_libasm)
	@echo "[AR] ${NAME32_libasm}"
	@ar rcs  ${NAME32_libasm} ${OBJS32_libasm}
	@echo "[RANLIB] libasm32.a"
	@${RANLIB} ${NAME32_libasm}
	@$(LD) -r -arch i386 $(OBJS32_libasm) -o ${NAME32_libasm_o}
	@echo "[LD] libasm32.so"
	@$(CC) -m32 -L../libaspect/ -laspect32 ${OBJS32_libasm} -o libasm32.so -shared

libasm64.so: $(PACKED_ARCH) $(OBJS64_libasm)
	ar rcs ${NAME64_libasm} ${OBJS64_libasm}
#echo "[AR] ${NAME64_libasm}"
	${RANLIB} ${NAME64_libasm}
#echo "[RANLIB] ${NAME64_libasm}"
	$(LD) -r $(OBJS64_libasm) -o ${NAME64_libasm_o}
#echo "[CC -shared] libasm64.so"
ifeq ($(IRIX),1)
	$(LD) -L../libaspect/ -laspect64 ${OBJS64_libasm} -o libasm64.so -shared
else
	$(CC) -L../libaspect/ -laspect64 ${OBJS64_libasm} -o libasm64.so -shared
endif

clean: fclean
	$(RM) ${OBJS32_libasm} ${OBJ64_libasm}
	find .   -name '*~' -exec rm -f {} \;
	find src -name '*.o' -exec rm -rf {} \;
	find src -name 'packed_handlers.c' -exec rm -rf {} \;

fclean:
	$(RM) ${NAME32_libasm} ${NAME64_libasm}
	$(RM) *.so *.o *.a

packed_ia32:
	@#echo "packed ia32 ..."
	@cat src/arch/ia32/handlers/*.c src/arch/ia32/operand_handlers/*.c > src/arch/ia32/packed_handlers.c
packed_sparc:
	@#echo "packed sparc ..."
	@cat src/arch/sparc/handlers/*.c src/arch/sparc/operand_handlers/*.c > src/arch/sparc/packed_handlers.c
packed_mips:
	@#echo "packed mips ..."
	@cat src/arch/mips/handlers/*.c src/arch/mips/operand_handlers/*.c > src/arch/mips/packed_handlers.c
packed_arm:
	@#echo "packed arm ..."
	@cat src/arch/arm/handlers/*.c src/arch/arm/operand_handlers/*.c > src/arch/arm/packed_handlers.c

%.32.o 		: %.c
		@echo "[CC] $< ..."
		@$(CC) $(CFLAGS32) -c -o $@ $<

%.64.o 		: %.c
		$(CC) $(CFLAGS64) -c -o $@ $<
