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

#include <algorithm>
#include <unordered_map>

static struct viewport gvp;

gltestp::dstring cmdbuffer[CB_LINES];
int cmdcurline = 2;
int cmddispline = 0;

static gltestp::dstring cmdhist[MAX_COMMAND_HISTORY];
static int cmdcurhist = 0, cmdselhist = 0;

gltestp::dstring cmdline;

void (*CmdPrintHandler)(const char *line) = NULL;

static int cvar_echo = 0;
static int cvar_cmd_echo = 1;
static int console_pageskip = 8;
static int console_mousewheelskip = 4;
static int console_undefinedecho = 0;
static int cvar_cmdlog = 0;

static int CmdExecD(char *, bool server, ServerClient *);
static int CmdExecParams(int argc, char *argv[], bool server, ServerClient *sc);
static double cmd_sqcalc(const char *str, const SQChar *context = _SC("expression"));

template<>
class std::hash<gltestp::dstring> {
public:
	size_t operator()(const gltestp::dstring &str) const
	{
		return str.hash();
	}
};

/* binary tree */
struct Command{
	union commandproc{
		int (*a)(int argc, char *argv[]);
		int (*p)(int argc, char *argv[], void *);
		int (*s)(int argc, char *argv[], ServerClient *);
	} proc;
	enum class Type : int{ Arg, Ptr, SvCl };
	Type type;
	void *param;
	Command(int (*a)(int argc, char* argv[])) :
		type(Type::Arg), param(nullptr) {
		proc.a = a;
	}
	Command(int (*p)(int argc, char *argv[], void *), void* param) :
		type(Type::Ptr), param(param) {
		proc.p = p;
	}
	Command(int (*s)(int argc, char *argv[], ServerClient *)) :
		type(Type::SvCl), param(nullptr) {
		proc.s = s;
	}
};
static std::unordered_map<gltestp::dstring, Command> cmdlist;


#define CVAR_BUCKETS 53

/* hash table */
static struct CVar *cvarlist[CVAR_BUCKETS] = {NULL}, *cvarlinear = NULL;
static int cvarlists = 0;

/* binary tree */
struct CmdAlias{
	gltestp::dstring str;
};
static std::unordered_map<gltestp::dstring, CmdAlias> aliaslist;

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
/// Most of the time, the argument is rvalue, so move semantics are effective.
void CmdPrint(gltestp::dstring &&str){
	if(cvar_cmdlog)
		fprintf(logfp(), "%s\n", (const char*)str);

	// Windows default encoding for multibyte string is often not UTF-8, so we
	// convert it with wide char functions.
#ifdef _WIN32
	// This buffer is reused every time this function is called for speed,
	// meaning it will be left allcated when the program exits, which is
	// usually freed by OS instead.
	static std::vector<wchar_t> wbuf;
	size_t wcc = MultiByteToWideChar(CP_UTF8, 0, str, str.len(), NULL, 0);
	if(wbuf.size() < wcc + 1){
		wbuf.resize(wcc + 1);
	}
	MultiByteToWideChar(CP_UTF8, 0, str, str.len(), wbuf.data(), (int)wbuf.size());
	wbuf[wcc] = '\0';

	// And the multibyte string buffer.
	// It should not be necessary if wprintf worked, but it didn't.
	static std::vector<char> buf;
	size_t cc = WideCharToMultiByte(CP_THREAD_ACP, 0, wbuf.data(), (int)wbuf.size(), NULL, 0, NULL, NULL);
	if(buf.size() < cc + 1){
		buf.resize(cc + 1);
	}
	WideCharToMultiByte(CP_THREAD_ACP, 0, wbuf.data(), (int)wbuf.size(), buf.data(), (int)buf.size(), NULL, NULL);
	buf[cc] = '\0';

	if(CmdPrintHandler){
		CmdPrintHandler(buf.data());
	}

	printf("%s\n", buf.data());
	cmdbuffer[cmdcurline] = std::move(str);
#else
	const char *c_str = cmdbuffer[cmdcurline];
	std::cout << c_str;
#endif
	cmdcurline = (cmdcurline + 1) % CB_LINES;
}

/// Print a string to console.
void CmdPrint(const char *str){
	CmdPrint(gltestp::dstring(str));
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
	if(argc <= 1)
		return 0;
	gltestp::dstring out;
	char **argend = &argv[argc];
	for(argv++; argend != argv && *argv; argv++){
		out << *argv << ' ';
	}
	CmdPrint(std::move(out));
	return 0;
}


