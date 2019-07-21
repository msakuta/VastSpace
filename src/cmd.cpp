#define _CRT_SECURE_NO_WARNINGS
#include "cmd.h"
#include "cmd_int.h"
#include "argtok.h"
#include "Viewer.h"
#include "sqadapt.h"
#include "Application.h"
extern "C"{
#include <clib/c.h>
#ifndef DEDICATED
#include <clib/gl/gldraw.h>
#endif
#include <clib/timemeas.h>
}
/*#include <GL/glut.h>*/
#include "antiglut.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

static struct viewport gvp;

cpplib::dstring cmdbuffer[CB_LINES];
int cmdcurline = 2;
int cmddispline = 0;

static cpplib::dstring cmdhist[MAX_COMMAND_HISTORY];
static int cmdcurhist = 0, cmdselhist = 0;

cpplib::dstring cmdline;

void (*CmdPrintHandler)(const char *line) = NULL;

static int cvar_echo = 0;
static int cvar_cmd_echo = 1;
static int console_pageskip = 8;
static int console_mousewheelskip = 4;
static int console_undefinedecho = 0;
static int cvar_cmdlog = 0;

static int CmdExecD(char *, bool server, ServerClient *);
struct cmdalias **CmdAliasFindP(const char *name);
static double cmd_sqcalc(const char *str, const SQChar *context = _SC("expression"));

/* binary tree */
static struct command{
	union commandproc{
		int (*a)(int argc, char *argv[]);
		int (*p)(int argc, char *argv[], void *);
		int (*s)(int argc, char *argv[], ServerClient *);
	} proc;
	int type;
	void *param;
	const char *name;
	struct command *left, *right;
} *cmdlist = NULL;
static int cmdlists = 0;


#define CVAR_BUCKETS 53

/* hash table */
static struct cvar *cvarlist[CVAR_BUCKETS] = {NULL}, *cvarlinear = NULL;
static int cvarlists = 0;

/* binary tree */
static struct cmdalias{
	char *name;
	struct cmdalias *left, *right;
	char str[1];
} *aliaslist = NULL;

static unsigned long hashfunc(const char *s){
	int i = 0;
	unsigned long ret = 0;
	while(*s){
		ret ^= (*s << (7 * i) % 32) | (*s >> (32 - 7 * i % 32));
		s++;
		i++;
	}
	return ret;
}


static FILE *logfp(){
	static FILE *fp = NULL;
	if(!fp)
		fp = fopen("cmdlog.log", "w");
	return fp;
}

/// Directly assign dstring to command buffer.
/// This overloaded version reallocates memory less times.
void CmdPrint(const cpplib::dstring &str){
	cmdbuffer[cmdcurline] = str;
	if(cvar_cmdlog)
		fprintf(logfp(), "%s\n", (const char*)str);

	// Windows default encoding for multibyte string is often not UTF-8, so we
	// convert it with wide char functions.
#ifdef _WIN32
	// This buffer is reused every time this function is called for speed,
	// meaning it will be left allcated when the program exits, which is
	// usually freed by OS instead.
	static wchar_t *wbuf = NULL;
	static size_t wbufsiz = 0;
	int wcc = MultiByteToWideChar(CP_UTF8, 0, str, str.len(), NULL, 0);
	if(wbufsiz < wcc){
		wbuf = (wchar_t*)realloc(wbuf, sizeof(*wbuf) * (wcc + 1));
		wbufsiz = wcc;
	}
	MultiByteToWideChar(CP_UTF8, 0, str, str.len(), wbuf, wbufsiz);
	wbuf[wcc] = '\0';

	// And the multibyte string buffer.
	// It should not be necessary if wprintf worked, but it didn't.
	static char *buf = NULL;
	static size_t bufsiz = 0;
	int cc = WideCharToMultiByte(CP_THREAD_ACP, 0, wbuf, wbufsiz, NULL, 0, NULL, NULL);
	if(bufsiz < cc){
		buf = (char*)realloc(buf, sizeof(*buf) * (cc + 1));
		bufsiz = cc;
	}
	WideCharToMultiByte(CP_THREAD_ACP, 0, wbuf, wbufsiz, buf, bufsiz, NULL, NULL);
	buf[cc] = '\0';

	if(CmdPrintHandler){
		CmdPrintHandler(buf);
	}

	printf("%s\n", buf);
#else
	puts(cmdbuffer[cmdcurline]);
#endif
	cmdcurline = (cmdcurline + 1) % CB_LINES;
}

