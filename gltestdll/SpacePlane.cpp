#include "SpacePlane.h"
#include "Docker.h"
#include "Player.h"
#include "Viewer.h"
#include "EntityCommand.h"
#include "cmd.h"
#include "judge.h"
#include "astrodef.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "serial_util.h"
#include "material.h"
#include "motion.h"
#include "btadapt.h"
#include "glstack.h"
#include "draw/WarDraw.h"
#include "Island3.h"
#include "sqadapt.h"
#include "ShadowMap.h"
#include "glsl.h"
extern "C"{
#include "bitmap.h"
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufvbo.h>
#include <clib/GL/gldraw.h>
#include <clib/GL/cull.h>
#include <clib/GL/multitex.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
#include <clib/colseq/cs.h>
}
#include <assert.h>
#include <string.h>
#include <gl/glext.h>
#include <iostream>





#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.2, COLOR32RGBA(255,255,191,0)},
	{0.2, COLOR32RGBA(255,255,191,255)},
	{0.3, COLOR32RGBA(255,255,31,191)},
	{0.9, COLOR32RGBA(255,127,31,95)},
	{0.6, COLOR32RGBA(255,31,0,63)},
};
static const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 2.2);






SpacePlane::SpacePlane(WarField *aw) : st(aw), ai(NULL){
	init();
}

SpacePlane::SpacePlane(Entity *docksite) : st(docksite->w),ai(NULL){
	init();
	ai = new TransportAI(this, docksite);
}

void SpacePlane::init(){
	undocktime = 0.f;
	health = maxhealth();
	mass = 2e7;
	people = RandomSequence((unsigned long)this).next() % 100 + 100;
	engineHeat = 0.f;

	for(int i = 0; i < numof(pf); i++)
		pf[i] = NULL;
}

SpacePlane::~SpacePlane(){
	delete ai;
}

const char *SpacePlane::idname()const{
	return "SpacePlane";
}

const char *SpacePlane::classname()const{
	return "SpacePlane";
}

const unsigned SpacePlane::classid = registerClass("SpacePlane", Conster<SpacePlane>);
const unsigned SpacePlane::entityid = registerEntity("SpacePlane", new Constructor<SpacePlane>);

void SpacePlane::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << ai;
	sc.o << undocktime;
	sc.o << people;
}

void SpacePlane::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> ai;
	sc.i >> undocktime;
	sc.i >> people;

	// Re-create temporary entities if flying in a WarSpace.
	// If surrounding WarSpace is unserialized after this function, it would cause errors,
	// but while WarSpace::dive defines that the WarSpace itself go first, it will never happen.
	WarSpace *ws;
	if(w && (ws = (WarSpace*)w)){
		buildBody();
		ws->bdw->addRigidBody(bbody, 1, ~2);
		setPosition(&pos, &rot, &this->velo, &omg);
		for(int i = 0; i < numof(pf); i++)
			pf[i] = AddTefpolMovable3D(ws->tepl, this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICKEST | TEP3_ROUGH, cs_orangeburn.t);
	}
	else{
		for(int i = 0; i < numof(pf); i++)
			pf[i] = NULL;
	}
}

void SpacePlane::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	if(ai)
		ai->dive(sc, method);
}

const char *SpacePlane::dispname()const{
	return "SpacePlane";
}

