/*	see copyright notice in squirrel.h */

#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdmath.h>	
#include <sqstdstring.h>
#include <sqstdaux.h>

#ifdef SQUNICODE
#define scfprintf fwprintf
#define scfopen	_wfopen
#define scvprintf vfwprintf
#else
#define scfprintf fprintf
#define scfopen	fopen
#define scvprintf vfprintf
#endif

#include "sds.h"
#include "linenoise.h"

void printfunc(HSQUIRRELVM v,const SQChar *s,...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stdout, s, vl);
	va_end(vl);
}

void errorfunc(HSQUIRRELVM v,const SQChar *s,...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
}

sds print_sqobject(HSQUIRRELVM v,SQInteger idx) {
    const SQChar *s;
    idx = (idx < 0) ? sq_gettop(v) + idx + 1 : idx;
    SQObjectType t = sq_gettype(v,idx);
    if (t == OT_ARRAY || t == OT_TABLE) {
        sds buf = sdsempty();
        char *key,*val;
        int i = 0;
        buf = sdscat(buf,(t == OT_ARRAY) ? "[" : "{");
        sq_pushnull(v);
        while(SQ_SUCCEEDED(sq_next(v,idx))) {
            if (i++) {
                buf = sdscat(buf,", ");
            }
            key = print_sqobject(v,-2);
            val = print_sqobject(v,-1);
            if (t == OT_ARRAY) {
                buf = sdscat(buf,val);
            } else {
                buf = sdscatprintf(buf,"%s: %s",key,val);
            }
            sdsfree(key);
            sdsfree(val);
            sq_pop(v,2);
        }
        buf = sdscat(buf,(t == OT_ARRAY) ? "]" : "}");
        sq_pop(v,1);
        return buf;
    } else {
        sq_tostring(v,idx);
        sq_getstring(v,-1,&s);
        sds buf = sdsnew(s);
        sq_pop(v,1);
        return buf;
    }
}

void print_stack(HSQUIRRELVM v, const char *msg) {
    SQInteger i;
    SQInteger nargs = sq_gettop(v);
    const SQChar *s;
    printf("-- Stack %s\n",msg);
    int n;
    for(n=1;n<=nargs;n++) {
        sq_tostring(v,n);
        sq_getstring(v,-1,&s);
        printf("Arg: %d -- %s\n",n,s);
        sq_pop(v,1);
    }
}

void print_table(HSQUIRRELVM v, SQInteger idx, const char *name) {
    printf("-- Table: %s\n",name);
    idx = (idx < 0) ? sq_gettop(v) + idx + 1 : idx;
    sq_pushnull(v);
    sds key,val;
    while(SQ_SUCCEEDED(sq_next(v,idx))) {
        key = print_sqobject(v,-2);
        val = print_sqobject(v,-1);
        printf("%s: %s\n",key,val);
        sdsfree(key);
        sdsfree(val);
        sq_pop(v,2);
    }
    sq_pop(v,1);

}

void sqrun(HSQUIRRELVM v,sds buffer) {
    SQInteger _top = sq_gettop(v);
    if (SQ_SUCCEEDED(sq_compilebuffer(v,buffer,sdslen(buffer),_SC("cli"),SQTrue))) {
        sq_pushroottable(v);
        if (SQ_SUCCEEDED(sq_call(v,1,SQTrue,SQTrue))) {
            SQObjectType t = sq_gettype(v,-1);
            if (t != OT_NULL) {
                sds result = print_sqobject(v,-1);
                printf("%s\n", result);
                sdsfree(result);
            }
        }
        sq_settop(v,_top);
    }
}

int statement_complete(sds buffer) {
    int string = 0;
    int block = 0;
    size_t i = 0;
    for (i=0;i<sdslen(buffer);++i) {
        switch (buffer[i]) {
            case '{':
                if (!string) {
                    ++block;
                }
                break;
            case '}':
                if (!string) {
                    --block;
                }
                break;
            case '\'':
            case '"':
                if ((i>0 && (buffer[i-1] != '\\')) || (i == 0)) {
                    string = !string;
                }
                break;
        }
    }
    return block == 0 && string == 0;
}

