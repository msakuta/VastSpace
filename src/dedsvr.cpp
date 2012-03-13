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
	feenableexcept(FE_INVALID | FE_DIVBYZERO);
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

