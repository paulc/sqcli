/*	see copyright notice in squirrel.h */

#include <ctype.h>
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

char *print_object(HSQUIRRELVM v,SQInteger idx) {
    const SQChar *s;
    idx = (idx < 0) ? sq_gettop(v) + idx + 1 : idx;
    SQObjectType t = sq_gettype(v,idx);
    if (t == OT_ARRAY || t == OT_TABLE) {
        int buffer_length = 1024;
        int buffer_pos = 0;
        char *buffer = malloc(buffer_length);
        char *key,*val;
        buffer_pos += snprintf(buffer,buffer_length-buffer_pos,
                                (t == OT_ARRAY) ? "[" : "{");
        sq_pushnull(v);
        while(SQ_SUCCEEDED(sq_next(v,idx))) {
            key = print_object(v,-2);
            val = print_object(v,-1);
            if ((strlen(key) + strlen(val) + 4) > (buffer_length - buffer_pos)) {
                buffer = realloc(buffer,buffer_length * 2);
            }
            if (t == OT_ARRAY) {
                buffer_pos += snprintf(buffer+buffer_pos,buffer_length-buffer_pos,
                                            "%s, ",val);
            } else {
                buffer_pos += snprintf(buffer+buffer_pos,buffer_length-buffer_pos,
                                            "%s: %s, ",key,val);
            }
            free(key);
            free(val);
            sq_pop(v,2);
        }
        buffer_pos -= 2;
        snprintf(buffer+buffer_pos,buffer_length-buffer_pos,
                                (t == OT_ARRAY) ? "]" : "}");
        sq_pop(v,1);
        return buffer;
    } else {
        sq_tostring(v,idx);
        sq_getstring(v,-1,&s);
        char *r = strdup(s);
        sq_pop(v,1);
        return r;
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
    printf("-- Table %s\n",name);
    idx = (idx < 0) ? sq_gettop(v) + idx + 1 : idx;
    sq_pushnull(v);
    char *key,*val;
    while(SQ_SUCCEEDED(sq_next(v,idx))) {
        key = print_object(v,-2);
        val = print_object(v,-1);
        printf("%s: %s\n",key,val);
        free(key);
        free(val);
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
    char *result;
    if (SQ_SUCCEEDED(sq_compilebuffer(v,buffer,size,_SC("cli"),SQTrue))) {
        sq_pushroottable(v);
        if (SQ_SUCCEEDED(sq_call(v,1,SQTrue,SQTrue))) {
            print_stack(v,"(call)");
            result = print_object(v,-1);
            printf(">>%s\n", result);
            free(result);
        }
        sq_settop(v,_top);
    }
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
    int buffer_length = 1024;
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

        if (strcmp(line,".edit") == 0) {
            int fd;
            int n = 0;
            char template[32];
            char cmd[64];
            struct stat sb;
            strcpy(template,"/tmp/sqXXXXXX");
            if ((fd = mkstemp(template)) != -1) {
                while (n < buffer_pos) {
                    n += write(fd,buffer+n,buffer_pos-n);
                }
                snprintf(cmd,64,"${EDITOR-vi} %s", template);
                system(cmd);
                stat(template,&sb);
                char *out = malloc(sb.st_size+1);
                memset(out,0,sb.st_size+1);
                n = 0;
                while (n < sb.st_size) {
                    n += read(fd,out,sb.st_size-n);
                }
                unlink(template);
                printf(">>%s<< (%d)\n",out,strlen(out));
                free(line);
                line_length = strlen(out);
                line = out;
            }
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
            if (buffer[0] == '=') {
                char *tmp = malloc(buffer_pos+8);
                snprintf(tmp,buffer_pos+8,"return(%s)",buffer+1);
                free(buffer);
                buffer = tmp;
                buffer_pos += 7;
            }
            printf("Run: >>%s<< (%d)\n",buffer,buffer_pos);
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

