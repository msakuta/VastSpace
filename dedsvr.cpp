#include "Application.h"
#include "Game.h"
#include "cmd.h"
#include "sqadapt.h"
#include <stdio.h>
#include <stdlib.h>

DedicatedServerApplication application;

int main(int argc, char *argv[]){
	if(!application.parseArgs(argc, argv))
		return 1;

	application.serverGame = new ServerGame();

	application.init(false);

	application.hostgame(application.serverGame, application.port);

	char line[512];
	while(fgets(line, sizeof line, stdin))
		CmdExec(line);
}

