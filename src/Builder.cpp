/** \file
 * \brief Implementation of Builder, GLWbuild and BuildCommand classes.
 */
#include "Scarry.h"
#include "judge.h"
#include "serial_util.h"
#include "Player.h"
#include "antiglut.h"
#include "Sceptor.h"
#include "Beamer.h"
#include "sqadapt.h"
#include "glw/popup.h"
#include "Application.h"
#include "StaticInitializer.h"
extern "C"{
#include <clib/mathdef.h>
}

/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289
#define SQRT2P2 (M_SQRT2/2.)

/// The multiplier on the build speed for debugging.
static double g_buildtimescale = 1.;

const Builder::BuildStatic sceptor_build = {
	"Interceptor",
	Sceptor::create,
	10.,
	60.,
};

const Builder::BuildStatic *Builder::builder0[] = {/*&scontainer_build, &worker_build,*/ &sceptor_build, /*&Beamer::builds, &Assault::builds*/};
const unsigned Builder::nbuilder0 = numof(builder0);

void Builder::serialize(SerializeContext &sc){
	// Do NOT call Entity::serialize here because this class is a branch class.
	sc.o << build;
	sc.o << nbuildque;
	for(int i = 0; i < nbuildque; i++){
		sc.o << buildque[i].num/* << buildque[i].st*/;
	}
}

void Builder::unserialize(UnserializeContext &sc){
	// Do NOT call Entity::unserialize here because this class is a branch class.
	sc.i >> build;
	sc.i >> nbuildque;
	for(int i = 0; i < nbuildque; i++){
		sc.i >> buildque[i].num/* >> buildque[i].st*/;
		buildque[i].st = &sceptor_build;
	}
}

bool Builder::addBuild(const BuildStatic *st){
	if(numof(buildque) <= nbuildque && buildque[nbuildque-1].st != st)
		return false;
	if(buildque[nbuildque-1].st == st)
		buildque[nbuildque-1].num++;
	else{
		buildque[nbuildque].st = st;
		buildque[nbuildque].num = 1;
		if(0 == nbuildque)
			build = buildque[nbuildque].st->buildtime;
		nbuildque++;
	}
	return true;
}

bool Builder::cancelBuild(int index, bool recalc){
	if(index < 0 || nbuildque <= index)
		return false;
	if(!--buildque[index].num){
		::memmove(&buildque[index], &buildque[index+1], (--nbuildque - index) * sizeof *buildque);
	}
	if(recalc && index == 0 && nbuildque)
		build = buildque[0].st->buildtime;
	return true;
}

void Builder::doneBuild(Entity *child){}

void Builder::anim(double dt){
	dt *= g_buildtimescale;

	if(nbuildque){
		// If resource units gonna run out, stop construction then.
		if(ru < buildque[0].st->cost * (buildque[0].st->buildtime - build + dt) / buildque[0].st->buildtime){
			dt = ru * buildque[0].st->buildtime / buildque[0].st->cost + build - buildque[0].st->buildtime;
		}
	}

	while(nbuildque && build < dt){
//		ru -= buildque[0].st->cost * build / buildque[0].st->buildtime;
		// If we just subtract the actual RU value every frame, we could have it
		// not equal to RUs before start building subtracted by the unit cost,
		// because variable frame time's calculation errors build up.
		// So we don't really subtract the value until the construction is
		// complete.  Instead, we show the RUs partially subtracted by
		// calculating the amount every time requested.  See getRU().
		ru -= buildque[0].st->cost;
		dt -= build;
		Entity *created = buildque[0].st->create(this->w, this);
		doneBuild(created);
		cancelBuild(0, false);
		build += buildque[0].st->buildtime;
	}
	if(nbuildque){
//		ru -= buildque[0].st->cost * dt / buildque[0].st->buildtime;
		build -= dt;
	}
}

double Builder::getRU()const{
	if(nbuildque)
		return ru - buildque[0].st->cost * (buildque[0].st->buildtime - build) / buildque[0].st->buildtime;
	else
		return ru;
}

bool Builder::command(EntityCommand *com){
	if(BuildCommand *bc = InterpretCommand<BuildCommand>(com)){
		for(int i = 0; i < numof(builder0); i++){
			if(bc->buildOrder == builder0[i]->name){
				addBuild(builder0[i]);
			}
		}
		return true;
	}
	return false;
}


IMPLEMENT_COMMAND(BuildCommand, "BuildCommand");

void BuildCommand::serialize(SerializeContext &sc){
	sc.o << buildOrder;
}

void BuildCommand::unserialize(UnserializeContext &sc){
	sc.i >> buildOrder;
}


