#include "Warpable.h"
#include "player.h"
#include "bullet.h"
#include "coordsys.h"
#include "viewer.h"
#include "cmd.h"
#include "glwindow.h"
#include "judge.h"
#include "astrodef.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "serial_util.h"
//#include "sensor.h"
extern "C"{
#include "bitmap.h"
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/GL/gldraw.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
}
#include <assert.h>
#include <string.h>



/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289



int g_healthbar = 1;
double g_capacitor_gen_factor = 1.;

/* color sequences */
#if 0
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.45, COLOR32RGBA(255,127,31,95)},
	{2.3, COLOR32RGBA(255,31,0,63)},
};
static const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 3.);
static const struct color_node cnl_shortburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.25, COLOR32RGBA(255,127,31,0)},
};
static const struct color_sequence cs_shortburn = DEFINE_COLSEQ(cnl_shortburn, (COLOR32)-1, 0.5);
#endif


#if 0


static void warp_anim(struct war_field *, double dt);
static void warp_draw(struct war_field *, struct war_draw_data *);
static int warp_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, const struct contact_info *);
static void warp_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo);
static double warp_atmospheric_pressure(warf_t *w, const avec3_t *pos);
static int warp_spherehit(warf_t *w, const avec3_t *pos, double rad, struct contact_info *ci);
static int warp_orientation(warf_t *w, amat3_t *dst, const avec3_t *pos);

static struct war_field_static warp_static = {
	warp_anim,
	WarPostFrame,
	WarEndFrame,
	warp_draw,
	warp_pointhit,
	warp_accel,
	warp_atmospheric_pressure,
	warp_atmospheric_pressure,
	warp_spherehit,
	warp_orientation,
};


#endif

#if 0
static void Warp::anim(struct war_field *w, double dt){
	struct bullet **ppb = &w->bl;
	static int wingfile = 0;
	static int walks = 0, apaches = 2;

	w->effects = 0;

	w->war_time += dt;

	while(*ppb){
		struct bullet *pb = *ppb;
		if(!pb->vft->anim(pb, w, w->tell, dt)){
			pb->active = 0;
		}
		ppb = &pb->next;
	}

	/* tank think */
	{
		entity_t **ppt = &w->tl;
		for(; *ppt; ppt = &(*ppt)->next){
			((struct entity_private_static*)(*ppt)->vft)->anim(*ppt, w, dt);
		}
	}
#if 0

	/* postframe process. do not change activity state while in postframe functions. */
	{
		struct bullet *pb = w->bl;
		for(; pb; pb = pb->next)
			pb->vft->postframe(pb);
	}
	{
		entity_t *pt = w->tl;
		for(; pt; pt = pt->next) if(((struct entity_private_static*)pt->vft)->postframe)
			((struct entity_private_static*)pt->vft)->postframe(pt);
	}

	/* purge inactive entities */
/*	ppb = &w->bl;
	while(*ppb){
		struct bullet *pb = *ppb;
		if(!pb->active){
			if(pb == (struct bullet*)w->pl->chase)
				w->pl->chase = NULL;
			{
				struct bullet *pb = (*ppb)->next;
				free(*ppb);
				*ppb = pb;
			}
			continue;
		}
		ppb = &pb->next;
	}*/
	{
		entity_t **ppt;
		warf_t *dw;
		for(ppt = &w->tl; *ppt; ) if(!(*ppt)->active && !(1 & ((struct entity_private_static*)(*ppt)->vft)->reuse) /*&& ((*ppt)->vft == &flare_s || (*ppt)->vft != &fly_s && (*ppt)->vft != &tank_s)*/){

			/* there are two possibilities to disappear from a warfield, that is
			  to be destroyed or to be transported to another field. we determine
			  this by calling the virtual function designed for this. 
			   note that activity state must be disabled prior to transportation,
			  to force referrers to clear their inter-field references that tend to
			  cause problems due to assumption referrer and referree belongs to the
			  same field. */
			if((*ppt)->vft->warp_dest && (dw = (*ppt)->vft->warp_dest(*ppt, w)) && dw != w){
				entity_t *pt = *ppt;
				*ppt = pt->next;
				pt->next = dw->tl;
				dw->tl = pt;
				pt->active = 1;
			}
			else{
				entity_t *next = (*ppt)->next;
				if(!(2 & ((struct entity_private_static*)(*ppt)->vft)->reuse)){
					if(*ppt == w->pl->chase)
						w->pl->chase = NULL;
					if(*ppt == w->pl->control)
						w->pl->control = NULL;
					if(*ppt == w->pl->selected)
						w->pl->selected = NULL;
					if((*ppt)->vft->destruct)
						(*ppt)->vft->destruct(*ppt);
				}
				if(!(2 & ((struct entity_private_static*)(*ppt)->vft)->reuse))
					free(*ppt);
				*ppt = next;
			}
		}
		else
			ppt = &(*ppt)->next;
	}
#endif

}