static int cmd_cmdlist(int argc, char *argv[]){
	auto pattern = 2 <= argc ? argv[1] : NULL;
	size_t count = 0;
	for(auto& it : cmdlist){
		if(!pattern || !strncmp(pattern, it.first, strlen(pattern))){
			CmdPrint(it.first);
			count++;
		}
	}
	CmdPrint(gltestp::dstring() << count << " commands listed");
	return 0;
}

static int cmd_cvarlist(int argc, char *argv[]){
	static const char typechar[] = {
		'i', 'f', 'd', 's'
	};
	int c;
	struct CVar *cv;
	c = 0;
	for(cv = cvarlinear; cv; cv = cv->linear) if(argc < 2 || !strncmp(argv[1], cv->name, strlen(argv[1]))){
#ifdef _DEBUG
		char buf[32];
		sprintf(buf, "%08X", hashfunc(cv->name));
		CmdPrint(gltestp::dstring() << typechar[cv->type] << ": " << cv->name << " (" << buf << ")");
#else
		CmdPrint(gltestp::dstring() << typechar[cv->type] << ": " << cv->name);
#endif
		c++;
	}
	CmdPrint(gltestp::dstring() << c << " cvars listed");
	return 0;
}

static int cmd_toggle(int argc, char *argv[]){
	struct CVar *cv;
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
	struct CVar *cv;
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
	struct CVar *cv;
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
	if(argc <= 2){
		CmdPrint("Specify an arithmetic cvar and a constant or 2 cvars to multiply.");
		return 0;
	}
	const char* arg = argv[1];
	CVar* cv = CvarFind(arg);
	if(cv){
		double val;
		CVar* cv2 = CvarFind(argv[2]);
		if(cv2){
			val = cv2->asDouble();
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
			default:
				CmdPrint("Variable of first argument is not a number.");
				return 1;
		}
		if(cv->vrc)
			cv->vrc(cv->v.i);
	}
	else
		CmdPrint("Specified variable is either not a number nor existing.");
	return 0;
}


static char *stringdup(const char *s){
	char *ret;
	ret = (char*)malloc(strlen(s) + 1);
	if(!ret)
		return nullptr;
	else
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
		struct CVar *cv;
		if(cv = CvarFind(thekey)) switch(cv->type){
			case cvar_int: *cv->v.i = atoi(thevalue); break;
			case cvar_float: *cv->v.f = (float)cmd_sqcalc(thevalue); break;
			case cvar_double: *cv->v.d = cmd_sqcalc(thevalue); break;
			case cvar_string:
				cv->v.s = (char*)realloc(cv->v.s, strlen(thevalue) + 1);
				if(cv->v.s)
					strcpy(cv->v.s, thevalue);
				break;
		}
		else{
			CvarAdd(stringdup(thekey), stringdup(thevalue), cvar_string);
		}
	}
/*	else
		cmd_echoa("Specify value to set.");*/
	return 0;
}

static gltestp::dstring cmd_preprocess(const char *in){
	gltestp::dstring ret;
	while(*in){
		if(*in == '$'){
			gltestp::dstring cvarName;
			in++;
			while(*in){
				if(!isalnum(*in) && *in != '_')
					break;
				cvarName += *in++;
			}
			CVar *cv = CvarFind(cvarName);
			if(cv) switch(cv->type){
				case cvar_int: ret << *cv->v.i; break;
				case cvar_float: ret << *cv->v.f; break;
				case cvar_double: ret << *cv->v.d; break;
				case cvar_string: ret << cv->v.s; break;
			}
			else {
				CmdPrint(gltestp::dstring("Cvar substitution failed: ") << cvarName);
			}
		}
		else
			ret += *in++;
	}
	return ret;
}

static int cmd_exec(int argc, char *argv[]){
	if(argc <= 1){
		CmdPrint("Specify a file as argument to load the file to batch execute commands.");
		return 0;
	}
	FILE *fp = fopen(argv[1], "r");
	if(!fp){
		CmdPrint(gltestp::dstring() << "Couldn't load " << argv[1] << "!");
		return 0;
	}
	CmdPrint(gltestp::dstring() << "executing " << argv[1]);
	gltestp::dstring buf;
	int c;
	while(EOF != (c = fgetc(fp))) if(c == '\\'){
		char lc = c;
		if(EOF == (c = fgetc(fp)))
			break;
		if(c != '\n') // Only escape newlines; leave other escape sequnces to the preprocessor.
			buf << char(lc) << char(c);
	}
	else if(c == '\n'){
		CmdExec(cmd_preprocess(buf));
		buf = "";
	}
	else buf << char(c);

	// Process the last line without a newline.
	CmdExec(cmd_preprocess(buf));

	fclose(fp);
	return 0;
}

