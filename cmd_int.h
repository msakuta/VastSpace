#ifndef CMD_INT_H
#define CMD_INT_H

#define CB_LINES 256
#define CB_CHARS 128
#define MAX_ALIAS_NESTS 32
#define MAX_COMMAND_HISTORY 32
#define MAX_ARGC 32


extern char cmdbuffer[CB_LINES][CB_CHARS];
extern int cmdcurline;
extern int cmddispline;
extern int cmdcur;
extern char cmdline[CB_CHARS];
extern int console_cursorposdisp;


#endif
