#include "Game.h"
#include <stdio.h>

Game *g_pGame = new Game();

int main(int argc, char *argv[]){
	printf("Hello Universe %p\n", g_pGame);
}
