
SQUIRREL = ../SQUIRREL3
SQCLI = .

CC = cc
LD = c++
CFLAGS = -Wall -Werror -Os -I$(SQUIRREL)/include
LDFLAGS = -L$(SQUIRREL)/lib
LIBS = -Os -lsquirrel

#EXT_OBJS =
#EXT_INCLUDE =
#EXT_REGISTER_FUNC =
#EXT = -DEXT_INCLUDE=$(EXT_INCLUDE) -DEXT_REGISTER_FUNC=$(EXT_REGISTER_FUNC) -I.

CLI = cli
CLI_OBJS = cli.o linenoise.o sds.o list.o

STDEXT = -DSTDBLOB -DSTDSYSTEM -DSTDIO -DSTDMATH -DSTDSTRING -DSTDAUX
STDEXTLIB = -lsqstdlib

DEBUG ?= -O0 -g

all: $(CLI)

$(CLI): $(CLI_OBJS) $(EXT_OBJS)
	$(LD) $(LDFLAGS) $(DEBUG) -o $(CLI) $(CLI_OBJS) $(LIBS) $(STDEXTLIB) $(EXT_OBJS)

# Extension objects

# CLI objects
cli.o: $(SQCLI)/cli.c
	$(CC) -c $(CFLAGS) -I$(SQCLI) $(STDEXT) $(EXT) $(SQCLI)/cli.c

linenoise.o: $(SQCLI)/linenoise.c
	$(CC) -c $(CFLAGS) -I$(SQCLI) $(SQCLI)/linenoise.c

sds.o: $(SQCLI)/sds.c
	$(CC) -c $(CFLAGS) -I$(SQCLI) $(SQCLI)/sds.c

list.o: $(SQCLI)/list.c
	$(CC) -c $(CFLAGS) -I$(SQCLI) $(SQCLI)/list.c

# Generic build targets
.c.o:
	$(CC) -c $(CFLAGS) $(STDEXT) $(DEBUG) $<

.cpp.o:
	$(CC) -c $(CFLAGS) $(STDEXT) $(DEBUG) $<

dep:
	$(CC) -MM $(CFLAGS) *.c

clean:
	rm -rf $(CLI) $(LIB) *.o *~
