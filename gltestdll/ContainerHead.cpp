#include "ContainerHead.h"
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




#define BEAMER_HEALTH 15000.
#define BEAMER_SCALE .0002


#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.2, COLOR32RGBA(255,255,191,0)},
	{0.2, COLOR32RGBA(255,255,191,255)},
	{0.3, COLOR32RGBA(255,255,31,191)},
	{0.9, COLOR32RGBA(255,127,31,95)},
	{0.6, COLOR32RGBA(255,31,0,63)},
};
static const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 2.2);



ContainerHead::ContainerHead(WarField *aw) : st(aw){
	init();
}

ContainerHead::ContainerHead(CoordSys *docksite) : st(docksite->parent->w), docksite(docksite){
	init();
	task = sship_undock;
	undocktime = 30.;
}

void ContainerHead::init(){
	RandomSequence rs((unsigned long)this);
	ncontainers = rs.next() % (maxcontainers - 1) + 1;
	for(int i = 0; i < ncontainers; i++)
		containers[i] = ContainerType(rs.next() % Num_ContainerType);
	undocktime = 0.f;
	health = maxhealth();
	mass = 2e7 + 1e7 * ncontainers;

	for(int i = 0; i < numof(pf); i++)
		pf[i] = NULL;

	if(!w)
		return;
}

const char *ContainerHead::idname()const{
	return "ContainerHead";
}

const char *ContainerHead::classname()const{
	return "ContainerHead";
}

const unsigned ContainerHead::classid = registerClass("ContainerHead", Conster<ContainerHead>);
const unsigned ContainerHead::entityid = registerEntity("ContainerHead", new Constructor<ContainerHead>);

void ContainerHead::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << undocktime;
}

void ContainerHead::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> undocktime;
}

const char *ContainerHead::dispname()const{
	return "Container Ship";
}

