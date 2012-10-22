#include "Application.h"
#include "Game.h"
#include "cmd.h"
#include "sqadapt.h"
#include <stdio.h>
#include <stdlib.h>
#include <fenv.h>
#include <signal.h>

DedicatedServerApplication application;

static void sigfpe(int signal){
	printf("SIGFPE!\n");
	abort();
}

int main(int argc, char *argv[]){
#if defined _WIN32
	// MinGW cannot compile _controlfp for unclear reason.
#	if !defined __MINGW32__
	// Unmask invalid floating points to detect bugs
	unsigned flags = _controlfp(0,_EM_INVALID|_EM_ZERODIVIDE);
	printf("controlfp %p\n", flags);
#	endif
#else
	feenableexcept(FE_INVALID | FE_DIVBYZERO);
#endif
	signal(SIGFPE, sigfpe);
	if(!application.parseArgs(argc, argv))
		return 1;

	application.serverGame = new ServerGame();

	application.init(false);

	application.hostgame(application.serverGame, application.serverParams.port);

	char line[512];
	while(fgets(line, sizeof line, stdin))
		CmdExec(line);
}

