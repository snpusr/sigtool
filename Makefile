CC		= gcc
DFLAGS 	= -m32 -ggdb -g3 -DERESI32 -fPIC -I./include/ -I./src/libasm/include -I./src/libaspect/include
CFLAGS	= -Wall -Wno-unused -m32 -DERESI32 -fPIC -funroll-all-loops -fomit-frame-pointer -I./include/ -I./src/libasm/include -I./src/libaspect/include $(DFLAGS)
SRCS	= src/image.c src/fastcrc.c src/list.c src/db.c
OBJECTS	= $(SRCS:.c=.o)
TARGET	= sigtool
LIBS	= src/libasm/libasm32.a src/libaspect/libaspect32.a

all: libasm32 $(OBJECTS)
	@$(CC) $(DFLAGS) $(LIBS) $(OBJECTS) -o $(TARGET)

libasm32:
	@echo "BUILDING LIBASPECT..."
	@make -C src/libaspect all
	@echo ""
	@echo "BUILDING LIBASM..."
	@make -C src/libasm all

debug:
	@$(CC) $(DFLAGS) $(LIBS) $(OBJECTS) -o $(TARGET)

dis:
	@$(CC) $(CFLAGS) $(LIBS) disasm.c -o $(TARGET)

clean: 
	rm -rf $(OBJECTS) $(TARGET) 
	make -C src/libasm clean
	make -C src/libaspect clean

.SUFFIXES: .c .o
.c.o:
	@echo "[CC SIGTOOL] $< ..."
	@$(CC) $(DFLAGS) -c -o $@ $<

