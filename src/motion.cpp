#include "motion.h"
#include "Player.h"
//#include "war.h"
#include "cmd.h"

/** \file
 * \brief Implemetation of toggle and momentary keys.
 *
 * Motion command handling, branched from the main module because it seldom changes but consumes
 * a fair amount of lines.
 */

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
fgather(rotup, PL_8);
fgather(rotdown, PL_2);
fgather(rotleft, PL_4);
fgather(rotright, PL_6);
fgather(rollccw, PL_7);
fgather(rollcw, PL_9);
fgather(stop, PL_E);
fgather(tether, PL_B);
fgather(focus, PL_F);
fgather(enter, PL_ENTER);
fgather(eject, PL_SPACE);
fgather(sprint, PL_SHIFT);
fgather(crouch, PL_CTRL);
fgather(focusset, PL_ALT);
fgather(gear, PL_G);
fgather(lclick, PL_LCLICK);
fgather(rclick, PL_RCLICK);
fgather(move_z, PL_SHIFT);


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
	CmdAddParam("+rotup", cmd_protup, &inputstate);
	CmdAddParam("-rotup", cmd_nrotup, &inputstate);
	CmdAddParam("+rotdown", cmd_protdown, &inputstate);
	CmdAddParam("-rotdown", cmd_nrotdown, &inputstate);
	CmdAddParam("+rotleft", cmd_protleft, &inputstate);
	CmdAddParam("-rotleft", cmd_nrotleft, &inputstate);
	CmdAddParam("+rotright", cmd_protright, &inputstate);
	CmdAddParam("-rotright", cmd_nrotright, &inputstate);
	CmdAddParam("+rollcw", cmd_prollcw, &inputstate);
	CmdAddParam("-rollcw", cmd_nrollcw, &inputstate);
	CmdAddParam("+rollccw", cmd_prollccw, &inputstate);
	CmdAddParam("-rollccw", cmd_nrollccw, &inputstate);
	CmdAddParam("+stop", cmd_pstop, &inputstate);
	CmdAddParam("-stop", cmd_nstop, &inputstate);
	CmdAddParam("+tether", cmd_ptether, &inputstate);
	CmdAddParam("-tether", cmd_ntether, &inputstate);
	CmdAddParam("+focus", cmd_pfocus, &inputstate);
	CmdAddParam("-focus", cmd_nfocus, &inputstate);
	CmdAddParam("+enter", cmd_penter, &inputstate);
	CmdAddParam("-enter", cmd_nenter, &inputstate);
	CmdAddParam("+eject", cmd_peject, &inputstate);
	CmdAddParam("-eject", cmd_neject, &inputstate);
	CmdAddParam("+sprint", cmd_psprint, &inputstate);
	CmdAddParam("-sprint", cmd_nsprint, &inputstate);
	CmdAddParam("+crouch", cmd_pcrouch, &inputstate);
	CmdAddParam("-crouch", cmd_ncrouch, &inputstate);
	CmdAddParam("+focusset", cmd_pfocusset, &inputstate);
	CmdAddParam("-focusset", cmd_nfocusset, &inputstate);
	CmdAddParam("+gear", cmd_pgear, &inputstate);
	CmdAddParam("-gear", cmd_ngear, &inputstate);
	CmdAddParam("+lclick", cmd_plclick, &inputstate);
	CmdAddParam("-lclick", cmd_nlclick, &inputstate);
	CmdAddParam("+rclick", cmd_prclick, &inputstate);
	CmdAddParam("-rclick", cmd_nrclick, &inputstate);
	CmdAddParam("+move_z", cmd_pmove_z, &inputstate);
	CmdAddParam("-move_z", cmd_nmove_z, &inputstate);
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

void MotionAnim(Player &pl, double dt, double accel){
	if(inputstate & PL_F){
		pl.fov *= exp((toggleinputstate & PL_F ? 1 : -1) * dt);
		if(1. < pl.fov)
			pl.fov = 1.;
	}
}
