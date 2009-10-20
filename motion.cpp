#include "player.h"
#include "war.h"
#include "cmd.h"

/* Motion command handling, branched from the main module because it seldom changes but consumes
  a fair amount of lines. */

static input_t inputstate = {0};

static int cmd_pforward(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state |= PL_W;
	return 0;
}

static int cmd_nforward(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state &= ~PL_W;
	return 0;
}

static int cmd_pback(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state |= PL_S;
	return 0;
}

static int cmd_nback(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state &= ~PL_S;
	return 0;
}

static int cmd_pleft(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state |= PL_A;
	return 0;
}

static int cmd_nleft(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state &= ~PL_A;
	return 0;
}

static int cmd_pright(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state |= PL_D;
	return 0;
}

static int cmd_nright(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state &= ~PL_D;
	return 0;
}

static int cmd_pup(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state |= PL_Q;
	return 0;
}

static int cmd_nup(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state &= ~PL_Q;
	return 0;
}

static int cmd_pdown(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state |= PL_Z;
	return 0;
}

static int cmd_ndown(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state &= ~PL_Z;
	return 0;
}

static int cmd_pgear(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state |= PL_G;
/*	if(!pl.chase && !pl.control)
		indmenu = indmenu == indgear ? indnone : indgear;*/
	return 0;
}

static int cmd_ngear(int argc, char *argv[], void *pv){
	int *state = (int*)pv;
	*state &= ~PL_G;
/*	if(indmenu == indgear)
		indmenu = indnone;*/
	return 0;
}

void MotionInit(){
	CmdAddParam("+forward", cmd_pforward, &inputstate.press);
	CmdAddParam("-forward", cmd_nforward, &inputstate.press);
	CmdAddParam("+back", cmd_pback, &inputstate.press);
	CmdAddParam("-back", cmd_nback, &inputstate.press);
	CmdAddParam("+left", cmd_pleft, &inputstate.press);
	CmdAddParam("-left", cmd_nleft, &inputstate.press);
	CmdAddParam("+right", cmd_pright, &inputstate.press);
	CmdAddParam("-right", cmd_nright, &inputstate.press);
	CmdAddParam("+up", cmd_pup, &inputstate.press);
	CmdAddParam("-up", cmd_nup, &inputstate.press);
	CmdAddParam("+down", cmd_pdown, &inputstate.press);
	CmdAddParam("-down", cmd_ndown, &inputstate.press);
}

void MotionAnim(Player &pl, double dt){
		if(inputstate.press & PL_W)
			pl.velo -= pl.rot.itrans(vec3_001) * dt;
		if(inputstate.press & PL_S)
			pl.velo += pl.rot.itrans(vec3_001) * dt;
		if(inputstate.press & PL_A)
			pl.velo -= pl.rot.itrans(vec3_100) * dt;
		if(inputstate.press & PL_D)
			pl.velo += pl.rot.itrans(vec3_100) * dt;
		if(inputstate.press & PL_Z)
			pl.velo -= pl.rot.itrans(vec3_010) * dt;
		if(inputstate.press & PL_Q)
			pl.velo += pl.rot.itrans(vec3_010) * dt;

}