/// Print a string to console.
void CmdPrint(const char *str){
	CmdPrint(static_cast<const cpplib::dstring&>(cpplib::dstring(str)));
}

/// Formatted version of CmdPrint.
/// TODO: Buffer overrun unsafe!
void CmdPrintf(const char *str, ...){
	char buf[512];
	va_list args;
	va_start(args, str);
	vsprintf(buf, str, args);
	va_end(args);
	CmdPrint(buf);
}


static int cmd_echo(int argc, char *argv[]){
	cpplib::dstring &out = cmdbuffer[cmdcurline];
	if(argc <= 1)
		return 0;
	out = "";
	char **argend = &argv[argc];
	for(argv++; argend != argv && *argv; argv++){
		out << *argv << ' ';
	}
	puts(cmdbuffer[cmdcurline]);
	cmdcurline = (cmdcurline + 1) % CB_LINES;
	return 0;
}


static int cmd_cmdlist_int(const struct command *c, const char *pattern, int level){
	int ret = 0;
	if(c->right)
		ret += cmd_cmdlist_int(c->right, pattern, level + 1);
	if(!pattern || !strncmp(pattern, c->name, strlen(pattern))){
#ifdef _DEBUG
		CmdPrint(cpplib::dstring() << level << ": " << c->name);
#else
		CmdPrint(c->name);
#endif
		ret++;
	}
	if(c->left)
		ret += cmd_cmdlist_int(c->left, pattern, level + 1);
	return ret;
}

static int cmd_cmdlist(int argc, char *argv[]){
	int c;
	c = cmd_cmdlist_int(cmdlist, 2 <= argc ? argv[1] : NULL, 0);
	CmdPrint(cpplib::dstring() << c << " commands listed");
	return 0;
}

static int cmd_cvarlist(int argc, char *argv[]){
	static const char typechar[] = {
		'i', 'f', 'd', 's'
	};
	int c;
	struct cvar *cv;
	c = 0;
	for(cv = cvarlinear; cv; cv = cv->linear) if(argc < 2 || !strncmp(argv[1], cv->name, strlen(argv[1]))){
#ifdef _DEBUG
		char buf[32];
		sprintf(buf, "%08X", hashfunc(cv->name));
		CmdPrint(cpplib::dstring() << typechar[cv->type] << ": " << cv->name << " (" << buf << ")");
#else
		CmdPrint(cpplib::dstring() << typechar[cv->type] << ": " << cv->name);
#endif
		c++;
	}
	CmdPrint(cpplib::dstring() << c << " cvars listed");
	return 0;
}

static int cmd_toggle(int argc, char *argv[]){
	struct cvar *cv;
	const char *arg = argv[1];
	if(!arg){
		CmdPrint("Specify a integer cvar to toggle it, assuming flag-like usage.");
		return 0;
	}
	if((cv = CvarFind(arg)) && cv->type == cvar_int){
		*cv->v.i = !*cv->v.i;
		if(cv->vrc)
			cv->vrc(cv->v.i);
	}
	else
		CmdPrint("Specified variable is either not a integer nor existing.");
	return 0;
}

static int cmd_inc(int argc, char *argv[]){
	struct cvar *cv;
	const char *arg = argv[1];
	if(!arg){
		CmdPrint("Specify a integer cvar to increment by 1.");
		return 0;
	}
	if((cv = CvarFind(arg)) && cv->type == cvar_int){
		++*cv->v.i;
		if(cv->vrc)
			cv->vrc(cv->v.i);
	}
	else
		CmdPrint("Specified variable is either not a integer nor existing.");
	return 0;
}

static int cmd_dec(int argc, char *argv[]){
	struct cvar *cv;
	const char *arg = argv[1];
	if(!arg){
		CmdPrint("Specify a integer cvar to decrement by 1.");
		return 0;
	}
	if((cv = CvarFind(arg)) && cv->type == cvar_int){
		--*cv->v.i;
		if(cv->vrc)
			cv->vrc(cv->v.i);
	}
	else
		CmdPrint("Specified variable is either not a integer nor existing.");
	return 0;
}

