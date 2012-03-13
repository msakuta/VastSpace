#ifndef CMD_INT_H
#define CMD_INT_H
#include <cpplib/dstring.h>
#ifdef __cplusplus
extern "C"{
#endif

#define CB_LINES 256
#define MAX_ALIAS_NESTS 32
#define MAX_COMMAND_HISTORY 32
#define MAX_ARGC 64

struct cvar{
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
};

extern cpplib::dstring cmdbuffer[CB_LINES];
extern int cmdcurline;
extern int cmddispline;
//extern int cmdcur;
extern cpplib::dstring cmdline;
#ifdef _WIN32
extern int console_cursorposdisp;
#endif


#ifdef __cplusplus
}
#endif
#endif
