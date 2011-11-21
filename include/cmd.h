#ifndef CMD_H
#define CMD_H
#include "export.h"
#ifdef __cplusplus
#include <cpplib/dstring.h>
EXPORT void CmdPrint(const cpplib::dstring &);
extern "C"{
#endif

enum cvartype{cvar_int, cvar_float, cvar_double, cvar_string};

struct command;
struct ServerClient;

void CmdInit(struct viewport *pvp);
EXPORT int CmdInput(char c);
EXPORT int CmdSpecialInput(int c);
EXPORT int CmdMouseInput(int button, int state, int x, int y);
EXPORT int CmdExec(const char *cmdline);
EXPORT int ServerCmdExec(const char *cmdline, struct ServerClient *sc);
EXPORT void CmdPrint(const char *str);
EXPORT void CmdPrintf(const char *str, ...);
EXPORT void CmdAdd(const char *cmdname, int (*proc)(int argc, char *argv[]));
EXPORT void CmdAddParam(const char *cmdname, int (*proc)(int argc, char *argv[], void *), void *);
EXPORT void ServerCmdAdd(const char *cmdname, int (*proc)(int argc, char *argv[], struct ServerClient*));
EXPORT struct command *CmdFind(const char *name);
EXPORT void CvarAdd(const char *cvarname, void *value, enum cvartype type);
EXPORT void CvarAddVRC(const char *cvarname, void *value, enum cvartype type, int (*vrc)(void *value));
EXPORT struct cvar *CvarFind(const char *cvarname);
EXPORT const char *CvarGetString(const char *cvarname); /* The returned string's strage duration is assured until next call of this function */
EXPORT void CmdAliasAdd(const char *name, const char *str);
EXPORT struct cmdalias *CmdAliasFind(const char *name);

EXPORT int cmd_set(int argc, char *argv[]);

#ifdef __cplusplus
struct viewport;
void CmdDraw(viewport &);
}
#endif
#endif