static int cmd_mul(int argc, char *argv[]){
	struct cvar *cv, *cv2;
	const char *arg = argv[1];
	if(argc <= 2){
		CmdPrint("Specify an arithmetic cvar and a constant or 2 cvars to multiply.");
		return 0;
	}
	if((cv = CvarFind(arg))){
		double val;
		if((cv2 = CvarFind(argv[2]))) switch(cv2->type){
			case cvar_int:
				val = *cv->v.i;
				break;
			case cvar_float:
				val = *cv->v.f;
				break;
			case cvar_double:
				val = *cv->v.d;
				break;
		}
		else
			val = atof(argv[2]);
		switch(cv->type){
			case cvar_int:
				*cv->v.i *= (int)val;
				break;
			case cvar_float:
				*cv->v.f *= (float)val;
				break;
			case cvar_double:
				*cv->v.d *= val;
				break;
		}
		if(cv->vrc)
			cv->vrc(cv->v.i);
	}
	else
		CmdPrint("Specified variable is either not a integer nor existing.");
	return 0;
}


static char *stringdup(const char *s){
	char *ret;
	ret = (char*)malloc(strlen(s) + 1);
	strcpy(ret, s);
	return ret;
}

/// explicit cvar setter, useful in some occasion.
int cmd_set(int argc, char *argv[]){
	const char *thekey, *thevalue;
	if(argc <= 1){
		CmdPrint("Specify a cvar to set it.");
		return 0;
	}
	thekey = argv[1];
	if(argc <= 2){
		CmdPrint("Specify a value to set.");
		return 0;
	}
	thevalue = argv[2];
/*	else if(strncpy(args, arg, sizeof args), !(thekey = strtok(args, " \t")))
		return 0;
	else if(thevalue = strtok(NULL, ""))*/{
		struct cvar *cv;
		if(cv = CvarFind(thekey)) switch(cv->type){
			case cvar_int: *cv->v.i = atoi(thevalue); break;
			case cvar_float: *cv->v.f = (float)cmd_sqcalc(thevalue); break;
			case cvar_double: *cv->v.d = cmd_sqcalc(thevalue); break;
			case cvar_string: cv->v.s = (char*)realloc(cv->v.s, strlen(thevalue) + 1); strcpy(cv->v.s, thevalue); break;
		}
		else{
			CvarAdd(stringdup(thekey), stringdup(thevalue), cvar_string);
		}
	}
/*	else
		cmd_echoa("Specify value to set.");*/
	return 0;
}

static void cmd_preprocess(char *out, const char *in){
	char *dollar = NULL;
	do{
		if(*in == '$')
			dollar = out;
		if(dollar){
			if(out != dollar && (!isalnum(*in) && *in != '_')){
				struct cvar *cv;
				*out = '\0';
				cv = CvarFind(dollar+1);
				if(cv) switch(cv->type){
					case cvar_int: sprintf(dollar, "%d", *cv->v.i); break;
					case cvar_float: sprintf(dollar, "%f", *cv->v.f); break;
					case cvar_double: sprintf(dollar, "%lf", *cv->v.d); break;
					case cvar_string: strcpy(dollar, cv->v.s); break;
				}
				out = strchr(dollar, '\0');
				dollar = NULL;
			}
		}
		*out++ = *in;
	}while(*in++);
	*out = '\0';
}

static int cmd_exec(int argc, char *argv[]){
	if(argc <= 1){
		CmdPrint("Specify a file as argument to load the file to batch execute commands.");
		return 0;
	}
	FILE *fp = fopen(argv[1], "r");
	if(!fp){
		CmdPrint(cpplib::dstring() << "Couldn't load " << argv[1] << "!");
		return 0;
	}
	CmdPrint(cpplib::dstring() << "executing " << argv[1]);
	cpplib::dstring buf;
	int c;
	while(EOF != (c = fgetc(fp))) if(c == '\\'){
		char lc = c;
		if(EOF == (c = fgetc(fp)))
			break;
		if(c != '\n') // Only escape newlines; leave other escape sequnces to the preprocessor.
			buf << char(lc) << char(c);
	}
	else if(c == '\n'){
		char *linebuf = new char[buf.len() + 2];
		cmd_preprocess(linebuf, buf);
		CmdExec(linebuf);
		delete[] linebuf;
		buf = "";
	}
	else buf << char(c);

	// Process the last line without a newline.
	char *linebuf = new char[buf.len() + 2];
	cmd_preprocess(linebuf, buf);
	CmdExec(linebuf);
	delete[] linebuf;

	fclose(fp);
	return 0;
}

