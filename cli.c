/*	see copyright notice in squirrel.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

SQInteger quit(HSQUIRRELVM v)
{
	int *done;
	sq_getuserpointer(v,-1,(SQUserPointer*)&done);
	*done=1;
	return 0;
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

#include <ctype.h>

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

void run(HSQUIRRELVM v,char *buffer,int size,int retval) {
    SQInteger oldtop=sq_gettop(v);
    if(SQ_SUCCEEDED(sq_compilebuffer(v,buffer,size,_SC("interactive console"),SQTrue))){
        sq_pushroottable(v);
        if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue)) &&	retval){
            scprintf(_SC("\n"));
            sq_pushroottable(v);
            sq_pushstring(v,_SC("print"),-1);
            sq_get(v,-2);
            sq_pushroottable(v);
            sq_push(v,-4);
            sq_call(v,2,SQFalse,SQTrue);
            retval=0;
            scprintf(_SC("\n"));
        }
    } else {
        printf("Error compiling");
    }
    sq_settop(v,oldtop);
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
            run(v,buffer,buffer_pos,1);
            prompt = ps1;
            buffer_pos = 0;
        } else {
            prompt = ps2;
        }
    }
    free(buffer);
    /*
#define MAXINPUT 1024
	SQChar buffer[MAXINPUT];
	SQInteger blocks =0;
	SQInteger string=0;
	SQInteger retval=0;
	SQInteger done=0;
		
    while (!done) 
	{
		SQInteger i = 0;
		scprintf(_SC("\nsq>"));
		for(;;) {
			int c;
			if(done)return;
			c = getchar();
			if (c == _SC('\n')) {
				if (i>0 && buffer[i-1] == _SC('\\'))
				{
					buffer[i-1] = _SC('\n');
				}
				else if(blocks==0)break;
				buffer[i++] = _SC('\n');
			}
			else if (c==_SC('}')) {blocks--; buffer[i++] = (SQChar)c;}
			else if(c==_SC('{') && !string){
					blocks++;
					buffer[i++] = (SQChar)c;
			}
			else if(c==_SC('"') || c==_SC('\'')){
					string=!string;
					buffer[i++] = (SQChar)c;
			}
			else if (i >= MAXINPUT-1) {
				scfprintf(stderr, _SC("sq : input line too long\n"));
				break;
			}
			else{
				buffer[i++] = (SQChar)c;
			}
		}
		buffer[i] = _SC('\0');
		
		if(buffer[0]==_SC('=')){
			scsprintf(sq_getscratchpad(v,MAXINPUT),_SC("return (%s)"),&buffer[1]);
			memcpy(buffer,sq_getscratchpad(v,-1),(scstrlen(sq_getscratchpad(v,-1))+1)*sizeof(SQChar));
			retval=1;
		}
		i=scstrlen(buffer);
		if(i>0){
			SQInteger oldtop=sq_gettop(v);
			if(SQ_SUCCEEDED(sq_compilebuffer(v,buffer,i,_SC("interactive console"),SQTrue))){
				sq_pushroottable(v);
				if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue)) &&	retval){
					scprintf(_SC("\n"));
					sq_pushroottable(v);
					sq_pushstring(v,_SC("print"),-1);
					sq_get(v,-2);
					sq_pushroottable(v);
					sq_push(v,-4);
					sq_call(v,2,SQFalse,SQTrue);
					retval=0;
					scprintf(_SC("\n"));
				}
			}
			
			sq_settop(v,oldtop);
		}
	}
*/
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

