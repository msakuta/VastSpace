#include "Application.h"
#include "Game.h"
#include "cmd.h"
#include "sqadapt.h"
#include <stdio.h>
#include <stdlib.h>

DedicatedServerApplication application;

static int cmd_exit(int argc, char *argv[]){
#ifdef _WIN32
	// Safely close the window
	PostMessage(hWndApp, WM_SYSCOMMAND, SC_CLOSE, 0);
#else
	exit(0);
#endif
	return 0;
}

static int cmd_sq(int argc, char *argv[]){
	if(argc < 2){
		CmdPrint("usage: sq \"Squirrel One Linear Source\"");
		return 0;
	}
	HSQUIRRELVM &v = g_sqvm;
	if(SQ_FAILED(sq_compilebuffer(g_sqvm, argv[1], strlen(argv[1]), _SC("sq"), SQTrue))){
		CmdPrint("Compile error");
	}
	else{
		sq_pushroottable(g_sqvm);
		sq_call(g_sqvm, 1, SQFalse, SQTrue);
		sq_pop(v, 1);
	}
	return 0;
}

int main(int argc, char *argv[]){
	printf("Hello Universe %p\n", &application);
	application.pg = new ServerGame();

	application.init(false);

	char line[512];
	while(fgets(line, sizeof line, stdin))
		CmdExec(line);
}

