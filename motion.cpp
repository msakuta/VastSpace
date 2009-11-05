#include "motion.h"
#include "player.h"
#include "war.h"
#include "cmd.h"

/* Motion command handling, branched from the main module because it seldom changes but consumes
  a fair amount of lines. */

static int inputstate = 0, previnputstate = 0, toggleinputstate = 0;

#define fgather(key,f) static int cmd_p##key(int argc, char *argv[], void *pv){\
	int *state = (int*)pv;\
	*state |= f;\
	return 0;\
}\
static int cmd_n##key(int argc, char *argv[], void *pv){\
	int *state = (int*)pv;\
	*state &= ~f;\
	return 0;\
}

/* Command callback functions. Using macro to define functions like this tend to mess up VC IntelliSense
  especially in older VC, but it's miserably confused anyway as made me give up. */
fgather(forward, PL_W);
fgather(back, PL_S);
fgather(left, PL_A);
fgather(right, PL_D);
fgather(up, PL_Q);
fgather(down, PL_Z);
fgather(stop, PL_E);
fgather(tether, PL_B);
fgather(focus, PL_F);
fgather(eject, PL_SPACE);
fgather(sprint, PL_SHIFT);
fgather(crouch, PL_CTRL);
fgather(gear, PL_G);


void MotionInit(){
	CmdAddParam("+forward", cmd_pforward, &inputstate);
	CmdAddParam("-forward", cmd_nforward, &inputstate);
	CmdAddParam("+back", cmd_pback, &inputstate);
	CmdAddParam("-back", cmd_nback, &inputstate);
	CmdAddParam("+left", cmd_pleft, &inputstate);
	CmdAddParam("-left", cmd_nleft, &inputstate);
	CmdAddParam("+right", cmd_pright, &inputstate);
	CmdAddParam("-right", cmd_nright, &inputstate);
	CmdAddParam("+up", cmd_pup, &inputstate);
	CmdAddParam("-up", cmd_nup, &inputstate);
	CmdAddParam("+down", cmd_pdown, &inputstate);
	CmdAddParam("-down", cmd_ndown, &inputstate);
	CmdAddParam("+stop", cmd_pstop, &inputstate);
	CmdAddParam("-stop", cmd_nstop, &inputstate);
	CmdAddParam("+tether", cmd_ptether, &inputstate);
	CmdAddParam("-tether", cmd_ntether, &inputstate);
	CmdAddParam("+focus", cmd_pfocus, &inputstate);
	CmdAddParam("-focus", cmd_nfocus, &inputstate);
	CmdAddParam("+eject", cmd_peject, &inputstate);
	CmdAddParam("-eject", cmd_neject, &inputstate);
	CmdAddParam("+sprint", cmd_psprint, &inputstate);
	CmdAddParam("-sprint", cmd_nsprint, &inputstate);
	CmdAddParam("+crouch", cmd_pcrouch, &inputstate);
	CmdAddParam("-crouch", cmd_ncrouch, &inputstate);
	CmdAddParam("+gear", cmd_pgear, &inputstate);
	CmdAddParam("-gear", cmd_ngear, &inputstate);
}

/* All momentary keys are integrated into toggleinputstate regardless of purpose
  because the usage of keys can vary.
  After all, CPU cost for accumulating key state is negligible. */
int MotionFrame(double dt){
	toggleinputstate ^= (previnputstate ^ inputstate) & inputstate;
	return previnputstate = inputstate;
}

int MotionGet(){
	return inputstate;
}

int MotionSet(int mask, int state){
	inputstate |= mask & state;
	return inputstate &= ~mask | state;
}

/* the derivative of the key state */
int MotionGetChange(){
	return previnputstate ^ inputstate;
}

/* the integral of the key state */
int MotionGetToggle(){
	return toggleinputstate;
}

int MotionSetToggle(int mask, int state){
	toggleinputstate |= mask & state;
	return toggleinputstate &= ~mask | state;
}

void MotionAnim(Player &pl, double dt){
	if(inputstate & PL_W)
		pl.velo -= pl.rot.itrans(vec3_001) * dt;
	if(inputstate & PL_S)
		pl.velo += pl.rot.itrans(vec3_001) * dt;
	if(inputstate & PL_A)
		pl.velo -= pl.rot.itrans(vec3_100) * dt;
	if(inputstate & PL_D)
		pl.velo += pl.rot.itrans(vec3_100) * dt;
	if(inputstate & PL_Z)
		pl.velo -= pl.rot.itrans(vec3_010) * dt;
	if(inputstate & PL_Q)
		pl.velo += pl.rot.itrans(vec3_010) * dt;
	if(inputstate & PL_E)
		pl.velo = Vec3d(0,0,0);
	if(inputstate & PL_F){
		pl.fov *= exp((toggleinputstate & PL_F ? 1 : -1) * dt);
		if(1. < pl.fov)
			pl.fov = 1.;
	}
}