static void warp_draw(struct war_field *w, struct war_draw_data *wd){
	amat4_t lightmats[3];
	struct bullet **ppb;

	wd->lightdraws = 0;
	wd->light_on(wd);

	g_gldcache.valid = 0;
	{
		entity_t *pt;
		for(pt = w->tl; pt; pt = pt->next) if(((const struct entity_private_static*)pt->vft)->draw){
			((const struct entity_private_static*)pt->vft)->draw(pt, wd);
		}
	}
	{
		GLfloat whity[4] = {1., 1., 1., 1.};
		gldMaterialfv(GL_FRONT, GL_DIFFUSE, &whity, &g_gldcache);
		gldMaterialfv(GL_FRONT, GL_AMBIENT, &whity, &g_gldcache);
	}
	for(ppb = &w->bl; *ppb; ppb = &(*ppb)->next){
		struct bullet *pb = *ppb;
		pb->vft->drawmodel(pb, wd);
	}
	if(w->gibs){
		struct tent3d_line_drawdata dd;
/*		glPushAttrib(GL_ENABLE_BIT);*/
		glDisable(GL_CULL_FACE);
		VECCPY(dd.viewpoint, wd->view);
		VECCPY(dd.viewdir, wd->viewdir);
		VECNULL(dd.pyr);
		memcpy(dd.invrot, wd->irot, sizeof dd.invrot);
		dd.fov = wd->fov;
		dd.pgc = wd->pgc;
		DrawTeline3D(w->gibs, &dd);
/*		glPopAttrib();*/
	}
/*	glCallList(wd->listLight + 1);*/
	wd->light_off(wd);


	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(0);
	{
		entity_t *pt;
		for(pt = w->tl; pt; pt = pt->next){
			if(((const struct entity_private_static*)pt->vft)->drawtra)
				((const struct entity_private_static*)pt->vft)->drawtra(pt, wd);
		}
	}
	for(ppb = &w->bl; *ppb; ppb = &(*ppb)->next){
		struct bullet *pb = *ppb;
		pb->vft->draw(pb, wd);
	}
	if(w->tepl)
		DrawTefpol3D(w->tepl, wd->view, wd->pgc);

	glPopAttrib();
}

static int warp_pointhit(warf_t *w, const avec3_t *pos, const avec3_t *velo, double dt, struct contact_info *ci){
	return 0;
}

static void warp_accel(warf_t *w, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo){
	VECNULL(*dst);
}

static double warp_atmospheric_pressure(warf_t *w, const avec3_t *pos){
	return 0.;
}

static int warp_spherehit(warf_t *w, const avec3_t *pos, double plrad, struct contact_info *ci){
	return 0;
}

static int warp_orientation(warf_t *w, amat3_t *dst, const avec3_t *pos){
	MATIDENTITY(*dst);
	return 1;
}



#endif














#ifdef NDEBUG
#else
void hitbox_draw(const Entity *pt, const double sc[3]){
	glPushMatrix();
	glScaled(sc[0], sc[1], sc[2]);
	glPushAttrib(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_POLYGON_SMOOTH);
	glColor4ub(255,0,0,255);
	glBegin(GL_LINES);
	{
		int i, j, k;
		for(i = 0; i < 3; i++) for(j = -1; j <= 1; j += 2) for(k = -1; k <= 1; k += 2){
			double v[3];
			v[i] = j;
			v[(i+1)%3] = k;
			v[(i+2)%3] = -1.;
			glVertex3dv(v);
			v[(i+2)%3] = 1.;
			glVertex3dv(v);
		}
	}
	glEnd();
	glPopAttrib();
	glPopMatrix();
}
#endif


void draw_healthbar(Entity *pt, wardraw_t *wd, double v, double scale, double s, double g){
	double x = v * 2. - 1., h = MIN(.1, .1 / (1. + scale)), hs = h / 2.;
	if(!g_healthbar)
		return;
	if(g_healthbar == 1 && wd->w && wd->w->pl){
		Entity *pt2;
		for(pt2 = wd->w->pl->selected; pt2; pt2 = pt2->selectnext) if(pt2 == pt){
			break;
		}
		if(!pt2)
			return;
	}
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glPushMatrix();
	gldTranslate3dv(pt->pos);
	glMultMatrixd(wd->vw->irot);
	gldScaled(scale);
#if 0 /* signal spectrum drawing */
	if(vft->signalSpectrum){
		double x;
		glColor4ub(0,255,127,255);
		glBegin(GL_LINES);
		for(x = -10; x < 10; x++){
			glVertex2d(x / 10., -1);
			glVertex2d(x / 10., -.95);
		}
		glEnd();
		glBegin(GL_LINE_STRIP);
		for(x = -100; x < 100; x++){
			glVertex2d(x / 100., -1 + vft->signalSpectrum(pt, x / 10.));
		}
		glEnd();
	}
#endif
	glBegin(GL_QUADS);
	glColor4ub(0,255,0,255);
	glVertex3d(-1., 1., 0.);
	glVertex3d( x, 1., 0.);
	glVertex3d( x, 1. - h, 0.);
	glVertex3d(-1., 1. - h, 0.);
	glColor4ub(255,0,0,255);
	glVertex3d( x, 1., 0.);
	glVertex3d( 1., 1., 0.);
	glVertex3d( 1., 1. - h, 0.);
	glVertex3d( x, 1. - h, 0.);
	if(0 <= s){
		x = s * 2. - 1.;
		glColor4ub(0,255,255,255);
		glVertex3d(-1., 1. + hs * 2, 0.);
		glVertex3d( x, 1. + hs * 2, 0.);
		glVertex3d( x, 1. + hs, 0.);
		glVertex3d(-1., 1. + hs, 0.);
		glColor4ub(255,0,127,255);
		glVertex3d( x, 1. + hs * 2, 0.);
		glVertex3d( 1., 1. + hs * 2, 0.);
		glVertex3d( 1., 1. + hs, 0.);
		glVertex3d( x, 1. + hs, 0.);
	}
	glEnd();
	if(0 <= g){
		x = g * 2. - 1.;
		glTranslated(0, -2. * h, 0.);
		glBegin(GL_QUADS);
		glColor4ub(255,255,0,255);
		glVertex3d(-1., 1. - hs, 0.);
		glVertex3d( x, 1. - hs, 0.);
		glVertex3d( x, 1., 0.);
		glVertex3d(-1., 1., 0.);
		glColor4ub(255,0,127,255);
		glVertex3d( x, 1. - hs, 0.);
		glVertex3d( 1., 1. - hs, 0.);
		glVertex3d( 1., 1., 0.);
		glVertex3d( x, 1., 0.);
		glEnd();
	}
/*	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/
/*	glBegin(GL_LINE_LOOP);
	glColor4ub(0,255,255,255);
	glVertex3d(-1., 1., 0.);
	glVertex3d( 1, 1., 0.);
	glVertex3d( 1, 1. - h, 0.);
	glVertex3d(-1., 1. - h, 0.);
	glEnd();*/
	glPopMatrix();
	glPopAttrib();
}