void ContainerHead::anim(double dt){
	WarSpace *ws = *w;
	if(!ws){
		st::anim(dt);
		return;
	}

	if(bbody){
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
		if(task == sship_undock){
			if(!docksite || docksite->parent->w != w || undocktime < 0.){
				inputs.press&= ~PL_W;
				task = sship_idle;
			}
			else{
				inputs.press |= PL_W;
				undocktime -= dt;
			}
		}
		else if(task == sship_dockque || task == sship_dock){
			if(docksite == NULL){
				std::vector<CoordSys *> set;
				findIsland3(w->cs->findcspath("/"), set);
				if(set.size()){
					int i = RandomSequence((unsigned long)this + (unsigned long)(w->war_time() / .0001)).next() % set.size();
					docksite = set[i];
				}
			}
			if(docksite && !this->warping){
				if(docksite->parent != w->cs){
					WarpCommand com;
					com.destcs = docksite->parent;
					com.destpos = docksite->pos + docksite->rot.trans(Vec3d(0, -16. - 3.25 - 1.5 - 1., 0.));
					command(&com);
					task = sship_dockque;
				}
				else{
					Vec3d target = docksite->rot.trans(task == sship_dockque ? Vec3d(0, -16. - 3.25 - 1.5, 0.) : Vec3d(0, -16. - 3.25, 0.)) + docksite->pos;
					steerArrival(dt, target, docksite->velo, 1. / 10., .001);
					if((target - pos).slen() < .2 * .2){
						if(task == sship_dockque)
							task = sship_dock;
						else{
							this->w = NULL;
							return;
						}
					}
				}
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

	const Vec3d engines[3] = {
		Vec3d(0, 80, 250 + 150 * ncontainers) * sufscale,
		Vec3d(75, -25, 250 + 150 * ncontainers) * sufscale,
		Vec3d(-75, -25, 250 + 150 * ncontainers) * sufscale,
	};

	// inputs.press is filtered in st::anim, so we put tefpol updates after it.
	for(int i = 0; i < 3; i++) if(pf[i]){
		MoveTefpol3D(pf[i], mat.vp3(engines[i]), avec3_000, cs_orangeburn.t, !(inputs.press & PL_W));
	}


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

void ContainerHead::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	pos = this->rot.trans(Vec3d(0, 120, 150 * ncontainers + 50) * sufscale) + this->pos;
	rot = this->rot;
}

void ContainerHead::enterField(WarField *target){
	WarSpace *ws = *target;
	if(ws && ws->bdw){
		static btCompoundShape *shapes[maxcontainers] = {NULL};
		btCompoundShape *&shape = shapes[ncontainers];
		if(!shape){
			shape = new btCompoundShape();
			Vec3d sc = Vec3d(100, 100, 250 + 150 * ncontainers) * sufscale;
			const Quatd rot = quat_u;
			const Vec3d pos = Vec3d(0,0,0);
			btBoxShape *box = new btBoxShape(btvc(sc));
			btTransform trans = btTransform(btqc(rot), btvc(pos));
			shape->addChildShape(trans, box);
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
//		rbInfo.m_linearDamping = .5;
//		rbInfo.m_angularDamping = .25;
		bbody = new btRigidBody(rbInfo);

		//add the body to the dynamics world
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

/// if we are transitting WarField or being destroyed, trailing tefpols should be marked for deleting.
void ContainerHead::leaveField(WarField *w){
	for(int i = 0; i < 3; i++) if(this->pf[i]){
		ImmobilizeTefpol3D(this->pf[i]);
		this->pf[i] = NULL;
	}
	st::leaveField(w);
}


const double ContainerHead::sufscale = BEAMER_SCALE;

void ContainerHead::draw(wardraw_t *wd){
	ContainerHead *const p = this;
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / maxhealth(), .1, 0, capacitor / frigate_mn.capacity);

	static bool initialized = false;
	static suf_t *sufs[2 + Num_ContainerType] = {NULL};
	static VBO *vbo[2 + Num_ContainerType] = {NULL};
	static suftex_t *pst[2 + Num_ContainerType] = {NULL};
	if(!initialized){

		// Register alpha test texture
		suftexparam_t stp;
		stp.flags = STP_ALPHA | STP_ALPHA_TEST | STP_TRANSPARENTCOLOR;
		stp.transparentColor = 0;
		AddMaterial("containerrail.bmp", "models/containerrail.bmp", &stp, NULL, NULL);

		static const char *names[2 + Num_ContainerType] = {"models/containerhead.bin", "models/containertail.bin", "models/gascontainer.bin", "models/hexcontainer0.bin"};
		for(int i = 0; i < 2 + Num_ContainerType; i++){
			sufs[i] = CallLoadSUF(names[i]);
			vbo[i] = CacheVBO(sufs[i]);
			CacheSUFMaterials(sufs[i]);
			pst[i] = AllocSUFTex(sufs[i]);
		}
		initialized = true;
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
				if(vbo)
					DrawVBO(vbo, wd->shadowmapping ? SUF_TEX : SUF_ATR | SUF_TEX, tex);
				else if(suf)
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
/*		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		if(wd->texShadow){
			glBindTexture(GL_TEXTURE_2D, wd->texShadow);
			glEnable(GL_TEXTURE_2D);
		}
		else{
			glDisable(GL_ALPHA_TEST);
		}*/
//		glEnable(GL_ALPHA_TEST);
//		glAlphaFunc(GL_GEQUAL, .5f);

		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		id.glTranslated(0, 0, 150 * ncontainers);
		id.drawModel(sufs[0], vbo[0], pst[0]);
		id.glTranslated(0, 0, -150);
		for(int i = 0; i < ncontainers; i++){
			id.drawModel(sufs[2 + containers[i]], vbo[2 + containers[i]], pst[2 + containers[i]]);
			id.glTranslated(0, 0, -300);
		}
		id.glTranslated(0, 0, 150);
		id.drawModel(sufs[1], vbo[1], pst[1]);
		glPopMatrix();

/*		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);*/

		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void ContainerHead::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	ContainerHead *p = this;
	Mat4d mat;

/*	if(p->dock && p->undocktime == 0)
		return;*/

	transform(mat);

//	drawCapitalBlast(wd, Vec3d(0,-0.003,.06), .01);

//	drawShield(wd);
}

Entity::Props ContainerHead::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("I am ContainerHead!"));
	return ret;
}

bool ContainerHead::command(EntityCommand *com){
	if(InterpretCommand<DockCommand>(com)){
		task = sship_dockque;
		docksite = NULL;
		return true;
	}
	else return st::command(com);
}

bool ContainerHead::undock(Docker *d){
	task = sship_undock;
	mother = d;
	return true;
}

void ContainerHead::post_warp(){
	task = sship_dockque;
}

double ContainerHead::maxhealth()const{return BEAMER_HEALTH;}

void ContainerHead::findIsland3(CoordSys *root, std::vector<CoordSys *> &ret)const{
	for(CoordSys *cs = root->children; cs; cs = cs->next){
		if(!strcmp(cs->classname(), "Island3") && cs)
			ret.push_back(cs);
		findIsland3(cs, ret);
	}
}