static int cmd_time(int argc, char *argv[]){
	int i;
	size_t valuelen;
	timemeas_t tm;
	if(argc <= 1){
		CmdPrint("Measures time elapsed while executing commands.");
		return 0;
	}
	for(valuelen = 0, i = 1; i < argc; i++)
		valuelen += strlen(argv[i]) + 1;
	char *thevalue = (char*)malloc(valuelen);
	for(valuelen = 0, i = 1; i < argc; i++){
		strcpy(&thevalue[valuelen], argv[i]);
		valuelen += strlen(argv[i]) + 1;
		if(i != argc - 1)
			thevalue[valuelen-1] = ' ';
	}
	TimeMeasStart(&tm);
	CmdExecD(thevalue, false, NULL);
	CmdPrint(cpplib::dstring() << TimeMeasLap(&tm) << " seconds");
	free(thevalue);
	return 0;
}

static int listalias(struct cmdalias *a, int level){
	int ret = 0;
	if(!a)
		return 0;
	ret += listalias(a->right, level + 1);
	{
#ifdef _DEBUG
		CmdPrint(cpplib::dstring() << "(" << level << ") " << a->name << ": " << a->str);
#else
		CmdPrint(cpplib::dstring() << a->name << ": " << a->str);
#endif
	}
	ret += listalias(a->left, level + 1);
	return ret + 1;
}

static int cmd_alias(int argc, char *argv[]){
	char *thekey, *thevalue;
	int i;
	size_t valuelen;
	if(argc <= 1){
		int ret;
		ret = listalias(aliaslist, 0);
		CmdPrint(cpplib::dstring() << ret << " aliases listed");
		return 0;
	}
	else if(argc == 2){
/*	else if(strncpy(args, arg, sizeof args), !(thekey = strtok(args, " \t")))
		return 0;
	else if(!(thevalue = strtok(NULL, ""))){*/
		struct cmdalias *a;
		thekey = argv[1];
		if(a = CmdAliasFind(thekey)){
			CmdPrint(cpplib::dstring() << a->name << ": " << a->str);
			return 0;
		}
		CmdPrint(cpplib::dstring() << "no aliase named " << thekey << " is found.");
		return -1;
	}
	thekey = argv[1];
	for(valuelen = 0, i = 2; i < argc; i++)
		valuelen += strlen(argv[i]) + 1;
	thevalue = (char*)malloc(valuelen);
	for(valuelen = 0, i = 2; i < argc; i++){
		strcpy(&thevalue[valuelen], argv[i]);
		valuelen += strlen(argv[i]) + 1;
		if(i != argc - 1)
			thevalue[valuelen-1] = ' ';
	}
	CmdAliasAdd(thekey, thevalue);
	free(thevalue);
	return 1;
}

#if 0 /* btree deletion is somewhat complex */
static int cmd_unalias(const char *arg){
	struct cmdalias **pp;
	if(!arg){
		cmd_echoa("Specify alias name to undefine.");
		return 0;
	}
	if((pp = CmdAliasFindP(arg)) && *pp){
		struct cmdalias *next;
		if(!(*pp)->left)
			next = (*pp)->right;
		else if(!(*pp)->right)
			next = (*pp)->left;
		else
			next = strcmp((*pp)->left, (*pp)->right) < 0 ? (*pp)->
		free(*pp);
		*pp = next;
		return 1;
	}
	else{
		cmd_echoa("Specified alias name couldn't be found.");
	}
}
#endif

size_t cmd_memory_alias(const struct cmdalias *p){
	size_t ret = sizeof *p;
	if(!p)
		return 0;
	ret += strlen(p->name) + strlen(p->str);
	ret += cmd_memory_alias(p->left);
	ret += cmd_memory_alias(p->right);
	return ret;
}

/* print consumed memory for console management */
static int cmd_memory(int argc, char *argv[]){
	size_t size = 0;
	for(int i = 0; i < numof(cmdbuffer); i++)
		size += cmdbuffer[i].len();
	size += sizeof cmdbuffer + sizeof cmdhist;
	CmdPrint(cpplib::dstring() << "cmdbuf: " << size << " bytes = " << (size + 1023) / 1024 << " kilobytes used");
	size = cmd_memory_alias(aliaslist);
	CmdPrint(cpplib::dstring() << "alias: " << size << " bytes = " << (size + 1024) / 1024 << " kilobytes used");
	CmdPrint(cpplib::dstring() << "cvar: " << cvarlists * sizeof **cvarlist << " bytes = " << cvarlists * sizeof **cvarlist / 1024 << " kilobytes used");
#ifndef DEDICATED
	size = CircleCutsMemory();
	CmdPrint(cpplib::dstring() << "circut: " << size << " bytes = " << (size + 1023) / 1024 << " kilobytes used");
#endif
	return 0;
}