void Warpable::drawtra(wardraw_t *wd){
	if(task == sship_moveto){
		glBegin(GL_LINES);
		glColor4ub(0,0,255,255);
		glVertex3dv(pos);
		glColor4ub(0,255,255,255);
		glVertex3dv(dest);
		glEnd();
	}
}



/* maneuvering spaceships, integrated common responses.
 assumption is that accelerations towards all directions except forward movement
 is a half the maximum accel. */
void Warpable::maneuver(const amat4_t mat, double dt, const struct maneuve *mn){
	Entity *pt = this;
	double const maxspeed2 = mn->maxspeed * mn->maxspeed;
	if(!warping){
		if(pt->inputs.press & PL_2){
			VECSADD(pt->omg, &mat[0], dt * mn->angleaccel);
		}
		if(pt->inputs.press & PL_8){
			VECSADD(pt->omg, &mat[0], -dt * mn->angleaccel);
		}
		if(pt->inputs.press & PL_4){
			VECSADD(pt->omg, &mat[4], dt * mn->angleaccel);
		}
		if(pt->inputs.press & PL_6){
			VECSADD(pt->omg, &mat[4], -dt * mn->angleaccel);
		}
		if(pt->inputs.press & PL_7){
			VECSADD(pt->omg, &mat[8], dt * mn->angleaccel);
		}
		if(pt->inputs.press & PL_9){
			VECSADD(pt->omg, &mat[8], -dt * mn->angleaccel);
		}
		if(mn->maxanglespeed * mn->maxanglespeed < VECSLEN(pt->omg)){
			VECNORMIN(pt->omg);
			VECSCALEIN(pt->omg, mn->maxanglespeed);
		}
		if(!(pt->inputs.press & (PL_8 | PL_2 | PL_4 | PL_6 | PL_7 | PL_9))){
			double f;
			f = VECLEN(pt->omg);
			if(f){
				VECSCALEIN(pt->omg, 1. / f);
				f = MAX(0, f - dt * mn->angleaccel);
				VECSCALEIN(pt->omg, f);
			}
		}

		if(pt->inputs.press & PL_W){
			VECSADD(pt->velo, &mat[8], -dt * mn->accel);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(pt->inputs.press & PL_S){
			VECSADD(pt->velo, &mat[8], dt * mn->accel * .5);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(pt->inputs.press & PL_A){
			VECSADD(pt->velo, &mat[0], -dt * mn->accel * .5);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(pt->inputs.press & PL_D){
			VECSADD(pt->velo, &mat[0],  dt * mn->accel * .5);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(pt->inputs.press & PL_Q){
			VECSADD(pt->velo, &mat[4],  dt * mn->accel * .5);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(pt->inputs.press & PL_Z){
			VECSADD(pt->velo, &mat[4], -dt * mn->accel * .5);
			if(VECSLEN(pt->velo) < maxspeed2);
			else{
				VECNORMIN(pt->velo);
				VECSCALEIN(pt->velo, mn->maxspeed);
			}
		}
		if(1){
			double f, dropoff = !(pt->inputs.press & (PL_W | PL_S | PL_A | PL_D | PL_Q | PL_Z)) ? mn->accel : mn->accel * .2;
			f = VECLEN(pt->velo);
			if(f){
				VECSCALEIN(pt->velo, 1. / f);
				f = MAX(0, f - dt * dropoff);
				VECSCALEIN(pt->velo, f);
			}
		}
	}
}

void Warpable::steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed){
	Vec3d target(atarget);
	Vec3d rdr = target - this->pos; // real dr
	Vec3d rdrn = rdr.norm();
	Vec3d dv = targetvelo - this->velo;
	Vec3d dvLinear = rdrn.sp(dv) * rdrn;
	Vec3d dvPlanar = dv - dvLinear;
	double dist = rdr.len();
	if(rdrn.sp(dv) < 0) // estimate only when closing
		target += dvPlanar * dist / dvLinear.len() * .1;
	Vec3d dr = this->pos - target;
	if(rot.trans(-vec3_001).sp(dr) < 0) // burst only when heading closer
		this->inputs.press |= PL_W;
//	this->throttle = dr.len() * speedfactor + minspeed;
	this->omg = 3 * this->rot.trans(vec3_001).vp(dr.norm());
	const maneuve &mn = getManeuve();
	if(mn.maxanglespeed * mn.maxanglespeed < this->omg.slen())
		this->omg.normin().scalein(mn.angleaccel);
	this->rot = this->rot.quatrotquat(this->omg * dt);
}




Warpable::Warpable(WarField *aw) : st(aw), task(sship_idle){
	Warpable::init();
}

void Warpable::init(){
	st::init();
	warpSpeed = /*1e6 * LIGHTYEAR_PER_KILOMETER */5. * AU_PER_KILOMETER;
	warping = 0;
//	warp_next_warf = NULL;
	capacitor = 0.;
	inputs.change = 0;
}

// transit to a CoordSys from another, keeping absolute position and velocity.
int cmd_transit(int argc, char *argv[], void *pv){
	Player *ppl = (Player*)pv;
	Entity *pt;
	if(argc < 2){
		CmdPrintf("Usage: transit dest");
		return 1;
	}
	for(pt = ppl->selected; pt; pt = pt->selectnext){
		Vec3d pos;
		CoordSys *pcs;
		if(pcs = const_cast<CoordSys*>(ppl->cs)->findcspath(argv[1])){
			pt->transit_cs(pcs);
		}
	}
	return 0;
}

int cmd_warp(int argc, char *argv[], void *pv){
	double g_warp_cost_factor = 1.;
	Player *ppl = (Player*)pv;
	Entity *pt;
	if(argc < 2){
		CmdPrintf("Usage: warp dest [x] [y] [z]");
		return 1;
	}
	for(pt = ppl->selected; pt; pt = pt->selectnext){
		Warpable *p = pt->toWarpable();
		if(!p)
			continue;
		WarField *w = p->w;
		if(!p->warping){
			Vec3d delta, pos;
			const CoordSys *pa = NULL;
			CoordSys *pcs;
			double landrad;
			double dist, cost;
			extern coordsys *g_galaxysystem;
			Vec3d dstpos = vec3_000;
			teleport *tp = Player::findTeleport(argv[1], TELEPORT_WARP);
			if(tp){
				pcs = tp->cs;
				pos = tp->pos;
				delta = w->cs->tocs(pos, pcs);
				VECSUBIN(delta, pt->pos);
				VECCPY(dstpos, pos);
				pa = pcs;
			}
			else if(pa = ppl->cs->findcs(argv[1])){
				dstpos[0] = argc < 3 ? 0. : atof(argv[2]);
				dstpos[1] = argc < 4 ? 0. : atof(argv[3]);
				dstpos[2] = argc < 5 ? 0. : atof(argv[4]);
				delta = w->cs->tocs(dstpos, pa);
				VECSUBIN(delta, pt->pos);
			} 
			else
				continue;
			dist = VECLEN(delta);
			cost = g_warp_cost_factor * pt->mass / 1e3 * (log10(dist + 1.) + 1.);
			if(cost < p->capacitor){
				double f;
				int i;
				p->warping = 1;
				p->capacitor -= cost;
	/*			f = VECLEN(delta);
				f = (f - pa->rad * 1.1) / f;
				VECNULL(p->warpdst);
				VECSCALE(p->warpdst, delta, f);
				VECADDIN(p->warpdst, pt->pos);*/
				VECCPY(p->warpdst, dstpos);
				for(i = 0; i < 3; i++)
					p->warpdst[i] += 2. * (drseq(&p->w->rs) - .5);
				p->totalWarpDist = dist;
				p->currentWarpDist = 0.;
				p->warpcs = NULL;
				p->warpdstcs = const_cast<CoordSys*>(pa);
//				p->warp_next_warf = NULL;
			}
		}
	}
	return 0;
}

static int enum_cs_flags(const CoordSys *root, int mask, int flags, const CoordSys ***ret, int left){
	const CoordSys *cs;
	if((root->flags & mask) == (flags & mask)){
		*(*ret)++ = root;
		if(!--left)
			return 0;
	}
	for(cs = root->children; cs; cs = cs->next) if(!(left = enum_cs_flags(cs, mask, flags, ret, left)))
		return left;
	return left;
}



int cmd_togglewarpmenu(int argc, char *argv[], void *){
	extern coordsys *g_galaxysystem;
	char *cmds[64]; /* not much of menu items as 64 are able to displayed after all */
	const char *subtitles[64];
	coordsys *reta[64], **retp = reta;
	static const char *windowtitle = "Warp Destination";
	glwindow *wnd, **ppwnd;
	int left, i;
	ppwnd = GLwindow::findpp(&glwlist, &GLwindow::TitleCmp(windowtitle));
	if(ppwnd){
		glwActivate(ppwnd);
		return 0;
	}
/*	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}*/
/*	for(Player::teleport_iterator it = Player::beginTeleport(), left = 0; it != Player::endTeleport() && left < numof(cmds); it++){
		teleport *tp = Player::getTeleport(it);
		if(!(tp->flags & TELEPORT_WARP))
			continue;
		cmds[left] = (char*)malloc(sizeof "warp \"\"" + strlen(tp->name));
		strcpy(cmds[left], "warp \"");
		strcat(cmds[left], tp->name);
		strcat(cmds[left], "\"");
		subtitles[left] = tp->name;
		left++;
	}*/
	PopupMenu pm;
	for(Player::teleport_iterator it = Player::beginTeleport(); it != Player::endTeleport(); it++){
		teleport *tp = Player::getTeleport(it);
		if(!(tp->flags & TELEPORT_WARP))
			continue;
		pm.append(tp->name, 0, cpplib::dstring("warp \"") << tp->name << '"');
	}
//	wnd = glwMenu(windowtitle, left, subtitles, NULL, cmds, 0);
	wnd = glwMenu(windowtitle, pm, GLW_CLOSE | GLW_COLLAPSABLE);
/*	for(i = 0; i < left; i++){
		free(cmds[i]);
	}*/
/*	left = enum_cs_flags(g_galaxysystem, CS_WARPABLE, CS_WARPABLE, &retp, numof(reta));
	{
		int i;
		for(i = 0; i < numof(cmds) - left; i++){
			cmds[i] = malloc(sizeof "warp \"\"" + strlen(reta[i]->name));
			strcpy(cmds[i], "warp \"");
			strcat(cmds[i], reta[i]->name);
			strcat(cmds[i], "\"");
			subtitles[i] = reta[i]->fullname ? reta[i]->fullname : reta[i]->name;
		}
	}
	wnd = glwMenu(numof(reta) - left, subtitles, NULL, cmds, 0);*/
//	wnd->title = windowtitle;
	return 0;
}

int Warpable::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.appendSeparator().append("Warp to...", 'w', "togglewarpmenu");
	return ret;
}

Warpable *Warpable::toWarpable(){
	return this;
}

Entity::Props Warpable::props()const{
	Props ret = st::props();
	ret.push_back(cpplib::dstring("Capacitor: ") << capacitor << '/' << maxenergy());
	return ret;
}

void Warpable::control(input_t *inputs, double dt){
	Warpable *p = this;

	/* camera distance is always configurable. */
/*	if(inputs->press & PL_MWU){
		g_viewdist *= VIEWDIST_FACTOR;
	}
	if(inputs->press & PL_MWD){
		g_viewdist /= VIEWDIST_FACTOR;
	}*/

	if(!w || health <= 0.)
		return;
	this->inputs = *inputs;

/*	if(inputs->change & inputs->press & PL_G)
		p->menu = !p->menu;*/

#if 0
	if(p->menu && !p->warping){
		extern coordsys *g_sun_low_orbit, *g_iserlohnsystem, *g_rwport, *g_earthorbit, *g_lagrange1, *g_moon_low_orbit, *g_jupiter_low_orbit, *g_saturn_low_orbit;
		double landrad;
		avec3_t dstpos = {.0};
		coordsys *pa = NULL;
		if(0);
/*		else if(inputs->change & inputs->press & PL_D){
			pa = g_iserlohnsystem;
			dstpos[0] = +75.;
		}*/
		else if(inputs->change & inputs->press & PL_B){
			pa = g_rwport;
		}
		else if(inputs->change & inputs->press & PL_A){
			pa = g_earthorbit;
			dstpos[1] = 1.;
		}
		else if(inputs->change & inputs->press & PL_S){
			pa = g_lagrange1;
			dstpos[0] = +15.;
			dstpos[1] = -22.;
		}
		else if(inputs->change & inputs->press & PL_D){
			pa = g_moon_low_orbit;
		}
		else if(inputs->change & inputs->press & PL_W){
			pa = g_jupiter_low_orbit;
		}
		else if(inputs->change & inputs->press & PL_Z){
			pa = g_saturn_low_orbit;
		}
		else if(inputs->change & inputs->press & PL_Q){
			pa = g_sun_low_orbit;
		}
		if(pa){
			avec3_t delta;
			double f;
			int i;
			p->warping = 1;
			tocs(delta, w->cs, pa->pos, pa);
			VECSUBIN(delta, pt->pos);
			f = VECLEN(delta);
			f = (f - pa->rad * 1.1) / f;
/*			VECNULL(p->warpdst);
			VECSCALE(p->warpdst, delta, f);
			VECADDIN(p->warpdst, pt->pos);*/
			VECCPY(p->warpdst, dstpos);
			for(i = 0; i < 3; i++)
				p->warpdst[i] += 2. * (drseq(&w->rs) - .5);
			p->menu = 0;
			p->warpcs = NULL;
			p->warpdstcs = pa;
			p->warp_next_warf = NULL;
		}
	}
#endif
}

unsigned Warpable::analog_mask(){
	return 0;
}

#if 0
static void warpable_drawHUD(entity_t *pt, warf_t *wf, wardraw_t *wd, const double irot[16], void (*gdraw)(void)){
	warpable_t *p = (warpable_t*)pt;
/*	base_drawHUD_target(pt, wf, wd, gdraw);*/
	base_drawHUD(pt, wf, wd, irot, gdraw);
	glPushMatrix();
	glPushAttrib(GL_CURRENT_BIT);
	if(p->warping){
		avec3_t warpdstpos, eyedelta;
		tocs(warpdstpos, wf->cs, p->warpdst, p->warpdstcs);
		VECSUB(eyedelta, warpdstpos, wd->view);
		VECNORMIN(eyedelta);
		glLoadMatrixd(wd->rot);
		glRasterPos3dv(eyedelta);
		gldprintf("warpdst");
	}
	glLoadIdentity();

	{
		GLint vp[4];
		int w, h, m, mi, i;
		double left, bottom, velo;
		amat3_t ort;

		glGetIntegerv(GL_VIEWPORT, vp);
		w = vp[2], h = vp[3];
		m = w < h ? h : w;
		mi = w < h ? w : h;
		left = -(double)w / m;
		bottom = -(double)h / m;

		velo = p->warping && p->warpcs ? VECLEN(p->warpcs->velo) : VECLEN(pt->velo);
		wf->vft->orientation(wf, &ort, pt->pos);
		glRasterPos3d(left + 20. / m, -bottom - 100. / m, -1);
		gldprintf("%lg km/s", velo);
		glRasterPos3d(left + 20. / m, -bottom - 120. / m, -1);
		gldprintf("%lg kt", 1944. * velo);

/*		if(p->menu){
			glRasterPos3d(left + 00. / m, bottom + 140. / m, -1);
			gldprintf("Q Sun");
			glRasterPos3d(left + 00. / m, bottom + 120. / m, -1);
			gldprintf("Z Saturn");
			glRasterPos3d(left + 00. / m, bottom + 100. / m, -1);
			gldprintf("A Earth");
			glRasterPos3d(left + 00. / m, bottom + 80. / m, -1);
			gldprintf("S Island 3");
			glRasterPos3d(left + 00. / m, bottom + 60. / m, -1);
			gldprintf("D Moon");
			glRasterPos3d(left + 00. / m, bottom + 40. / m, -1);
			gldprintf("W Jupiter");
		}*/

		if(p->warping){
			double desiredvelo, velo, f, dist;
			double (*cuts)[2];
			char buf[128];
			avec3_t *pvelo = p->warpcs ? &p->warpcs->velo : &pt->velo;
			avec3_t dstcspos, warpdst, delta; /* current position measured in destination coordinate system */
			int iphase;
			tocs(dstcspos, p->warpdstcs, pt->pos, wf->cs);
			tocs(warpdst, wf->cs, p->warpdst, p->warpdstcs);

			velo = VECLEN(*pvelo);
			VECSUB(delta, p->warpdst, dstcspos);
			dist = VECLEN(delta);
			p->totalWarpDist = p->currentWarpDist + dist;
			cuts = CircleCuts(32);
			glTranslated(0, -.5, -1);
			glScaled(.25, .25, 1.);

			f = log10(velo / p->warpSpeed);
			iphase = -8 < f ? 8 + f : 0;

			/* speed meter */
			glColor4ub(127,127,127,255);
			glBegin(GL_QUAD_STRIP);
			for(i = 4; i <= 4 + iphase; i++){
				glVertex2d(-cuts[i][1], cuts[i][0]);
				glVertex2d(-.5 * cuts[i][1], .5 * cuts[i][0]);
			}
			glEnd();

			glColor4ub(255,255,255,255);
			glBegin(GL_LINE_LOOP);
			for(i = 4; i <= 12; i++){
				glVertex2d(cuts[i][1], cuts[i][0]);
			}
			for(i = 12; 4 <= i; i--){
				glVertex2d(.5 * cuts[i][1], .5 * cuts[i][0]);
			}
			glEnd();

			/* progress */
			glColor4ub(127,127,127,255);
			glBegin(GL_QUADS);
			glVertex2d(-1., -.5);
			glVertex2d(p->currentWarpDist / p->totalWarpDist * 2. - 1., -.5);
			glVertex2d(p->currentWarpDist / p->totalWarpDist * 2. - 1., -.7);
			glVertex2d(-1., -.7);
			glEnd();

			glColor4ub(255,255,255,255);
			glBegin(GL_LINE_LOOP);
			glVertex2d(-1., -.5);
			glVertex2d(1., -.5);
			glVertex2d(1., -.7);
			glVertex2d(-1., -.7);
			glEnd();

			sprintf(buf, "%lg%%", p->currentWarpDist / p->totalWarpDist * 100.);
			glRasterPos2d(- 0. / w, -.4 + 16. * 4 / h);
			gldPutString(buf);
			if(velo != 0){
				double eta = (p->totalWarpDist - p->currentWarpDist) / p->warpSpeed;
				eta = fabs(eta);
				if(3600 * 24 < eta)
					sprintf(buf, "ETA: %d days %02d:%02d:%02lg", (int)(eta / (3600 * 24)), (int)(fmod(eta, 3600 * 24) / 3600), (int)(fmod(eta, 3600) / 60), fmod(eta, 60));
				else if(60 < eta)
					sprintf(buf, "ETA: %02d:%02d:%02lg", (int)(eta / 3600), (int)(fmod(eta, 3600) / 60), fmod(eta, 60));
				else
					sprintf(buf, "ETA: %lg sec", eta);
				glRasterPos2d(- 8. / w, -.4);
				gldPutString(buf);
			}
		}
	}

	glPopAttrib();
	glPopMatrix();
}



static warf_t *warpable_warp_dest(entity_t *pt, const warf_t *w){
	warpable_t *p = ((warpable_t*)pt);
	if(!p->warping)
		return NULL;
	return p->warp_next_warf;
}
#endif


void Warpable::warp_collapse(){
	int i;
	Warpable *p = this;
	Entity *pt2;
	Vec3d dstcspos; /* current position measured in destination coordinate system */
/*	tocs(dstcspos, p->warpdstcs, pt->pos, w->cs);
	VECCPY(pt->pos, dstcspos);
	tocsv(dstcspos, p->warpdstcs, pt->velo, pt->pos, w->cs);
	VECCPY(pt->velo, dstcspos);*/
/*	transit_cs(pt, w, p->warpdstcs);*/
	for(pt2 = w->el; pt2; pt2 = pt2->next)
		transit_cs(p->warpdstcs);
	if(p->warpcs){
/*		for(i = 0; i < npf; i++){
			if(pf[i])
				ImmobilizeTefpol3D(pf[i]);
			pf[i] = AddTefpolMovable3D(p->warpdstcs->w->tepl, p->st.pos, pt->velo, avec3_000, &cs_orangeburn, TEP3_THICKEST | TEP3_ROUGH, cs_orangeburn.t);
		}*/
		p->warpcs->flags |= CS_DELETE;
		p->warpcs = NULL;
	}
}

const Warpable::maneuve Warpable::mymn = {
	0, // double accel;
	0, // double maxspeed;
	0, // double angleaccel;
	0, // double maxanglespeed;
	0, // double capacity; /* capacity of capacitor [MJ] */
	0, // double capacitor_gen; /* generated energy [MW] */
};
const Warpable::maneuve &Warpable::getManeuve()const{
	return mymn;
}
bool Warpable::isTargettable()const{
	return true;
}
bool Warpable::isSelectable()const{return true;}

void Warpable::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << capacitor; /* Temporarily stored energy, MegaJoules */
	sc.o << dest;
	sc.o << warping;
	if(warping){
		sc.o << warpdst;
		sc.o << warpSpeed;
		sc.o << totalWarpDist << currentWarpDist;
		sc.o << warpcs << warpdstcs;
	}
	sc.o << task;
}

void Warpable::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> capacitor; /* Temporarily stored energy, MegaJoules */
	sc.i >> dest;
	sc.i >> warping;
	if(warping){
		sc.i >> warpdst;
		sc.i >> warpSpeed;
		sc.i >> totalWarpDist >> currentWarpDist;
		sc.i >> warpcs >> warpdstcs;
	}
	sc.i >> (int&)task;
}

void Warpable::anim(double dt){
	Mat4d mat;
	transform(mat);
	Warpable *const p = this;
	Entity *pt = this;
	const maneuve *mn = &getManeuve();

	if(p->warping){
		double desiredvelo, velo;
		Vec3d *pvelo = p->warpcs ? &p->warpcs->velo : &pt->velo;
		Vec3d dstcspos, warpdst; /* current position measured in destination coordinate system */
		double sp, scale;
		dstcspos = p->warpdstcs->tocs(pt->pos, w->cs);
		warpdst = w->cs->tocs(p->warpdst, p->warpdstcs);
		{
			Quatd qc;
			Vec3d omega, dv, forward;
			dv = warpdst - pt->pos;
			dv.normin();
			forward = pt->rot.trans(avec3_001);
			forward *= -1.;
			sp = dv.sp(forward);
			omega = dv.vp(forward);
			if(sp < 0.){
				omega += mat.vec3(0) * mn->angleaccel;
			}
			scale = -.5 * dt;
			if(scale < -1.)
				scale = -1.;
			omega *= scale;
			pt->rot = pt->rot.quatrotquat(omega);
		}
		velo = (*pvelo).len();
		desiredvelo = .5 * VECDIST(warpdst, dstcspos);
		desiredvelo = MIN(desiredvelo, p->warpSpeed);
/*		desiredvelo = desiredvelo < 5. ? desiredvelo * desiredvelo / 5. : desiredvelo;*/
/*		desiredvelo = MIN(desiredvelo, 1.47099e8);*/
		if((warpdst - dstcspos).slen() < 1. * 1.){
			warp_collapse();
			p->warping = 0;
			pt->velo.clear();
			if(w->pl->chase == pt){
				w->pl->velo = w->pl->cs->tocsv(pt->velo, pt->pos, w->cs);
			}
		}
		else if(desiredvelo < velo){
			Vec3d delta, dst, dstvelo;
			double dstspd, u, len;
			delta = p->warpdst - dstcspos;
/*			VECSUB(delta, warpdst, pt->pos);*/
			len = delta.len();
#if 1
			u = desiredvelo / len;
			dstvelo = delta * u;
			*pvelo = dstvelo;
#else
			u = -exp(-desiredvelo * dt);
			VECSCALE(dst, delta, u);
			VECADDIN(dst, warpdst);
			VECSUB(*pvelo, dst, pt->pos);
			VECSCALEIN(*pvelo, 1. / dt);
#endif
			if(p->warpcs){
#if 1
				p->warpcs->adopt_child(p->warpdstcs);
#else
				avec3_t pos, velo;
				int i;
				for(i = 0; i < numof(p->warpcs->children); i++) if(p->warpcs->parent->children[i] == p->warpcs)
					break;
				assert(i != numof(p->warpcs->children));
				tocs(pos, p->warpdstcs, p->warpcs->pos, p->warpcs->parent);
				tocsv(velo, p->warpdstcs, p->warpcs->velo, p->warpcs->pos, p->warpcs->parent);
/*				p->warpcs->parent->children[i] = NULL;*/
				memmove(&p->warpcs->parent->children[i], &p->warpcs->parent->children[i+1], (numof(p->warpcs->parent->children) - i - 1) * sizeof p->warpcs);
				p->warpcs->parent->nchildren--;
				VECCPY(p->warpcs->pos, pos);
				VECCPY(p->warpcs->velo, velo);
				p->warpcs->parent = p->warpdstcs;
				legitimize_child(p->warpcs);
#endif
			}
		}
		else if(.9 < sp){
			avec3_t delta, dstvelo;
			double dstspd, u, len;
			const double L = LIGHT_SPEED;
			VECSUB(delta, warpdst, pt->pos);
			len = VECLEN(delta);
			u = (velo + .5) * 1e1 /** 5e-1 * (len - p->sight->rad)*/;
	/*		u = L - L * L / (u + L);*/
	/*		dstspd = (u + v) / (1 + u * v / LIGHT_SPEED / LIGHT_SPEED);*/
			VECNORMIN(delta);
			VECSCALE(dstvelo, delta, u);
			VECSUBIN(dstvelo, *pvelo);
			VECSADD(*pvelo, dstvelo, .2 * dt);
	/*		VECSUB(delta, dstvelo, p->velo);
			VECCPY(p->velo, dstvelo);*/
	/*		VECSADD(p->velo, delta, dt * 1e-8 * (1e0 + VECLEN(p->velo)));*/
		}
		if(!p->warpcs && w->cs != p->warpdstcs && w->cs->csrad * w->cs->csrad < VECSLEN(pt->pos)){
			int i;
			p->warpcs = new CoordSys("warpbubble", w->cs);//malloc(sizeof *p->warpcs + sizeof *p->warpcs->w);
			p->warpcs->pos = pt->pos;
			p->warpcs->velo = pt->velo;
/*			VECNULL(pt->velo);*/
			p->warpcs->csrad = pt->hitradius() * 10.;
			p->warpcs->flags = 0;
			p->warpcs->w = new WarField(p->warpcs);
			p->warpcs->w->pl = w->pl;
/*			for(i = 0; i < npf; i++) if(pf[i]){
				ImmobilizeTefpol3D(pf[i]);
				pf[i] = NULL;
			}*/
			transit_cs(p->warpcs);
#if 0
			/* TODO: bring docked objects along */
			if(pt->vft == &scarry_s){
				entity_t **ppt2;
				for(ppt2 = &w->tl; *ppt2; ppt2 = &(*ppt2)->next){
					entity_t *pt2 = *ppt2;
					if(pt2->vft == &scepter_s && ((scepter_t*)pt2)->mother == p){
						if(((scepter_t*)*ppt2)->docked)
							transit_cs(pt2, w, p->warpcs);
						else
							((scepter_t*)*ppt2)->mother = NULL;
/*						if(((scepter_t*)*ppt2)->docked){
							transit_cs
							*ppt2 = pt2->next;
							pt2->next = p->warpcs->w->tl;
							p->warpcs->w->tl = pt2;
						}*/
					}
				}
			}
#endif
		}
		else{
			if(w->cs == p->warpcs && dstcspos.slen() < p->warpdstcs->csrad * p->warpdstcs->csrad)
				warp_collapse();
		}

		if(p->warpcs){
			p->currentWarpDist += VECLEN(p->warpcs->velo) * dt;
			VECSADD(p->warpcs->pos, p->warpcs->velo, dt);
		}
		pos += this->velo * dt;
	}
	else{ /* Regenerate energy in capacitor only unless warping */
		double gen = dt * g_capacitor_gen_factor * mn->capacitor_gen;
		if(p->capacitor + gen < mn->capacity)
			p->capacitor += gen;
		else
			p->capacitor = mn->capacity;

		if(task == sship_moveto){
			steerArrival(dt, dest, vec3_000, .1, .01);
			if((pos - dest).slen() < hitradius() * hitradius()){
				task = sship_idle;
				inputs.press = 0;
			}
		}

		maneuver(mat, dt, mn);

		pos += velo * dt;
		rot = rot.quatrotquat(omg * dt);
	/*	VECSCALEIN(pt->omg, 1. / (dt * .4 + 1.));
		VECSCALEIN(pt->velo, 1. / (dt * .01 + 1.));*/
	}
	st::anim(dt);
}

bool Warpable::command(unsigned comid, std::set<Entity*> *arg){
	if(comid == cid_halt){
		task = sship_idle;
		inputs.press = 0;
		return true;
	}
	else if(comid == cid_move){
		task = sship_moveto;
		dest = *(Vec3d*)arg;
		return true;
	}
	else if(comid == cid_attack){
		if(arg && !arg->empty())
			enemy = *arg->begin();
	}
	return false;
}
