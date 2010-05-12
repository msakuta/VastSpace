#define _CRT_SECURE_NO_WARNINGS
#include "cmd.h"
#include "argtok.h"
#include "calc/calc.h"
#include "cmd_int.h"
#include "viewer.h"
#include <clib/c.h>
/*#include <clib/gl/gldraw.h>*/
#include <clib/timemeas.h>
/*#include <GL/glut.h>*/
#include "antiglut.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>

static struct viewport gvp;

char cmdbuffer[CB_LINES][CB_CHARS] = {0}/*{"gltest running.", "build: " __DATE__ ", " __TIME__}*/;
int cmdcurline = 2;
int cmddispline = 0;

static char cmdhist[MAX_COMMAND_HISTORY][CB_CHARS] = {""};
static int cmdcurhist = 0, cmdselhist = 0;

int cmdcur = 0;
char cmdline[CB_CHARS];

static int cvar_echo = 0;
static int cvar_cmd_echo = 1;
static int console_pageskip = 8;
static int console_mousewheelskip = 4;
static int console_undefinedecho = 0;

int CmdExecD(char *);
struct cmdalias **CmdAliasFindP(const char *name);

/* binary tree */
static struct command{
	union commandproc{
		int (*a)(int argc, char *argv[]);
		int (*p)(int argc, char *argv[], void *);
	} proc;
	int type;
	void *param;
	const char *name;
	struct command *left, *right;
} *cmdlist = NULL;
static int cmdlists = 0;


#define CVAR_BUCKETS 53

/* hash table */
static struct cvar{
	enum cvartype type;
	const char *name;
	struct cvar *next, *linear;
	union{
		int *i;
		float *f;
		double *d;
		char *s;
	} v;
	int (*vrc)(void *); /* Value Range Check */
} *cvarlist[CVAR_BUCKETS] = {NULL}, *cvarlinear = NULL;
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

void CmdPrint(const char *str){
	strncpy(cmdbuffer[cmdcurline], str, CB_CHARS);
	puts(cmdbuffer[cmdcurline]);
/*	putchar('\n');*/
	cmdcurline = (cmdcurline + 1) % CB_LINES;
}

void CmdPrintf(const char *str, ...){
	char buf[512];
	va_list args;
	va_start(args, str);
	vsprintf(buf, str, args);
	va_end(args);
	CmdPrint(buf);
}


static int cmd_echo(int argc, const char *argv[]){
	char *out = cmdbuffer[cmdcurline];
	if(argc <= 1)
		return 0;
	for(argv++; *argv; argv++){
		strncpy(out, *argv, CB_CHARS - (out - cmdbuffer[cmdcurline]));
		out += strlen(*argv);
		*out++ = ' ';
	}
	puts(cmdbuffer[cmdcurline]);
/*	putchar('\n');*/
	cmdcurline = (cmdcurline + 1) % CB_LINES;
	return 0;
}

static int cmd_echoa(char *arg){
	char *argv[3];
	argv[0] = "echo";
	argv[1] = arg;
	argv[2] = NULL;
	return cmd_echo(2, argv);
}


static int cmd_cmdlist_int(const struct command *c, const char *pattern, int level){
	int ret = 0;
	if(c->right)
		ret += cmd_cmdlist_int(c->right, pattern, level + 1);
	if(!pattern || !strncmp(pattern, c->name, strlen(pattern))){
#ifdef _DEBUG
		char buf[CB_CHARS];
		sprintf(buf, "%d: %s", level, c->name);
		cmd_echoa(buf);
#else
		cmd_echoa(c->name);
#endif
		ret++;
	}
	if(c->left)
		ret += cmd_cmdlist_int(c->left, pattern, level + 1);
	return ret;
}

static int cmd_cmdlist(int argc, const char *argv[]){
	int c;
	char buf[CB_CHARS];
	c = cmd_cmdlist_int(cmdlist, 2 <= argc ? argv[1] : NULL, 0);
	sprintf(buf, "%d commands listed", c);
	cmd_echoa(buf);
	return 0;
}

static int cmd_cvarlist(int argc, const char *argv[]){
	static const char typechar[] = {
		'i', 'f', 'd', 's'
	};
	int c;
	struct cvar *cv;
	char buf[CB_CHARS];
	c = 0;
	for(cv = cvarlinear; cv; cv = cv->linear) if(argc < 2 || !strncmp(argv[1], cv->name, strlen(argv[1]))){
#ifdef _DEBUG
		sprintf(buf, "%c: %s (%08X)", typechar[cv->type], cv->name, hashfunc(cv->name));
#else
		sprintf(buf, "%c: %s", typechar[cv->type], cv->name);
#endif
		cmd_echoa(buf);
		c++;
	}
#ifdef _NDEBUG
	for(i = 0; i < numof(cvarlist); i++){
		int n = 0;
		for(cv = cvarlist[i]; cv; cv = cv->next) n++;
		sprintf(buf, "cvarlist[%d]: %d", i, n);
		cmd_echoa(buf);
	}
#endif
	sprintf(buf, "%d cvars listed", c);
	cmd_echoa(buf);
	return 0;
}

