/** \file
 * \brief Implementation of RStation class.
 */
#include "RStation.h"
#include "EntityRegister.h"
#include "Warpable.h"
#include "Player.h"
#include "Bullet.h"
#include "CoordSys.h"
#include "cmd.h"
#include "serial_util.h"
#include "glw/PopupMenu.h"
#include "tent3d.h"
#include "SqInitProcess-ex.h"
extern "C"{
#include <clib/mathdef.h>
}

#ifdef NDEBUG
#define hitbox_draw
#endif



double g_rstation_occupy_time = 10.;

double RStation::modelScale = 0.1;
double RStation::hitRadius = 4.;
double RStation::defaultMass = 1e7;
double RStation::maxHealthValue = 1500000.;
HitBoxList RStation::hitboxes;
HSQOBJECT RStation::overlayProc = sq_nullobj();
ModelEntity::NavlightList RStation::navlights;
double RStation::maxRU = 10000.;
double RStation::defaultRU = 1000.; ///< Initial RUs

RStation::RStation(WarField *aw) : st(aw){
	init();
	mass = defaultMass;
	race = -1;
	health = getMaxHealth();
	ru = defaultRU;
	occupytime = g_rstation_occupy_time;
	occupyrace = -1;
}

void RStation::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, _SC("models/RStation.nut"),
			ModelScaleProcess(modelScale) <<=
			SingleDoubleProcess(hitRadius, "hitRadius", false) <<=
			MassProcess(defaultMass) <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			HitboxProcess(hitboxes) <<=
			SqCallbackProcess(overlayProc, "drawOverlay") <<=
			NavlightsProcess(navlights) <<=
			SingleDoubleProcess(maxRU, "maxRU") <<=
			SingleDoubleProcess(defaultRU, "defaultRU"));
		initialized = true;
	}
}

const char *RStation::idname()const{return "rstation";}
const char *RStation::classname()const{return "RStation";}
const unsigned RStation::classid = registerClass("RStation", Conster<RStation>);
Entity::EntityRegister<RStation> RStation::entityRegister("RStation");
const char *RStation::dispname()const{return "Resource St.";}
bool RStation::isTargettable()const{return true;}
bool RStation::isSelectable()const{return true;}
double RStation::getMaxHealth()const{return maxHealthValue;}

void RStation::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << ru << occupytime << occupyrace;
}

void RStation::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> ru >> occupytime >> occupyrace;
}

/*double *__fastcall rstation_ru(rstation_t *p){
	return &p->ru;
}*/

double RStation::getHitRadius()const{
	return hitRadius;
}

void RStation::cockpitview(Vec3d &pos, Quatd &rot, int seatid)const{
	Player *player = game->player;
	double g_viewdist = 1.;
	Vec3d ofs, src[3] = {Vec3d(0., .001, -.002), Vec3d(0., 0., 10.)};
	Mat4d mat;
	Quatd q;
	seatid = (seatid + 3) % 3;
	if(seatid == 1){
		double f;
		q = this->rot * player->getrot();
		src[2][2] = .1 * g_viewdist;
		f = src[2][2] * .001 < .001 ? src[2][2] * .001 : .001;
		src[2][2] += f * sin(player->gametime * M_PI / 2. + M_PI / 2.);
		src[2][0] = f * sin(player->gametime * M_PI / 2.);
		src[2][1] = f * cos(player->gametime * M_PI / 2.);
	}
	else
		q = this->rot;
	ofs = q.trans(src[seatid]);
	pos = this->pos + ofs;
}

/*static int rstation_getrot(struct entity *pt, warf_t *w, double (*rot)[16]){
	quat2imat(*rot, pt->rot);
	return w->pl->control != pt;
}*/

int RStation::popupMenu(PopupMenu &pm){
	pm.appendSeparator();
	return 0;
}

void RStation::anim(double dt){
	RStation *p = this;
	Entity *pt2 = NULL;
	int orace = -1, mix = 0;

	if(p->ru + dt * 2. < maxRU)
		p->ru += dt * 2.;
	else
		p->ru = maxRU;

	/* Rstations can only be occupied by staying space vehicles in distance of
	  5 kilometers with no other races. */
	for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++){
		if(*it){
			Entity *e = *it;
			if(this != e && w == e->w && 0 <= e->race &&
				(this->pos - e->pos).slen() < 8. * 8.)
			{
				if(orace < 0)
					orace = e->race;
				else if(orace != e->race){
					pt2 = e;
					break;
				}
			}
		}
	}
	if(!pt2 && (0 <= orace) && orace != this->race){
		p->occupyrace = this->race < 0 ? orace : -1;
		if(p->occupytime < dt){
			this->race = p->occupyrace;
			p->occupytime = 0.;
		}
		else
			p->occupytime -= dt;
	}
	else{
		if(p->occupytime + dt < g_rstation_occupy_time)
			p->occupytime += dt;
		else
			p->occupytime = g_rstation_occupy_time;
	}
}


int RStation::takedamage(double damage, int hitpart){
	Teline3List *tell = w->getTeline3d();
	int ret = 1;
	if(this->health < 0.)
		return 1;
//	playWave3D("hit.wav", this->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < health && health - damage <= 0){
		int i;
		ret = 0;
		Vec3d accel = w->accel(pos, velo);
/*		effectDeath(w, pt);*/
		if(tell) for(i = 0; i < 32; i++){
			Vec3d pos, velo;
			velo[0] = drseq(&w->rs) - .5;
			velo[1] = drseq(&w->rs) - .5;
			velo[2] = drseq(&w->rs) - .5;
			velo.normin();
			pos = this->pos + velo * .1 * .1;
			AddTeline3D(tell, pos, velo, .005, quat_u, vec3_000, accel, COLOR32RGBA(255, 31, 0, 255), TEL3_HEADFORWARD | TEL3_THICK | TEL3_FADEEND | TEL3_REFLECT, 1.5 + drseq(&w->rs));
		}
		health = -2.;
	}
	else
		health -= damage;
	return ret;
}

int RStation::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
	for(n = 0; n < hitboxes.size(); n++){
		HitBox &hb = hitboxes[n];
		Vec3d org;
		Quatd rot;
		org = this->rot.itrans(hb.org);
		org += this->pos;
		rot = this->rot * hb.rot;
		for(i = 0; i < 3; i++)
			sc[i] = hb.sc[i] + rad;
		Vec3d n;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, &n)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = i + 1;
			if(retn) *retn = n;
		}
	}
	return reti;
}

Entity::Props RStation::props()const{
	Props ret = st::props();
//	ret.push_back(cpplib::dstring("RUs: ") << ru << "/" << RSTATION_MAX_RU);
	return ret;
}

#ifdef DEDICATED
void RStation::draw(WarDraw*){}
void RStation::drawtra(WarDraw*){}
#endif
