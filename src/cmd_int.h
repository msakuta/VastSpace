#ifndef CMD_INT_H
#define CMD_INT_H
#include "dstring.h"
#ifdef __cplusplus
extern "C"{
#endif

#define CB_LINES 256
#define MAX_ALIAS_NESTS 32
#define MAX_COMMAND_HISTORY 32
#define MAX_ARGC 64

struct CVar{
	enum CVarType type;
	gltestp::dstring name;
	struct CVar *next, *linear;
	union{
		int *i;
		float *f;
		double *d;
		char *s;
	} v;
	int (*vrc)(void *); /* Value Range Check */
	double asDouble();
};

extern gltestp::dstring cmdbuffer[CB_LINES];
extern int cmdcurline;
extern int cmddispline;
//extern int cmdcur;
extern gltestp::dstring cmdline;
#ifdef _WIN32
extern int console_cursorposdisp;
#endif

extern void (*CmdPrintHandler)(const char *line);

#ifdef __cplusplus
}
#endif
#endif