static int cmd_toggle(int argc, const char *argv[]){
	struct cvar *cv;
	const char *arg = argv[1];
	if(!arg){
		cmd_echoa("Specify a integer cvar to toggle it, assuming flag-like usage.");
		return 0;
	}
	if((cv = CvarFind(arg)) && cv->type == cvar_int){
		*cv->v.i = !*cv->v.i;
		if(cv->vrc)
			cv->vrc(cv->v.i);
	}
	else
		cmd_echoa("Specified variable is either not a integer nor existing.");
	return 0;
}

static int cmd_inc(int argc, const char *argv[]){
	struct cvar *cv;
	const char *arg = argv[1];
	if(!arg){
		cmd_echoa("Specify a integer cvar to increment by 1.");
		return 0;
	}
	if((cv = CvarFind(arg)) && cv->type == cvar_int){
		++*cv->v.i;
		if(cv->vrc)
			cv->vrc(cv->v.i);
	}
	else
		cmd_echoa("Specified variable is either not a integer nor existing.");
	return 0;
}

static int cmd_dec(int argc, const char *argv[]){
	struct cvar *cv;
	const char *arg = argv[1];
	if(!arg){
		cmd_echoa("Specify a integer cvar to decrement by 1.");
		return 0;
	}
	if((cv = CvarFind(arg)) && cv->type == cvar_int){
		--*cv->v.i;
		if(cv->vrc)
			cv->vrc(cv->v.i);
	}
	else
		cmd_echoa("Specified variable is either not a integer nor existing.");
	return 0;
}

