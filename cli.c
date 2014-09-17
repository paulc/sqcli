/*	see copyright notice in squirrel.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

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

#include "linenoise.h"

typedef struct {
    SQObjectType t;
    const char *s;
} _SQTypeMap;

static _SQTypeMap _sqtypemap[] = {
    {OT_NULL,"null"}, {OT_BOOL,"bool"}, {OT_INTEGER,"integer"},
    {OT_FLOAT,"float"}, {OT_STRING,"string"}, {OT_TABLE,"table"},
    {OT_ARRAY,"array"}, {OT_USERDATA,"userdata"}, {OT_CLOSURE,"closure"},
    {OT_NATIVECLOSURE,"native closure"}, {OT_GENERATOR,"generator"},
    {OT_USERPOINTER,"userpointer"}, {OT_CLASS,"class"},
    {OT_INSTANCE,"instance"}, {OT_WEAKREF,"weak reference"}
};

const char *print_type(HSQUIRRELVM v,SQInteger idx) {
    SQObjectType t = sq_gettype(v,idx);
    int i = 0;
    for (i=0; i<(sizeof(_sqtypemap)/sizeof(_SQTypeMap));++i) {
        if (_sqtypemap[i].t == t) {
            return _sqtypemap[i].s;
        }
    }
    return "unknown type";
}

const char *print_object(HSQUIRRELVM v,SQInteger idx) {
    const SQChar *s;
    char *r;
    sq_tostring(v,idx);
    sq_getstring(v,-1,&s);
    sq_pop(v,1);
    return s;
}

void print_stack(HSQUIRRELVM v, const char *msg) {
    SQInteger i;
    SQInteger nargs = sq_gettop(v);
    char *s;
    printf("-- Stack %s\n",msg);
    for(SQInteger n=1;n<=nargs;n++) {
        printf("Arg: %d -- %s\n",n,print_object(v,n));
    }
}

void print_table(HSQUIRRELVM v, SQInteger idx, const char *name) {
    printf("-- Table %s\n",name);
    idx = (idx < 0) ? sq_gettop(v) + idx + 1 : idx;
    sq_pushnull(v);
    while(SQ_SUCCEEDED(sq_next(v,idx))) {
        printf("%s: %s\n",print_object(v,-2),print_object(v,-1));
        sq_pop(v,2);
    }
    sq_pop(v,1);

}

void printfunc(HSQUIRRELVM v,const SQChar *s,...)
{
	va_list vl;
	va_start(vl, s);
	scvprintf(stdout, s, vl);
	va_end(vl);
}

void errorfunc(HSQUIRRELVM v,const SQChar *s,...)
{
	va_list vl;
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
}

void dump(char *buf,int len) {
    int i=0;
    for (i=0; i<len; i++) {
        if (isalnum(buf[i])) {
            printf("%c",buf[i]);
        } else {
            printf("\\%02x",buf[i]);
        }
    }
    printf("\n");
}

void run(HSQUIRRELVM v,char *buffer,int size) {
    SQInteger _top = sq_gettop(v);
    sq_compilebuffer(v,buffer,size,_SC("cli"),SQTrue);
    sq_pushroottable(v);
    sq_call(v,1,SQTrue,SQTrue);
    print_stack(v,"(call)");
    sq_settop(v,_top);
}

char *trimws(char *s) {
    char *start = s;
    char *end = start + strlen(start) - 1;;
    while (isspace(*start)) start++;
    if (*start == 0) return start;
    while (end > start && isspace(*end)) end--;
    *(end+1) = 0;
    return start;
}

void cli(HSQUIRRELVM v) {
	
    char *ps1 = "sq> ";
    char *ps2 = "... ";
    char *prompt = ps1;
    char *line;
    int buffer_length = 1;
    char *buffer = malloc(buffer_length);
    char buffer_pos = 0;
    size_t line_length;
    int string = 0;
    int block = 0;
    int cont = 0;
    int i = 0;

    while((line = linenoise(prompt)) != NULL) {

        line_length = strlen(line);

        if (strcmp(line,".quit") == 0) {
            free(line);
            break;
        }

        if (strcmp(line,".stack") == 0) {
            print_stack(v,"");
            continue;
        }

        if (strcmp(line,".root") == 0) {
            sq_pushroottable(v);
            print_table(v,-1,"root");
            sq_pop(v,1);
            continue;
        }

        if (strncmp(line,".save",5) == 0) {
            char *fname = trimws(line+5);
            if (strlen(fname)==0) {
                printf(".save <file>\n");
            } else {
                linenoiseHistorySave(fname);
                printf("Saved history: '%s'\n",fname);
            }
            free(line);
            continue;
        }

        linenoiseHistoryAdd(line);
        while ((buffer_pos + line_length + 2) > buffer_length) {
            buffer_length = buffer_length * 2;
            buffer = realloc(buffer,buffer_length);
        }

        for (i=0;i<line_length;++i) {
            switch (line[i]) {
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
                    if ((i>0 && (line[i-1] != '\\')) || (i == 0)) {
                        string = !string;
                    }
                    break;
            }
        }

        if (line[line_length-1] == '\\') {
            snprintf(buffer+buffer_pos,line_length,"%s",line);
            buffer_pos = buffer_pos + line_length - 1;
            cont = 1;
        } else {
            snprintf(buffer+buffer_pos,line_length+2,"%s\n",line);
            buffer_pos = buffer_pos + line_length + 1;
            cont = 0;
        }

        free(line);

        if (block == 0 && cont == 0 && string == 0) {
            buffer[--buffer_pos] = 0;
            printf("Run: >>%s<<\n",buffer);
            run(v,buffer,buffer_pos);
            prompt = ps1;
            buffer_pos = 0;
        } else {
            prompt = ps2;
        }
    }
    free(buffer);
}

int main(int argc, char* argv[])
{
	HSQUIRRELVM v;
	SQInteger retval = 0;
	const SQChar *filename=NULL;
	
	v=sq_open(1024);

	sq_setprintfunc(v,printfunc,errorfunc);
	sq_pushroottable(v);

	sqstd_register_bloblib(v);
	sqstd_register_iolib(v);
	sqstd_register_systemlib(v);
	sqstd_register_mathlib(v);
	sqstd_register_stringlib(v);

	//aux library
	//sets error handlers
	sqstd_seterrorhandlers(v);

    cli(v);
	sq_close(v);
	
	return retval;
}

