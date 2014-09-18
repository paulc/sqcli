
SQUIRREL = ../SQUIRREL3
SQRAT = ../sqrat

CC = cc
LD = c++
CFLAGS = -Os -I$(SQUIRREL)/include
LDFLAGS = -L$(SQUIRREL)/lib
LIBS = -Os -lsquirrel

PROG = cli
OBJS = cli.o linenoise.o sds.o list.o

EXT = -DSTDBLOB -DSTDSYSTEM -DSTDIO -DSTDMATH -DSTDSTRING -DSTDAUX
EXTLIB = -lsqstdlib
#EXT =
#EXTLIB =

DEBUG ?= -O0 -g

all: $(PROG)

# Target
$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) $(DEBUG) -o $(PROG) $(OBJS) $(LIBS) $(EXTLIB)

# Deps

# Generic build targets
.c.o:
	$(CC) -c $(CFLAGS) $(EXT) $(DEBUG) $<

.cpp.o:
	$(CC) -c $(CFLAGS) $(EXT) $(DEBUG) $<

dep:
	$(CC) -MM $(CFLAGS) *.c

clean:
	rm -rf $(PROG) $(LIB) *.o *~