static int cmd_time(int argc, char *argv[]){
	timemeas_t tm;
	if(argc <= 1){
		CmdPrint("Measures time elapsed while executing commands.");
		return 0;
	}
	TimeMeasStart(&tm);
	CmdExecParams(argc - 1, &argv[1], false, nullptr);
	CmdPrint(gltestp::dstring() << TimeMeasLap(&tm) << " seconds");
	return 0;
}

static int cmd_alias(int argc, char *argv[]){
	if(argc <= 1){
		int count = 0;
		for(const auto& it : aliaslist){
			CmdPrint(gltestp::dstring() << it.first << ": " << it.second.str);
			count++;
		}
		CmdPrint(gltestp::dstring() << count << " aliases listed");
		return 0;
	}
	else if(argc == 2){
/*	else if(strncpy(args, arg, sizeof args), !(thekey = strtok(args, " \t")))
		return 0;
	else if(!(thevalue = strtok(NULL, ""))){*/
		CmdAlias *a;
		char *thekey = argv[1];
		if(a = CmdAliasFind(thekey)){
			CmdPrint(gltestp::dstring() << thekey << ": " << a->str);
			return 0;
		}
		CmdPrint(gltestp::dstring() << "no alias named " << thekey << " is found.");
		return -1;
	}
	size_t valuelen = 0;
	for(int i = 2; i < argc; i++)
		valuelen += strlen(argv[i]) + 1;
	gltestp::dstring thevalue;
	for(int i = 2; i < argc; i++){
		thevalue += argv[i];
		if(i != argc - 1)
			thevalue += ' ';
	}
	thevalue.push_back('\0');
	CmdAliasAdd(argv[1], thevalue.c_str());
	return 1;
}

static int cmd_unalias(const char *arg){
	if(!arg){
		CmdPrint("Specify alias name to undefine.");
		return 1;
	}
	auto it = aliaslist.find(arg);
	if((it != aliaslist.end())){
		aliaslist.erase(it);
	}
	else{
		CmdPrint("Specified alias name couldn't be found.");
		return 1;
	}
	return 0;
}

size_t cmd_memory_alias(const std::unordered_map<gltestp::dstring, CmdAlias> &p){
	size_t ret = 0;
	for(const auto& it : p){
		ret += sizeof it + it.first.len() + strlen(it.second.str);
	}
	return ret;
}