void CmdInit(struct viewport *pvp){
	gvp = *pvp;
	CmdAdd("echo", cmd_echo);
	CmdAdd("cmdlist", cmd_cmdlist);
	CmdAdd("cvarlist", cmd_cvarlist);
	CmdAdd("toggle", cmd_toggle);
	CmdAdd("inc", cmd_inc);
	CmdAdd("dec", cmd_dec);
	CmdAdd("mul", cmd_mul);
	CmdAdd("set", cmd_set);
	CmdAdd("exec", cmd_exec);
	CmdAdd("time", cmd_time);
	CmdAdd("alias", cmd_alias);
/*	CmdAdd("unalias", cmd_unalias);*/
	CmdAdd("memory", cmd_memory);
	CvarAdd("cvar_echo", &cvar_echo, cvar_int);
	CvarAdd("cmd_echo", &cvar_cmd_echo, cvar_int);
	CvarAdd("console_mousewheelskip", &console_mousewheelskip, cvar_int);
	CvarAdd("console_pageskip", &console_pageskip, cvar_int);
	CvarAdd("console_undefinedecho", &console_undefinedecho, cvar_int);
	CvarAdd("cmdlog", &cvar_cmdlog, cvar_int);
	CmdPrint("gltestplus running.");
	CmdPrint("build: " __DATE__ ", " __TIME__);
}

#define DELETEKEY 0x7f

int CmdInput(char key){
	static char last = '\0';
	if(key == DELETEKEY)
		return 1;
	if(key == 0x08){
		cmdline = cpplib::dstring().strncat(cmdline, cmdline.len() - 1);
		return 0;
	}
	else if(key == '\n'){
		return 0;
	}
	else if(key == '\n' && last == '\r' || key == '\r'){
		if(cmdline.len()){
			int ret, echo;
			char *linebuf = new char[cmdline.len() + 2];
			cmdhist[cmdcurhist] = cmdline;
			cmdselhist = cmdcurhist = (cmdcurhist + 1) % MAX_COMMAND_HISTORY;
			echo = cvar_cmd_echo && cmdline[0] != '@';
			if(echo){
				cmdbuffer[cmdcurline] = cmdline;
				cmdcurline = (cmdcurline + 1) % CB_LINES;
			}
			cmd_preprocess(linebuf, cmdline);
			ret = CmdExecD(linebuf, false, NULL);
			cmdline = "";
#ifdef _WIN32
			if(console_cursorposdisp)
				cmddispline = 0;
#endif
			delete[] linebuf;
		}
	}
	else
		cmdline << char(key);
	last = key;
	return 0;
}

int CmdMouseInput(int button, int state, int x, int y){
	if(state != GLUT_UP)
		return 0;
	if(button == GLUT_WHEEL_UP){
		if(cmddispline + console_mousewheelskip <= CB_LINES - (int)(gvp.h * .8 * 2. / 30.))
			cmddispline += console_mousewheelskip;
		else
			cmddispline = CB_LINES - (int)(gvp.h * .8 * 2. / 30.);
	}
	if(button == GLUT_WHEEL_DOWN){
		if(0 <= cmddispline - console_mousewheelskip)
			cmddispline -= console_mousewheelskip;
		else
			cmddispline = 0;
	}
	return 0;
}

int CmdSpecialInput(int key){
	if(key == GLUT_KEY_UP || key == GLUT_KEY_DOWN){
		cmdline = cmdhist[cmdselhist = (cmdselhist + (key == GLUT_KEY_UP ? -1 : 1) + MAX_COMMAND_HISTORY) % MAX_COMMAND_HISTORY];
	}
	if(key == GLUT_KEY_PAGE_UP/* && cmddispline < CB_LINES - (int)(gvp.h * .8 * 2. / 30.)*/){
		if(cmddispline + console_pageskip <= CB_LINES - (int)(gvp.h * .8 * 2. / 30.))
			cmddispline += console_pageskip;
		else
			cmddispline = CB_LINES - (int)(gvp.h * .8 * 2. / 30.);
	}
	if(key == GLUT_KEY_PAGE_DOWN/* && 0 < cmddispline*/){
		if(0 <= cmddispline - console_pageskip)
			cmddispline -= console_pageskip;
		else
			cmddispline = 0;
	}
	return 0;
}

static char *grouping(char *str, char **post){
	char *s = str, *begin = str;
	int inquote = 0;
	*post = NULL;
	if(!str)
		return 0;
	for(; *s; s++) if(*s == '"'){
		if(!inquote){
			inquote = 1;
			begin = s+1;
		}
		else{
			inquote = 0;
			*s = '\0';
			*post = (s[1] ? s+1 : NULL);
			return begin;
		}
	}
	if(inquote)
		CmdPrint("Double quotes(\") doesn't match.");
	return str;
}

