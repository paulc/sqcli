
SQUIRREL = ../SQUIRREL3
SQRAT = ../sqrat

CC = cc
LD = c++
CFLAGS = -I$(SQUIRREL)/include -I$(SQRAT)/include
LDFLAGS = -L$(SQUIRREL)/lib
LIBS = -lsquirrel -lsqstdlib

PROG = cli
OBJS = cli.o linenoise.o

DEBUG ?= -O0 -g

all: $(PROG)

# Target
$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) $(DEBUG) -o $(PROG) $(OBJS) $(LIBS)

# Deps

# Generic build targets
.c.o:
	$(CC) -c $(CFLAGS) $(DEBUG) $<

.cpp.o:
	$(CC) -c $(CFLAGS) $(DEBUG) $<

dep:
	$(CC) -MM $(CFLAGS) *.c

clean:
	rm -rf $(PROG) $(LIB) *.o *~