#ifndef DEDICATED
static int select_tab(GLwindow *wnd, int ix, int iy, const char *s, int *selected, int mx, int my){
	int ix0 = ix;
	ix += 3;
	ix += 8 * strlen(s);
	ix += 3;
	*selected = ix0 < mx && mx < ix && iy < my + 12 && my + 12 < iy + 12;
	return ix;
}

int GLWbuild::mouse(GLwindowState &ws, int mbutton, int state, int mx, int my){
	GLWbuild *p = this;
	int ind, sel0, sel1, ix;
	int fonth = (int)getFontHeight();
	if(st::mouse(ws, mbutton, state, mx, my))
		return 1;
	ind = (my - 3 * 12) / 12;
	if((mbutton == GLUT_RIGHT_BUTTON || mbutton == GLUT_LEFT_BUTTON) && state == GLUT_UP || mbutton == GLUT_WHEEL_UP || mbutton == GLUT_WHEEL_DOWN){
		int num = 1, i;
#ifdef _WIN32
		if(HIBYTE(GetKeyState(VK_SHIFT)))
			num = 10;
#endif
		ix = select_tab(this, ix = 2, 2 * fonth, "Build", &sel0, mx, my);
		select_tab(this, ix, 2 * fonth, "Queue", &sel1, mx, my);
		if(sel0) p->tabindex = 0;
		if(sel1) p->tabindex = 1;
		if(p->tabindex == 0){
#if 1
			if(0 <= ind && ind < Builder::nbuilder0){
				builder->addBuild(builder->builder0[ind]);
				if(!game->isServer()){
					BuildCommand com(builder->builder0[ind]->name);
					CMEntityCommand::s.send(builder->toEntity(), com);
				}
			}
#else
			if(0 <= ind && ind < 1) for(i = 0; i < num; i++) switch(ind){
				case 0:
/*					add_buildque(p->p, &scontainer_build);
					break;
				case 1:
					add_buildque(p->p, &worker_build);
					break;
				case 2:*/
					builder->addBuild(&sceptor_build);
					break;
/*				case 3:
					add_buildque(p->p, &beamer_build);
					break;
				case 4:
					if(mbutton == GLUT_RIGHT_BUTTON){
						int j;
						glwindow *wnd2;
						for(j = 0; j < p->p->narmscustom; j++) if(p->p->armscustom[j].builder == &assault_build)
							break;
						if(j == p->p->narmscustom){
							p->p->armscustom = realloc(p->p->armscustom, ++p->p->narmscustom * sizeof *p->p->armscustom);
							p->p->armscustom[j].builder = &assault_build;
							p->p->armscustom[j].c = numof(assault_hardpoints);
							p->p->armscustom[j].a = malloc(numof(assault_hardpoints) * sizeof *p->p->armscustom[j].a);
							memcpy(p->p->armscustom[j].a, assault_arms, numof(assault_hardpoints) * sizeof *p->p->armscustom[j].a);
						}
						wnd2 = ArmsShowWindow(AssaultNew, assault_build.cost, offsetof(assault_t, arms), p->p->armscustom[j].a, assault_hardpoints, numof(assault_hardpoints));
						wnd->modal = wnd2;
					}
					else
						add_buildque(p->p, &assault_build);
					break;*/
			}
#endif
		}
		else{
			if(builder->build != 0. && ind == 0 || ind == 1){
				builder->build = 0.;
				builder->cancelBuild(ind);
			}
/*			else if(2 <= ind && ind < pf->nbuildque + 2){
				int i;
				int ptr;
				ind -= 2;
				ptr = (pf->buildque0 + ind) % numof(pf->buildque);
				if(1 < pf->buildquenum[ptr])
					pf->buildquenum[ptr]--;
				else{
					for(i = ind; i < pf->nbuildque - 1; i++){
						int ptr = (pf->buildque0 + i) % numof(pf->buildque);
						int ptr1 = (pf->buildque0 + i + 1) % numof(pf->buildque);
						pf->buildque[ptr] = pf->buildque[ptr1];
						pf->buildquenum[ptr] = pf->buildquenum[ptr1];
					}
					pf->nbuildque--;
				}
			}*/
		}
		return 1;
	}
	return 0;
}

void GLWbuild::postframe(){
	if(builder && !builder->w)
		builder = NULL;
}

int cmd_build(int argc, char *argv[]){
	if(!application.clientGame)
		return 0;
	Player *player = application.clientGame->player;
	if(!player && player->selected.empty())
		return 0;
	Builder *pb = (*player->selected.begin())->getBuilder();
	if(pb)
		glwAppend(new GLWbuild(application.clientGame, "Build", pb));
	return 0;
}
#endif

static void register_build(){
#ifndef DEDICATED
	CmdAdd("buildmenu", cmd_build);
#endif
	CvarAdd("g_buildtimescale", &g_buildtimescale, cvar_double);
}

static StaticInitializer init(register_build);