/* print consumed memory for console management */
static int cmd_memory(int argc, char *argv[]){
	size_t size = 0;
	for(int i = 0; i < numof(cmdbuffer); i++)
		size += cmdbuffer[i].len();
	size += sizeof cmdbuffer + sizeof cmdhist;
	CmdPrint(gltestp::dstring() << "cmdbuf: " << size << " bytes = " << (size + 1023) / 1024 << " kilobytes used");
	size = cmd_memory_alias(aliaslist);
	CmdPrint(gltestp::dstring() << "alias: " << size << " bytes = " << (size + 1024) / 1024 << " kilobytes used");
	CmdPrint(gltestp::dstring() << "cvar: " << cvarlists * sizeof **cvarlist << " bytes = " << cvarlists * sizeof **cvarlist / 1024 << " kilobytes used");
#ifndef DEDICATED
	size = CircleCutsMemory();
	CmdPrint(gltestp::dstring() << "circut: " << size << " bytes = " << (size + 1023) / 1024 << " kilobytes used");
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
		cmdline = gltestp::dstring().strncat(cmdline, cmdline.len() - 1);
		return 0;
	}
	else if(key == '\n'){
		return 0;
	}
	else if(key == '\n' && last == '\r' || key == '\r'){
		if(cmdline.len()){
			int ret, echo;
			cmdhist[cmdcurhist] = cmdline;
			cmdselhist = cmdcurhist = (cmdcurhist + 1) % MAX_COMMAND_HISTORY;
			echo = cvar_cmd_echo && cmdline[0] != '@';
			if(echo){
				cmdbuffer[cmdcurline] = cmdline;
				cmdcurline = (cmdcurline + 1) % CB_LINES;
			}
			ret = CmdExec(cmd_preprocess(cmdline), false, NULL);
			cmdline = "";
#ifdef _WIN32
			if(console_cursorposdisp)
				cmddispline = 0;
#endif
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
		len = (int)strlen(s);
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
	char *post;
	int ret = 0;
	// This loop runs more than once when the cmdstring contains a semicolon.
	do{
		char *argv[MAX_ARGC];
		int argc;
		argc = argtok(argv, cmdstring, &post, MAX_ARGC);
		ret = CmdExecParams(argc, argv, server, sc);
	}while(cmdstring = post);
	return ret;
}

static int CmdExecParams(int argc, char *argv[], bool server, ServerClient *sc){
	char *cmd = argv[0];

	if(!cmd || !*cmd || *cmd == '#')
		return 0;

	if(*cmd == '@'){
		if(cmd[1])
			cmd = ++argv[0];
		else
			memmove(argv, &argv[1], --argc * sizeof *argv);
	}

	int ret = 0;
	struct CmdAlias *pa;
	/* aliases are searched before commands, allowing overrides of commands. */
	if(pa = CmdAliasFind(cmd)){
		int returned;
		if(aliasnest++ < MAX_ALIAS_NESTS){
			if(argv[1]){
				returned = CmdExec(gltestp::dstring(pa->str) << " " << argv[1]);
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
		return returned;
	}
	Command *pc = CmdFind(cmd);
	if(pc){
		switch(pc->type){
		case Command::Type::Arg:
			ret = pc->proc.a(argc, argv);
			break;
		case Command::Type::Ptr:
			ret = pc->proc.p(argc, argv, pc->param);
			break;
		case Command::Type::SvCl:
			if(server)
				ret = pc->proc.s(argc, argv, sc);
			break;
		}
		return ret;
	}

	/* Try Squirrel command layer. */
	int retval;
	if(sqa_console_command(argc, argv, &retval)){
		return retval;
	}

	{
		struct CVar *cv;
		for(cv = cvarlist[hashfunc(cmd) % numof(cvarlist)]; cv; cv = cv->next) if(!strcmp(cv->name, cmd)){
			gltestp::dstring buf;
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
				case cvar_string:
					cv->v.s = (char*)realloc(cv->v.s, strlen(arg) + 1);
					if(cv->v.s)
						strcpy(cv->v.s, arg);
					break;
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
			return 1;
		}
	}
	if(console_undefinedecho){
		CmdPrint(gltestp::dstring() << "Undefined command: " << cmd);
	}
	return ret;
}

int CmdExec(const char *cmdstring, bool server, ServerClient* sc){
	char *buf = new char[strlen(cmdstring) + 2];
	strcpy(buf, cmdstring);
	int ret = CmdExecD(buf, server, sc);
	delete[] buf;
	return ret;
}

int CmdExpandExec(const char* cmdstring) {
	return CmdExec(cmd_preprocess(cmdstring));
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
	cmdlist.emplace(cmdname, Command{proc});
}

void CmdAddParam(const char *cmdname, int (*proc)(int, char*[], void *), void *param){
	cmdlist.emplace(cmdname, Command{proc, param});
}

/// \brief Registers a cmdname as a server command and binds it with a callback function.
///
/// Might not necessary to share list of command strings with the client commands, because remote users
/// should not execute arbitrary client commands in the server.
void ServerCmdAdd(const char *cmdname, int (*proc)(int, char *[], ServerClient*)){
	cmdlist.emplace(cmdname, Command{proc});
}

#define profile 0

Command *CmdFind(const char *name){
	struct Command *p;
#if profile
	timemeas_t tm;
	TimeMeasStart(&tm);
#endif
	auto it = cmdlist.find(name);
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
	if(it != cmdlist.end())
		return &it->second;
	else
		return nullptr;
	}

double CVar::asDouble() {
	switch(type){
		case cvar_int: return *v.i; break;
		case cvar_float: return *v.f; break;
		case cvar_double: return *v.d; break;
		case cvar_string: {
			char* endptr;
			double ret = strtod(v.s, &endptr);
			if(endptr == &v.s[strlen(v.s)])
				return ret;
			else{
				CmdPrint("Conversion of cvar string to double failed");
				return 0.;
			}
		}break;
		default:
			assert(0);
	}
	return 1;
}

void CvarAdd(const char *cvarname, void *value, enum CVarType type){
	CVar *cv = new CVar();
	if(!cv)
		return;
	CVar **ppcv = &cvarlist[hashfunc(cvarname) % numof(cvarlist)];
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

void CvarAddVRC(const char *cvarname, void *value, enum CVarType type, int (*vrc)(void*)){
	CVar *cv = new CVar();
	CVar **ppcv = &cvarlist[hashfunc(cvarname) % numof(cvarlist)];
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

struct CVar *CvarFind(const char *cvarname){
	struct CVar *cv;
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
	struct CVar *cv;
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
	aliaslist[name] = {str};
}

struct CmdAlias *CmdAliasFind(const char *name){
	auto it = aliaslist.find(name);
	if(it != aliaslist.end()){
		return &it->second;
	}
	return nullptr;
}