int argtok(char *argv[], char *s, char **post, int maxargs){
	int ret = 0;
	int inquote = 0, escape = 0;
	int head = 0;
	if(post)
		*post = NULL;
	if(!s)
		return 0;
	for(; *s; s++) if(escape){
		int len;
		escape = 0;
		len = strlen(s);
		memmove(s-1, s, len);
		s[len-1] = '\0';
		s--;
	}
	else if(*s == '\\'){
		escape = 1;
		if(!head){
			if(maxargs - 1 <= ret){
				argv[ret] = "\0";
				return ret;
			}
			argv[ret++] = s;
			head = 1;
		}
	}
	else if(*s == '"'){
		if(!inquote){
			inquote = 1;
			if(maxargs <= ret)
				return ret;
			argv[ret++] = s+1;
			head = 1;
		}
		else{
			head = inquote = 0;
			*s = '\0';
		}
	}
	else if(!inquote){
		if(*s == '#'){
			*s = '\0';
			break;
		}
		if(*s == ';'){
			*s = '\0';
			if(post)
				*post = s+1;
			break;
		}
		if(isspace(*s)){
			*s = '\0';
			head = 0;
		}
		else if(!head){
			if(maxargs <= ret)
				return ret;
			argv[ret++] = s;
			head = 1;
		}
	}
	if(inquote)
		CmdPrint("Double quotes(\") doesn't match.");
	argv[ret] = NULL;
	return ret;
}

static int aliasnest = 0;

static double cmd_sqcalc(const char *str, const SQChar *context){
	HSQUIRRELVM v = application.clientGame ? application.clientGame->sqvm : application.serverGame->sqvm;
	if(!v)
		return 0.;
	StackReserver st(v);
	gltestp::dstring dst = gltestp::dstring("return(") << str << ")";
	if(SQ_FAILED(sq_compilebuffer(v, dst, dst.len(), context, SQTrue))){
		CmdPrint(gltestp::dstring() << "expression error: " << context);
		return 0.;
	}
	sq_push(v, -2);
	if(SQ_FAILED(sq_call(v, 1, SQTrue, SQTrue))){
		CmdPrint(gltestp::dstring() << "evaluation error: " << context);
		return 0.;
	}
	SQFloat f;
	if(SQ_FAILED(sq_getfloat(v, -1, &f))){
		CmdPrint(gltestp::dstring() << "expression not convertible to float: " << context);
		return 0.;
	}
	return f;
}

/** destructive, i.e. cmdstring is modified by strtok or similar
 * method to tokenize. */