#define READ_BUF 65536
#define CMD_HISTORY 1024

void cli(HSQUIRRELVM v) {
	
    char *ps1 = "sq> ";
    char *ps2 = "... ";
    char *prompt = ps1;
    char *line;
    sds cmd_history[CMD_HISTORY];
    int cmd_history_idx = 0;
    sds buffer = sdsempty();

    while((line = linenoise(prompt)) != NULL) {

        linenoiseHistoryAdd(line);

        if (strcmp(line,".quit") == 0) {
            break;
        } else if (strncmp(line,".root",6) == 0) {
            sq_pushroottable(v);
            print_table(v,-1,"Root");
            sq_pop(v,1);
        } else if (strncmp(line,".edit",5) == 0) {
            int fd;
            size_t n;
            char *tmpbuf[READ_BUF];
            int n_args;
            sds *args = sdssplitargs(line,&n_args);
            sds template = sdsnew("/tmp/sqXXXXXX");
            sds cmd = sdsempty();
            if ((fd = mkstemp(template)) != -1) {
                if (n_args > 1) {
                    unsigned long n_hist = strtoul(args[1],(char **) NULL,10);
                    if (n_hist < cmd_history_idx) {
                        size_t n_write = 0;
                        while (n_write < sdslen(cmd_history[n_hist])) {
                            n_write += write(fd,cmd_history[n_hist] + n_write,
                                                sdslen(cmd_history[n_hist]) - n_write);
                        }
                    }
                }
                fsync(fd);
                lseek(fd,0,0);
                cmd = sdscatprintf(cmd,"${EDITOR-vi} %s", template);
                system(cmd);
                while((n = read(fd,tmpbuf,READ_BUF)) > 0) {
                    buffer = sdscatlen(buffer,tmpbuf,n);
                }
                close(fd);
                unlink(template);
                sqrun(v,buffer);
                if (sdslen(buffer) > 0) {
                    cmd_history[cmd_history_idx++] = buffer;
                    buffer = sdsempty();
                }
            }
        } else if (strncmp(line,".history",8) == 0) {
            int i = 0;
            for (i=0;i<cmd_history_idx;i++) {
                printf("[%d]\n    ",i);
                int n_lines;
                sds *lines = sdssplitlen(cmd_history[i],
                                         sdslen(cmd_history[i]),
                                         "\n",1,&n_lines);
                sds out = sdsjoinsds(lines,n_lines,"\n    ",5);
                printf("%s\n",out);
                sdsfree(out);
                sdsfreesplitres(lines,n_lines);
            }
        } else if (strncmp(line,".save",5) == 0) {
        } else {
            buffer = sdscat(buffer,line);
            if (statement_complete(buffer)) {
                if (buffer[0] == '=') {
                    sds temp = sdscatprintf(sdsempty(),"return(%s)",buffer+1);
                    sdsfree(buffer);
                    buffer = temp;
                }
                sqrun(v,buffer);
                if (sdslen(buffer) > 0) {
                    cmd_history[cmd_history_idx++] = buffer;
                    buffer = sdsempty();
                }
                prompt = ps1;
            } else {
                buffer = sdscat(buffer,"\n");
                prompt = ps2;
            }
        }
        free(line);
    }
    sdsfree(buffer);
}

int main(int argc, char* argv[])
{
	HSQUIRRELVM v;
	
	v=sq_open(1024);

	sq_setprintfunc(v,printfunc,errorfunc);
	sq_pushroottable(v);

	//sqstd_register_bloblib(v);
	//sqstd_register_iolib(v);
	//sqstd_register_systemlib(v);
	//sqstd_register_mathlib(v);
	//sqstd_register_stringlib(v);

	sqstd_seterrorhandlers(v);

    cli(v);
	sq_close(v);
	
	return 0;
}

