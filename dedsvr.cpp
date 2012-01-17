#include "Application.h"
#include "Game.h"
#include <stdio.h>

DedicatedServerApplication application;

int main(int argc, char *argv[]){
	printf("Hello Universe %p\n", &application);
}