static int CmdExecD(char *cmdstring, bool server, ServerClient *sc){
	int ret = 0;
	struct command *pc;
	struct cmdalias *pa;
	char *cmd/*, *arg, *arg2*/, *post;
	do{
	char *argv[MAX_ARGC];
	int argc;
	int retval;
	argc = argtok(argv, cmdstring, &post, MAX_ARGC);
	cmd = argv[0];
/*	cmd = strtok(cmdstring, " \t@\n;");
	arg2 = strtok(NULL, "");
	arg = grouping(arg2, &post);
	if(arg2 == arg){
		arg = strtok(arg2, ";");
		post = strtok(NULL, "");
	}*/

	if(!cmd || !*cmd || *cmd == '#')
		continue;

	if(*cmd == '@'){
		if(cmd[1])
			cmd = ++argv[0];
		else
			memmove(argv, &argv[1], --argc * sizeof *argv);
	}

	/* aliases are searched before commands, allowing overrides of commands. */
	if(pa = CmdAliasFind(cmd)){
		int returned;
		if(aliasnest++ < MAX_ALIAS_NESTS){
			if(argv[1]){
				returned = CmdExec(cpplib::dstring(pa->str) << " " << argv[1]);
			}
			else
				returned = CmdExec(pa->str);
		}
		else{
			CmdPrint("Aliase stack overflow!");
			CmdPrint("Infinite loop is likely the case.");
			returned = -1;
		}
		aliasnest--;
		ret = returned;
		continue;
	}
	if(pc = CmdFind(cmd)){
		if(pc->type != 2)
			ret = (pc->type == 0 ? pc->proc.a(argc, argv) : pc->proc.p(argc, argv, pc->param));
		else if(server)
			ret = pc->proc.s(argc, argv, sc);
		continue;
	}

	/* Try Squirrel command layer. */
	if(sqa_console_command(argc, argv, &retval)){
		ret = retval;
		continue;
	}

	{
		struct cvar *cv;
		for(cv = cvarlist[hashfunc(cmd) % numof(cvarlist)]; cv; cv = cv->next) if(!strcmp(cv->name, cmd)){
			cpplib::dstring buf;
			char *arg = argv[1];
			if(!arg) switch(cv->type){
				case cvar_int: buf << "\"" << cmd << "\" is " << *cv->v.i; break;
				case cvar_float: buf << "\"" << cmd << "\" is " << *cv->v.f; break;
				case cvar_double: buf << "\"" << cmd << "\" is " << *cv->v.d; break;
				case cvar_string: buf << "\"" << cmd << "\" is " << cv->v.s; break;
			}
			else switch(cv->type){
				case cvar_int: *cv->v.i = atoi(arg); break;
				case cvar_float: *cv->v.f = (float)cmd_sqcalc(arg); break;
				case cvar_double: *cv->v.d = cmd_sqcalc(arg); break;
				case cvar_string: cv->v.s = (char*)realloc(cv->v.s, strlen(arg) + 1); strcpy(cv->v.s, arg); break;
			}
			if(cv->vrc)
				cv->vrc(cv->v.i);
			if(arg) switch(cv->type){
				case cvar_int: buf << "\"" << cmd << "\" set to " << *cv->v.i; break;
				case cvar_float: buf << "\"" << cmd << "\" set to " << *cv->v.f; break;
				case cvar_double: buf << "\"" << cmd << "\" set to " << *cv->v.d; break;
				case cvar_string: buf << "\"" << cmd << "\" set to " << cv->v.s; break;
			}
			if(cvar_echo)
				CmdPrint(buf);
			ret = 1;
			goto gcon;
		}
	}
	if(console_undefinedecho){
		CmdPrint(cpplib::dstring() << "Undefined command: " << cmd);
	}
gcon:;
	}while(cmdstring = post);
	return ret;
}

int CmdExec(const char *cmdstring){
	char *buf = new char[strlen(cmdstring) + 2];
	strcpy(buf, cmdstring);
	int ret = CmdExecD(buf, false, NULL);
	delete[] buf;
	return ret;
}


/// \brief The command executed in the server.
int ServerCmdExec(const char *cmdstring, ServerClient *sc){
	char *buf = new char[strlen(cmdstring) + 2];
	strcpy(buf, cmdstring);
	int ret = CmdExecD(buf, true, sc);
	delete[] buf;
	return ret;
}

void CmdAdd(const char *cmdname, int (*proc)(int, char*[])){
	struct command **pp, *p;
	int i;
	p = (struct command*)malloc(sizeof *cmdlist);
	++cmdlists;
	for(pp = &cmdlist; *pp; pp = (i < 0 ? &(*pp)->left : &(*pp)->right)) if(!(i = strcmp((*pp)->name, cmdname)))
		break;
	*pp = p;
	p->proc.a = proc;
	p->type = 0;
	p->name = cmdname;
	p->left = NULL;
	p->right = NULL;
}

void CmdAddParam(const char *cmdname, int (*proc)(int, char*[], void *), void *param){
	struct command **pp, *p;
	int i;
	p = (struct command*)malloc(sizeof *cmdlist);
	++cmdlists;
	for(pp = &cmdlist; *pp; pp = (i < 0 ? &(*pp)->left : &(*pp)->right)) if(!(i = strcmp((*pp)->name, cmdname)))
		break;
	*pp = p;
	p->proc.p = proc;
	p->type = 1;
	p->param = param;
	p->name = cmdname;
	p->left = NULL;
	p->right = NULL;
}

/// \brief Registers a cmdname as a server command and binds it with a callback function.
///
/// Might not necessary to share list of command strings with the client commands, because remote users
/// should not execute arbitrary client commands in the server.
void ServerCmdAdd(const char *cmdname, int (*proc)(int, char *[], ServerClient*)){
	struct command **pp, *p;
	int i;
	p = (struct command*)malloc(sizeof *cmdlist);
	++cmdlists;
	for(pp = &cmdlist; *pp; pp = (i < 0 ? &(*pp)->left : &(*pp)->right)) if(!(i = strcmp((*pp)->name, cmdname)))
		break;
	*pp = p;
	p->proc.s = proc;
	p->type = 2;
	p->param = NULL;
	p->name = cmdname;
	p->left = NULL;
	p->right = NULL;
}

#define profile 0

