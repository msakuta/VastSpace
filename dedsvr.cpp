#include "Application.h"
#include "Game.h"
#include "cmd.h"
#include "sqadapt.h"
#include <stdio.h>
#include <stdlib.h>

DedicatedServerApplication application;

int main(int argc, char *argv[]){
	application.pg = new ServerGame();

	application.init(false);

	application.hostgame(application.pg);

	char line[512];
	while(fgets(line, sizeof line, stdin))
		CmdExec(line);
}