static int cmd_mul(int argc, const char *argv[]){
	struct cvar *cv, *cv2;
	const char *arg = argv[1];
	if(argc <= 2){
		cmd_echoa("Specify an arithmetic cvar and a constant or 2 cvars to multiply.");
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
		cmd_echoa("Specified variable is either not a integer nor existing.");
	return 0;
}


static char *stringdup(const char *s){
	char *ret;
	ret = malloc(strlen(s) + 1);
	strcpy(ret, s);
	return ret;
}

/* explicit cvar setter, useful in some occasion */
int cmd_set(int argc, const char *argv[]){
	const char *thekey, *thevalue;
	if(argc <= 1){
		cmd_echoa("Specify a cvar to set it.");
		return 0;
	}
	thekey = argv[1];
	if(argc <= 2){
		cmd_echoa("Specify a value to set.");
		return 0;
	}
	thevalue = argv[2];
/*	else if(strncpy(args, arg, sizeof args), !(thekey = strtok(args, " \t")))
		return 0;
	else if(thevalue = strtok(NULL, ""))*/{
		struct cvar *cv;
		if(cv = CvarFind(thekey)) switch(cv->type){
			case cvar_int: *cv->v.i = atoi(thevalue); break;
			case cvar_float: *cv->v.f = (float)calc3(&thevalue, calc_mathvars(), NULL); break;
			case cvar_double: *cv->v.d = calc3(&thevalue, calc_mathvars(), NULL); break;
			case cvar_string: cv->v.s = realloc(cv->v.s, strlen(thevalue) + 1); strcpy(cv->v.s, thevalue); break;
		}
		else{
/*			cmd_echoa("Specified variable is not existing.");*/
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

static int cmd_exec(int argc, const char *argv[]){
	char buf[CB_CHARS];
	FILE *fp;
	if(argc <= 1){
		cmd_echoa("Specify a file as argument to load the file to batch execute commands.");
		return 0;
	}
	fp = fopen(argv[1], "r");
	if(!fp){
		sprintf(buf, "Couldn't load %s!", argv[1]);
		cmd_echoa(buf);
		return 0;
	}
	sprintf(buf, "execing %s", argv[1]);
	cmd_echoa(buf);
	while(fgets(buf, sizeof buf, fp)){
		char *p;
		char linebuf[CB_CHARS];
		while(p = strstr(buf, "\\\n")){
			if(!fgets(p, &buf[sizeof buf] - p, fp))
				goto gbreak;
		}
		if(p = strchr(buf, '\n'))
			*p = '\0';
		cmd_preprocess(linebuf, buf);
		CmdExec(linebuf);
	}
gbreak:
	fclose(fp);
	return 0;
}

static int cmd_time(int argc, const char *argv[]){
	char *thevalue;
	char buf[CB_CHARS];
	int i;
	size_t valuelen;
	timemeas_t tm;
	if(argc <= 1){
		cmd_echoa("Measures time elapsed while executing commands.");
		return 0;
	}
	for(valuelen = 0, i = 1; i < argc; i++)
		valuelen += strlen(argv[i]) + 1;
	thevalue = malloc(valuelen);
	for(valuelen = 0, i = 1; i < argc; i++){
		strcpy(&thevalue[valuelen], argv[i]);
		valuelen += strlen(argv[i]) + 1;
		if(i != argc - 1)
			thevalue[valuelen-1] = ' ';
	}
	TimeMeasStart(&tm);
	CmdExecD(thevalue);
	sprintf(buf, "%lg seconds", TimeMeasLap(&tm));
	cmd_echoa(buf);
	free(thevalue);
	return 0;
}

static int listalias(struct cmdalias *a, int level){
	int ret = 0;
	if(!a)
		return 0;
	ret += listalias(a->right, level + 1);
	{
		char buf[CB_CHARS];
#ifdef _DEBUG
		sprintf(buf, "(%d) %s: %s", level, a->name, a->str);
#else
		sprintf(buf, "%s: %s", a->name, a->str);
#endif
		cmd_echoa(buf);
	}
	ret += listalias(a->left, level + 1);
	return ret + 1;
}

static int cmd_alias(int argc, char *argv[]){
	char *thekey, *thevalue;
	char buf[CB_CHARS];
	int i;
	size_t valuelen;
	if(argc <= 1){
		int ret;
		ret = listalias(aliaslist, 0);
		sprintf(buf, "%d aliases listed", ret);
		cmd_echoa(buf);
		return 0;
	}
	else if(argc == 2){
/*	else if(strncpy(args, arg, sizeof args), !(thekey = strtok(args, " \t")))
		return 0;
	else if(!(thevalue = strtok(NULL, ""))){*/
		struct cmdalias *a;
		thekey = argv[1];
		if(a = CmdAliasFind(thekey)){
			sprintf(buf, "@echo %c: %s", a->name, a->str);
			CmdExec(buf);
			return 0;
		}
		sprintf(buf, "@echo no aliase named %s is found.", thekey);
		CmdExec(buf);
		return -1;
	}
	thekey = argv[1];
	for(valuelen = 0, i = 2; i < argc; i++)
		valuelen += strlen(argv[i]) + 1;
	thevalue = malloc(valuelen);
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

extern size_t CircleCutsMemory(void);

/* print consumed memory for console management */
static int cmd_memory(int argc, char *argv[]){
	char buf[CB_CHARS];
	size_t size;
	sprintf(buf, "cmdbuf: %lu bytes = %lu kilobytes used", sizeof cmdbuffer + sizeof cmdhist, (sizeof cmdbuffer + sizeof cmdhist) / 1024);
	cmd_echoa(buf);
	size = cmd_memory_alias(aliaslist);
	sprintf(buf, "alias: %lu bytes = %lu kilobytes used", size, (size) / 1024);
	cmd_echoa(buf);
	sprintf(buf, "cvar: %lu bytes = %lu kilobytes used", cvarlists * sizeof **cvarlist, cvarlists * sizeof **cvarlist / 1024);
	cmd_echoa(buf);
	size = CircleCutsMemory();
	sprintf(buf, "circut: %lu bytes = %lu kilobytes used", size, size / 1024);
	cmd_echoa(buf);
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
	cmd_echoa("gltestplus running.");
	cmd_echoa("build: " __DATE__ ", " __TIME__);
}

#define DELETEKEY 0x7f

int CmdInput(char key){
	static char last = '\0';
	if(key == DELETEKEY)
		return 1;
	if(key == 0x08){
		if(cmdcur){
			cmdcur--;
		}
		return 0;
	}
	else if(key == '\n'){
		return 0;
	}
	else if(key == '\n' && last == '\r' || key == '\r'){
/*		if(cmdcur && last == '\\'){
			cmdcur--;
		}
		else*/{
			cmdline[cmdcur] = '\0';
			if(cmdcur){
				int ret, echo;
				char linebuf[CB_CHARS];
				cmdcur = 0;
				strncpy(cmdhist[cmdcurhist], cmdline, CB_CHARS);
				cmdselhist = cmdcurhist = (cmdcurhist + 1) % MAX_COMMAND_HISTORY;
				echo = cvar_cmd_echo && cmdline[0] != '@';
				if(echo){
					strncpy(cmdbuffer[cmdcurline], cmdline, CB_CHARS);
					cmdcurline = (cmdcurline + 1) % CB_LINES;
				}
				cmd_preprocess(linebuf, cmdline);
				ret = CmdExecD(linebuf);
				cmdline[0] = '\0';
				if(console_cursorposdisp)
					cmddispline = 0;
			}
		}
	}
	else if(cmdcur < CB_CHARS)
		cmdline[cmdcur++] = key;
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
		strncpy(cmdline, cmdhist[cmdselhist = (cmdselhist + (key == GLUT_KEY_UP ? -1 : 1) + MAX_COMMAND_HISTORY) % MAX_COMMAND_HISTORY], sizeof cmdline);
		cmdcur = strlen(cmdline);
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
		cmd_echoa("Double quotes(\") doesn't match.");
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
			if(maxargs <= ret)
				return ret;
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
		cmd_echoa("Double quotes(\") doesn't match.");
	argv[ret] = NULL;
	return ret;
}

static int aliasnest = 0;

/* destructive, i.e. cmdstring is modified by strtok or similar
  method to tokenize. */
static int CmdExecD(char *cmdstring){
	int ret = 0;
	struct command *pc;
	struct cmdalias *pa;
	char *cmd/*, *arg, *arg2*/, *post;
	do{
	char *argv[MAX_ARGC];
	int argc;
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
			char buf[CB_CHARS+1];
			if(argv[1]){
				sprintf(buf, "%s %s", pa->str, argv[1]);
				returned = CmdExecD(buf);
			}
			else
				returned = CmdExec(pa->str);
		}
		else{
			cmd_echoa("Aliase stack overflow!");
			cmd_echoa("Infinite loop is likely the case.");
			returned = -1;
		}
		aliasnest--;
		ret = returned;
		continue;
	}
	if(pc = CmdFind(cmd)){
		ret = (pc->type == 0 ? pc->proc.a(argc, argv) : pc->proc.p(argc, argv, pc->param));
		continue;
	}
	{
		struct cvar *cv;
		for(cv = cvarlist[hashfunc(cmd) % numof(cvarlist)]; cv; cv = cv->next) if(!strcmp(cv->name, cmd)){
			char buf[CB_CHARS];
			char *arg = argv[1];
			if(!arg) switch(cv->type){
				case cvar_int: sprintf(buf, "\"%s\" is %d", cmd, *cv->v.i); break;
				case cvar_float: sprintf(buf, "\"%s\" is %f", cmd, *cv->v.f); break;
				case cvar_double: sprintf(buf, "\"%s\" is %lf", cmd, *cv->v.d); break;
				case cvar_string: sprintf(buf, "\"%s\" is %s", cmd, cv->v.s); break;
			}
			else switch(cv->type){
				case cvar_int: *cv->v.i = atoi(arg); break;
				case cvar_float: *cv->v.f = (float)calc3(&arg, calc_mathvars(), NULL); break;
				case cvar_double: *cv->v.d = calc3(&arg, calc_mathvars(), NULL); break;
				case cvar_string: cv->v.s = realloc(cv->v.s, strlen(arg) + 1); strcpy(cv->v.s, arg); break;
			}
			if(cv->vrc)
				cv->vrc(cv->v.i);
			if(arg) switch(cv->type){
				case cvar_int: sprintf(buf, "\"%s\" set to %d", cmd, *cv->v.i); break;
				case cvar_float: sprintf(buf, "\"%s\" set to %f", cmd, *cv->v.f); break;
				case cvar_double: sprintf(buf, "\"%s\" set to %lf", cmd, *cv->v.d); break;
				case cvar_string: sprintf(buf, "\"%s\" set to %s", cmd, cv->v.s); break;
			}
			if(cvar_echo)
				cmd_echoa(buf);
			ret = 1;
			goto gcon;
		}
	}
	if(console_undefinedecho){
		CmdPrintf("Undefined command: %s", cmd);
	}
gcon:;
	}while(cmdstring = post);
	return ret;
}

int CmdExec(const char *cmdstring){
	char buf[CB_CHARS+1];
	strncpy(buf, cmdstring, sizeof buf);
	return CmdExecD(buf);
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
	cv->v.i = value;
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
	cv->v.i = value;
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
		case cvar_int: buf = realloc(buf, 64); sprintf(buf, "%d", *cv->v.i); break;
		case cvar_float: buf = realloc(buf, 64); sprintf(buf, "%f", *cv->v.f); break;
		case cvar_double: buf = realloc(buf, 64); sprintf(buf, "%lf", *cv->v.d); break;
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


