#ifndef CMD_H
#define CMD_H


enum cvartype{cvar_int, cvar_float, cvar_double, cvar_string};

void CmdInit(void);
int CmdInput(char c);
int CmdSpecialInput(int c);
int CmdMouseInput(int button, int state, int x, int y);
int CmdExec(const char *cmdline);
void CmdPrint(const char *str);
void CmdPrintf(const char *str, ...);
void CmdAdd(const char *cmdname, int (*proc)(int argc, char *argv[]));
struct command *CmdFind(const char *name);
void CvarAdd(const char *cvarname, void *value, enum cvartype type);
void CvarAddVRC(const char *cvarname, void *value, enum cvartype type, int (*vrc)(void *value));
struct cvar *CvarFind(const char *cvarname);
const char *CvarGetString(const char *cvarname); /* The returned string's strage duration is assured until next call of this function */
void CmdAliasAdd(const char *name, const char *str);
struct cmdalias *CmdAliasFind(const char *name);
void CmdDraw(void);

#endif