void SpacePlane::anim(double dt){
	WarSpace *ws = *w;
	if(!ws){
		st::anim(dt);
		return;
	}

	if(bbody && !warping){
		const btTransform &tra = bbody->getCenterOfMassTransform();
		pos = btvc(tra.getOrigin());
		rot = btqc(tra.getRotation());
		velo = btvc(bbody->getLinearVelocity());
	}

	/* forget about beaten enemy */
	if(enemy && (enemy->health <= 0. || enemy->w != w))
		enemy = NULL;

	Mat4d mat;
	transform(mat);

	if(0 < health){
		Entity *collideignore = NULL;
		if(ai){
			if(ai->control(this, dt)){
				delete ai;
				ai = NULL;
			}
			if(!w)
				return;
		}
		else if(task == sship_undock){
			if(undocktime < 0.){
				inputs.press &= ~PL_W;
				task = sship_idle;
			}
			else{
				inputs.press |= PL_W;
				undocktime -= dt;
			}
		}
		else if(w->getPlayer()->control == this){
		}
		else if(!enemy && task == sship_parade){
			Entity *pm = mother ? mother->e : NULL;
			if(mother){
				if(paradec == -1)
					paradec = mother->enumParadeC(mother->Frigate);
				Vec3d target, target0(1.5, -1., -1.);
				Quatd q2, q1;
				target0[0] += paradec % 10 * .30;
				target0[2] += paradec / 10 * -.30;
				target = pm->rot.trans(target0);
				target += pm->pos;
				Vec3d dr = this->pos - target;
				if(dr.slen() < .10 * .10){
					q1 = pm->rot;
					inputs.press &= ~PL_W;
//					parking = 1;
					this->velo += dr * (-dt * .5);
					q2 = Quatd::slerp(this->rot, q1, 1. - exp(-dt));
					this->rot = q2;
				}
				else{
	//							p->throttle = dr.slen() / 5. + .01;
					steerArrival(dt, target, pm->velo, 1. / 10., .001);
				}
			}
			else
				task = sship_idle;
		}
		else if(task == sship_idle){
			if(race != 0 /*RandomSequence((unsigned long)this + (unsigned long)(w->war_time() / .0001)).nextd() < .0001*/){
				command(&DockCommand());
			}
			inputs.press = 0;
		}
		else{
			inputs.press = 0;
		}
	}
	else{
		this->w = NULL;
		return;
	}

	st::anim(dt);

	static const Vec3d engines[3] = {
		Vec3d(0, 0, 150) * sufscale,
		Vec3d(75, 0, 150) * sufscale,
		Vec3d(-75, 0, 150) * sufscale,
	};

	// inputs.press is filtered in st::anim, so we put tefpol updates after it.
	for(int i = 0; i < 3; i++) if(pf[i]){
		MoveTefpol3D(pf[i], mat.vp3(engines[i]), avec3_000, cs_orangeburn.t, !(inputs.press & PL_W));
	}

	engineHeat = approach(engineHeat, direction & PL_W ? 1.f : 0.f, dt, 0.);


#if 0
	if(p->pf){
		int i;
		avec3_t pos, pos0[numof(p->pf)] = {
			{.0, -.003, .045},
		};
		for(i = 0; i < numof(p->pf); i++){
			MAT4VP3(pos, mat, pos0[i]);
			MoveTefpol3D(p->pf[i], pos, avec3_000, cs_orangeburn.t, 0);
		}
	}
#endif
}

void SpacePlane::postframe(){
	st::postframe();
}

void SpacePlane::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	pos = this->rot.trans(Vec3d(0, 120, 50) * sufscale) + this->pos;
	rot = this->rot;
}

void SpacePlane::enterField(WarField *target){
	WarSpace *ws = *target;
	if(ws && ws->bdw){
		buildBody();
		ws->bdw->addRigidBody(bbody, 1, ~2);
	}
	if(ws){
		tent3d_fpol_list *tepl = w ? w->getTefpol3d() : NULL;
		for(int i = 0; i < 3; i++){
			if(this->pf[i])
				ImmobilizeTefpol3D(this->pf[i]);
			if(tepl)
				pf[i] = AddTefpolMovable3D(tepl, this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICKEST | TEP3_ROUGH, cs_orangeburn.t);
			else
				pf[i] = NULL;
		}
	}
}

bool SpacePlane::buildBody(){
	if(!bbody){
		static btCompoundShape *shape = NULL;
		if(!shape){
			shape = new btCompoundShape();
			Vec3d sc = Vec3d(50, 40, 150) * sufscale;
			const Quatd rot = quat_u;
			const Vec3d pos = Vec3d(0,0,0);
			btBoxShape *box = new btBoxShape(btvc(sc));
			btTransform trans = btTransform(btqc(rot), btvc(pos));
			shape->addChildShape(trans, box);
			shape->addChildShape(btTransform(btqc(rot), btVector3(0, 0, 100) * sufscale), new btBoxShape(btVector3(150, 10, 50) * sufscale));
		}

		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btvc(pos));

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
		bbody = new btRigidBody(rbInfo);
		return true;
	}
	return false;
}

/// if we are transitting WarField or being destroyed, trailing tefpols should be marked for deleting.
void SpacePlane::leaveField(WarField *w){
	for(int i = 0; i < 3; i++) if(this->pf[i]){
		ImmobilizeTefpol3D(this->pf[i]);
		this->pf[i] = NULL;
	}
	st::leaveField(w);
}


const double SpacePlane::sufscale = .0004;