struct command *CmdFind(const char *name){
	struct command *p;
#if profile
	timemeas_t tm;
	TimeMeasStart(&tm);
#endif
	for(p = cmdlist; p; p = (strcmp(p->name, name) < 0 ? p->left : p->right)) if(!strcmp(p->name, name)){
		break;
	}
#if profile
	{
		static int invokes = 0;
		static double avg = 0.;
		double v;
		char buf[128];
		v = TimeMeasLap(&tm);
		sprintf(buf, "CmdFind: %lg, %lg\n", v, avg = (avg * invokes + v) / (invokes++ + 1));
		OutputDebugString(buf);
	}
#endif
	return p;
}

void CvarAdd(const char *cvarname, void *value, enum cvartype type){
	struct cvar *cv, **ppcv;
	cv = (struct cvar*)malloc(sizeof *cv);
	ppcv = &cvarlist[hashfunc(cvarname) % numof(cvarlist)];
	cv->next = *ppcv;
	*ppcv = cv;
	cv->type = type;
	cv->name = cvarname;
	cv->v.i = (int*)value;
	cv->vrc = NULL;
	cv->linear = cvarlinear;
	cvarlinear = cv;
	++cvarlists;
}

void CvarAddVRC(const char *cvarname, void *value, enum cvartype type, int (*vrc)(void*)){
	struct cvar *cv, **ppcv;
	cv = (struct cvar*)malloc(sizeof *cv);
	ppcv = &cvarlist[hashfunc(cvarname) % numof(cvarlist)];
	cv->next = *ppcv;
	*ppcv = cv;
	cv->type = type;
	cv->name = cvarname;
	cv->v.i = (int*)value;
	cv->vrc = vrc;
	cv->linear = cvarlinear;
	cvarlinear = cv;
	++cvarlists;
}

struct cvar *CvarFind(const char *cvarname){
	struct cvar *cv;
#if profile
	timemeas_t tm;
	TimeMeasStart(&tm);
#endif
	cv = cvarlist[hashfunc(cvarname) % numof(cvarlist)];
	for(; cv; cv = cv->next) if(!strcmp(cv->name, cvarname)){
		break;
	}
#if profile
	{
		static int invokes = 0;
		static double avg = 0.;
		double v;
		char buf[128];
		v = TimeMeasLap(&tm);
		sprintf(buf, "CvarFind: %lg, %lg\n", v, avg = (avg * invokes + v) / (invokes++ + 1));
		OutputDebugString(buf);
	}
#endif
	return cv;
}

const char *CvarGetString(const char *cvarname){
	struct cvar *cv;
	static char *buf;
	cv = CvarFind(cvarname);
	if(!cv)
		return NULL;
	switch(cv->type){
		case cvar_int: buf = (char*)realloc(buf, 64); sprintf(buf, "%d", *cv->v.i); break;
		case cvar_float: buf = (char*)realloc(buf, 64); sprintf(buf, "%f", *cv->v.f); break;
		case cvar_double: buf = (char*)realloc(buf, 64); sprintf(buf, "%lf", *cv->v.d); break;
		case cvar_string: return cv->v.s;
	}
	return buf;
}

void CmdAliasAdd(const char *name, const char *str){
	struct cmdalias **pp, *p;
	int i;
	for(pp = &aliaslist; *pp; pp = (i < 0 ? &(*pp)->left : &(*pp)->right)) if(!(i = strcmp((*pp)->name, name)))
		break;
	if(*pp){
		p = (struct cmdalias*)realloc(*pp, offsetof(struct cmdalias, str) + strlen(name) + strlen(str) + 2);
	}
	else{
		p = (struct cmdalias*)malloc(offsetof(struct cmdalias, str) + strlen(name) + strlen(str) + 2);
		p->left = NULL;
		p->right = NULL;
	}
	*pp = p;
	p->name = &p->str[strlen(str) + 1];
	strcpy(p->name, name);
	strcpy(p->str, str);
}

struct cmdalias *CmdAliasFind(const char *name){
	struct cmdalias *p;
	for(p = aliaslist; p; p = (strcmp(p->name, name) < 0 ? p->left : p->right)) if(!strcmp(p->name, name)){
		return p;
	}
	return NULL;
}

struct cmdalias **CmdAliasFindP(const char *name){
	struct cmdalias **pp;
	for(pp = &aliaslist; *pp; pp = (strcmp((*pp)->name, name) < 0 ? &(*pp)->left : &(*pp)->right)) if(!strcmp((*pp)->name, name)){
		return pp;
	}
	return pp;
}