void SpacePlane::draw(WarDraw *wd){
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;

	draw_healthbar(this, wd, health / maxhealth(), .1, 0, capacitor / frigate_mn.capacity);

	struct TextureParams{
		SpacePlane *p;
		WarDraw *wd;
		static void onBeginTextureWindows(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd)
				p->wd->setAdditive(true);
		}
		static void onEndTextureWindows(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd)
				p->wd->setAdditive(false);
		}
		static void onBeginTextureEngine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd){
				p->wd->setAdditive(true);
				const AdditiveShaderBind *asb = p->wd->getAdditiveShaderBind();
				if(asb)
					asb->setIntensity(p->p->engineHeat);
			}
		}
		static void onEndTextureEngine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd)
				p->wd->setAdditive(false);
		}
	} tp = {this, wd};

	static bool initialized = false;
	static suf_t *sufs[2] = {NULL};
	static VBO *vbo[2] = {NULL};
	static suftex_t *pst[2] = {NULL};
	static int engineAtrIndex = -1, windowsAtrIndex = -1;
	if(!initialized){
		static const char *names[] = {"models/spaceplane0.bin"};
		suftexparam_t stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		CallCacheBitmap5("spaceplane.bmp", "models/spaceplane_br.bmp", &stp, "models/spaceplane.bmp", NULL);
		CallCacheBitmap5("engine2.bmp", "models/engine2br.bmp", &stp, "models/engine2.bmp", NULL);
		for(int i = 0; i < numof(names); i++){
			sufs[i] = CallLoadSUF(names[i]);
			vbo[i] = CacheVBO(sufs[i]);
//			CacheSUFMaterials(sufs[i]);
			pst[i] = AllocSUFTex(sufs[i]);
		}
		for(int i = 0; i < pst[0]->n; i++) if(!strcmp(sufs[0]->a[i].colormap, "engine2.bmp")){
			engineAtrIndex = i;
			pst[0]->a[i].onBeginTexture = TextureParams::onBeginTextureEngine;
			pst[0]->a[i].onEndTexture = TextureParams::onEndTextureEngine;
		}
		else if(!strcmp(sufs[0]->a[i].colormap, "spaceplane.bmp")){
			windowsAtrIndex = i;
			pst[0]->a[i].onBeginTexture = TextureParams::onBeginTextureWindows;
			pst[0]->a[i].onEndTexture = TextureParams::onEndTextureWindows;
		}
		initialized = true;
	}
	if(pst[0]){
		if(0 <= engineAtrIndex){
			pst[0]->a[engineAtrIndex].onBeginTextureData = &tp;
			pst[0]->a[engineAtrIndex].onEndTextureData = &tp;
		}
		if(0 <= windowsAtrIndex){
			pst[0]->a[windowsAtrIndex].onBeginTextureData = &tp;
			pst[0]->a[windowsAtrIndex].onEndTextureData = &tp;
		}
	}

	static int drawcount = 0;
	drawcount++;
	{
		static const double normal[3] = {0., 1., 0.};
		double scale = sufscale;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

		class IntDraw{
			WarDraw *wd;
		public:
			IntDraw(WarDraw *wd) : wd(wd){
			}
			void drawModel(suf_t *suf, VBO *vbo, suftex_t *tex){
//				if(vbo)
//					DrawVBO(vbo, wd->shadowmapping ? SUF_TEX : SUF_ATR | SUF_TEX, tex);
//				else if(suf)
					DecalDrawSUF(suf, wd->shadowmapping ? SUF_TEX : SUF_ATR | SUF_TEX, NULL, tex, NULL, NULL);
			}
			void glTranslated(double x, double y, double z){
				::glTranslated(x, y, z);
			}
		} id(wd);

		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

#if 0
		for(int i = 0; i < nhitboxes; i++){
			Mat4d rot;
			glPushMatrix();
			gldTranslate3dv(hitboxes[i].org);
			rot = hitboxes[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox_draw(this, hitboxes[i].sc);
			glPopMatrix();
		}
#endif

		GLattrib gla(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

/*		if(wd->shadowMap){
			wd->shadowMap->setAdditive(true);
			wd->shadowMap->getAdditive()->setIntensity(engineHeat);
		}*/

		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		id.drawModel(sufs[0], vbo[0], pst[0]);
		glPopMatrix();

		glPopMatrix();

/*		if(wd->shadowMap)
			wd->shadowMap->setAdditive(false);*/

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void SpacePlane::drawtra(wardraw_t *wd){
	st::drawtra(wd);
}

void SpacePlane::drawOverlay(wardraw_t *){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.10,  .00);
	glVertex2d(-.05, -.03);
	glVertex2d( .00, -.03);
	glVertex2d( .08, -.07);
	glVertex2d( .10, -.03);
	glVertex2d( .10,  .03);
	glVertex2d( .08,  .07);
	glVertex2d( .00,  .03);
	glVertex2d(-.05,  .03);
	glEnd();
}


Entity::Props SpacePlane::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("People: ") << people);
	ret.push_back(gltestp::dstring("I am SpacePlane!"));
	return ret;
}

bool SpacePlane::command(EntityCommand *com){
	if(DockToCommand *dtc = InterpretCommand<DockToCommand>(com)){
		ai = new DockAI(this, dtc->deste);
		return true;
	}
	else if(InterpretCommand<DockCommand>(com)){
		ai = new DockAI(this);
		return true;
	}
	else return st::command(com);
}

bool SpacePlane::dock(Docker *d){
	d->e->command(&TransportPeopleCommand(people));
	return true;
}

bool SpacePlane::undock(Docker *d){
	task = sship_undock;
	mother = d;
	d->e->command(&TransportPeopleCommand(-people));
	return true;
}

double SpacePlane::maxhealth()const{return 15000.;}


IMPLEMENT_COMMAND(TransportPeopleCommand, "TransportPeople")

TransportPeopleCommand::TransportPeopleCommand(HSQUIRRELVM v, Entity &e){
	int argc = sq_gettop(v);
	if(argc < 3)
		throw SQFArgumentError();
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 3, &i)))
		throw SQFArgumentError();
	people = i;
}


