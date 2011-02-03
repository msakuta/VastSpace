#include "war.h"
#include "Universe.h"
#include "Entity.h"
#include "Player.h"
#include "CoordSys.h"
#include "astro.h"
#include "judge.h"
#include "serial_util.h"
#include "cmd.h"
#include "astrodraw.h"
#include "sqadapt.h"
#include "btadapt.h"
#include "draw/WarDraw.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/timemeas.h>
}
#include <gl/glext.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h>

int WarSpace::g_otdrawflags = 0;

WarField::WarField(){}

WarField::WarField(CoordSys *acs) : cs(acs), el(NULL), bl(NULL), pl(NULL), realtime(0){
	init_rseq(&rs, 2426);
}

const char *WarField::classname()const{
	return "WarField";
}

const unsigned WarField::classid = registerClass("WarField", Conster<WarField>);

#if 0 // reference
	Player *pl;
	Entity *el; /* entity list */
	Entity *bl; /* bullet list */
	struct tent3d_line_list *tell, *gibs;
	struct tent3d_fpol_list *tepl;
	struct random_sequence rs;
	int effects; /* trivial; telines or tefpols generated by this frame: to control performance */
	double realtime;
	double soundtime;
	otnt *ot, *otroot, *ottemp;
	int ots, oti;
	CoordSys *cs; /* redundant pointer to indicate belonging coordinate system */
#endif

void WarField::serialize(SerializeContext &sc){
	Serializable::serialize(sc);
	sc.o << pl << el << bl << rs << realtime;
}

void WarField::unserialize(UnserializeContext &sc){
	Serializable::unserialize(sc);
	sc.i >> pl >> el >> bl >> rs >> realtime;
}

void WarField::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext&)){
	(this->*method)(sc);
	if(el)
		el->dive(sc, method);
	if(bl)
		bl->dive(sc, method);
}

static Entity *WarField::*const list[2] = {&WarField::el, &WarField::bl};

void aaanim(double dt, WarField *w, Entity *WarField::*li){
	Player *pl = w->getPlayer();
	for(Entity *pe = w->*li; pe; pe = pe->next){
		try{
			pe->anim(dt);
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::anim(): %s\n", __LINE__, pe, pe->idname(), e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::anim(): ?\n", __LINE__, pe, pe->idname());
		}
		if(pl->cs == w->cs && !pl->chase && (pe->pos - pl->getpos()).slen() < .002 * .002)
			pl->chase = pe;
	}
}

#if 0
#define TRYBLOCK(a) {try{a;}catch(std::exception e){fprintf(stderr, __FILE__"(%d) Exception %s\n", __LINE__, e.what());}catch(...){fprintf(stderr, __FILE__"(%d) Exception ?\n", __LINE__);}}
#else
#define TRYBLOCK(a) (a);
#endif

void WarField::anim(double dt){
	CoordSys *root = cs;
	for(; root; root = root->parent){
		Universe *u = root->toUniverse();
		if(u){
			pl = u->ppl;
			break;
		}
	}
	aaanim(dt, this, list[0]);
	aaanim(dt, this, list[1]);
}

void WarField::postframe(){
	for(int i = 0; i < 2; i++)
	for(Entity *e = this->*list[i]; e; e = e->next)
		e->postframe();
}

void WarField::endframe(){
	for(int i = 0; i < 2; i++)
	for(Entity **pe = &(this->*list[i]); *pe;) if((*pe)->w != this){
		Entity *e = *pe;
		*pe = e->next;

		e->leaveField(this);

		// Player does not follow entities into WarField, which has no positional information.
		if(!e->w || !(WarSpace*)*e->w)
			pl->unlink(e);

		// Delete if actually NULL is assigned.
		if(!e->w){
			sqa_delete_Entity(e);
			delete e;
		}
		else
			e->w->addent(e);
	}
	else
		pe = &(*pe)->next;
}

void WarField::draw(wardraw_t *wd){}
void WarField::drawtra(wardraw_t *wd){}
void WarField::drawOverlay(wardraw_t *){}

bool WarField::pointhit(const Vec3d &pos, const Vec3d &velo, double dt, struct contact_info*)const{
	return false;
}

Vec3d WarField::accel(const Vec3d &srcpos, const Vec3d &srcvelo)const{
	return Vec3d(0,0,0);
}

double WarField::war_time()const{
	CoordSys *top = cs;
	for(; top->parent; top = top->parent);
	if(!top || !top->toUniverse())
		return 0.;
	return top->toUniverse()->global_time;
}

struct tent3d_line_list *WarField::getTeline3d(){return NULL;}
struct tent3d_fpol_list *WarField::getTefpol3d(){return NULL;}
WarField::operator WarSpace*(){return NULL;}
WarField::operator Docker*(){return NULL;}

Entity *WarField::addent(Entity *e){
	Entity **plist = e->isTargettable() ? &el : &bl;
	e->w = this;
	e->next = *plist;
	*plist = e;
	e->enterField(this); // This method is called after the object is brought into entity list.
	return e;
}

Player *WarField::getPlayer(){
	if(pl)
		return pl;
	CoordSys *root = this->cs->findcspath("/");
	if(root && root->toUniverse())
		return root->toUniverse()->ppl;
}





























class WarSpaceFilterCallback : public btOverlapFilterCallback{
	virtual bool needBroadphaseCollision(btBroadphaseProxy *proxy0, btBroadphaseProxy *proxy1)const{
		bool ret = btOverlapFilterCallback::needBroadphaseCollision(proxy0, proxy1);
		return ret;
	}
};


const char *WarSpace::classname()const{
	return "WarSpace";
}


const unsigned WarSpace::classid = registerClass("WarSpace", Conster<WarSpace>);

#if 0 // reference
	Player *pl;
	Entity *el; /* entity list */
	Entity *bl; /* bullet list */
	struct tent3d_line_list *tell, *gibs;
	struct tent3d_fpol_list *tepl;
	struct random_sequence rs;
	int effects; /* trivial; telines or tefpols generated by this frame: to control performance */
	double realtime;
	double soundtime;
	otnt *ot, *otroot, *ottemp;
	int ots, oti;
	CoordSys *cs; /* redundant pointer to indicate belonging coordinate system */
#endif

void WarSpace::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << pl << effects << soundtime << cs;
}

void WarSpace::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> pl >> effects >> soundtime >> cs;
}

void WarSpace::init(){
	tell = NewTeline3D(2048, 128, 128);
	gibs = NewTeline3D(1024, 128, 128);
	tepl = NewTefpol3D(128, 32, 32);
}

WarSpace::WarSpace() : ot(NULL), otroot(NULL), oti(0), ots(0){
	init();
}


static btDiscreteDynamicsWorld *bulletInit(){
	btDiscreteDynamicsWorld *btc = NULL;

	///collision configuration contains default setup for memory, collision setup
	btDefaultCollisionConfiguration *m_collisionConfiguration = new btDefaultCollisionConfiguration();
	//m_collisionConfiguration->setConvexConvexMultipointIterations();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	btCollisionDispatcher *m_dispatcher = new	btCollisionDispatcher(m_collisionConfiguration);

	btDbvtBroadphase *m_broadphase = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
	btSequentialImpulseConstraintSolver* m_solver = sol;

	btc = new btDiscreteDynamicsWorld(m_dispatcher,m_broadphase,m_solver,m_collisionConfiguration);

	btc->setGravity(btVector3(0,0,0));

	return btc;
}

WarSpace::WarSpace(CoordSys *acs) : st(acs), ot(NULL), otroot(NULL), oti(0), ots(0),
	effects(0), soundtime(0)
{
	init();
	for(CoordSys *root = cs; root; root = root->parent){
		Universe *u = root->toUniverse();
		if(u){
			pl = u->ppl;
			break;
		}
	}
	bdw = bulletInit();
}

void WarSpace::anim(double dt){
	CoordSys *root = cs;

	for(; root; root = root->parent){
		Universe *u = root->toUniverse();
		if(u){
			pl = u->ppl;
			break;
		}
	}
	aaanim(dt, this, list[0]);
//	fprintf(stderr, "otbuild %p %p %p %d\n", this->ot, this->otroot, this->ottemp);

	bdw->stepSimulation(dt / 1., 0);

	TRYBLOCK(ot_build(this, dt));
	aaanim(dt, this, list[1]);
	TRYBLOCK(ot_check(this, dt));
	TRYBLOCK(AnimTeline3D(tell, dt));
	TRYBLOCK(AnimTeline3D(gibs, dt));
	TRYBLOCK(AnimTefpol3D(tepl, dt));
}

void WarSpace::endframe(){
	st::endframe();
}

//static double gradius = 1.;
static int g_debugdraw_bullet = 0;

static void init_gsc(){
//	CvarAdd("gradius", &gradius, cvar_double);
	CvarAdd("g_debugdraw_bullet", &g_debugdraw_bullet, cvar_int);
}

static Initializator initializator(init_gsc);

#define TEXSIZE 128
static const GLenum cubetarget[] = {
GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
GL_TEXTURE_CUBE_MAP_POSITIVE_X,
GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
};
/* type for cube texture. not sure any OpenGL implementation that do not accept float textures exist. */
typedef GLubyte cubetype;
#define GL_cubetype GL_UNSIGNED_BYTE

void WarSpace::draw(wardraw_t *wd){
	for(int i = 0; i < 2; i++)
	for(Entity *pe = this->*list[i]; pe; pe = pe->next) if(pe->w == this/* && wd->vw->zslice == (pl->chase && pl->mover == &Player::freelook && pl->chase->getUltimateOwner() == pe->getUltimateOwner() ? 0 : 1)*/){
		try{
			pe->draw(wd);
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::draw(): %s\n", __LINE__, pe, pe->idname(), e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::draw(): ?\n", __LINE__, pe, pe->idname());
		}
	}


#if 0
	glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
	static GLuint tex = 0;
	if(!tex){
		glGenTextures(1, &tex);
		extern double perlin_noise_pixel(int x, int y, int bit);
		GLubyte texbits[TEXSIZE][TEXSIZE][3];
		for(int i = 0; i < TEXSIZE; i++) for(int j = 0; j < TEXSIZE; j++) for(int k = 0; k < 3; k++){
			texbits[i][j][k] = 128 * perlin_noise_pixel(i, j + TEXSIZE * k, 4) + 128;
//			texbits[i][j][k] = rand() % 256;
		}
//		glBindTexture(GL_TEXTURE_2D, tex);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXSIZE, TEXSIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, texbits);
		glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
		for(int n = 0; n < numof(cubetarget); n++)
			glTexImage2D(cubetarget[n], 0, GL_RGB, TEXSIZE, TEXSIZE, 0, GL_RGB, GL_cubetype, texbits);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
//	glBindTexture(GL_TEXTURE_2D, tex);
	glBindTexture(GL_TEXTURE_CUBE_MAP,tex);
	glEnable(GL_NORMALIZE);
	{
		const GLfloat mat_specular[] = {0., 0., 0., 1.};
		const GLfloat mat_shininess[] = { 50.0 };
		const GLfloat color[] = {1.f, 1.f, 1.f, 1.f}, amb[] = {.25f, .25f, .25f, 1.f};

		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, color);
		glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
	}
//	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);
	drawIcosaSphere(Vec3d(.1,0,-1), gradius, *wd->vw, Vec3d(1,.125,1), Quatd(0, 0, sin(war_time() / 10.), cos(war_time() / 10.)) * Quatd(0, sin(war_time()), 0, cos(war_time())));
	glBindTexture(GL_TEXTURE_2D, 0);
	glPopAttrib();
#endif

	tent3d_line_drawdata dd;
	dd.viewdir = -wd->vw->rot.vec3(2);
	dd.viewpoint = wd->vw->pos;
	dd.invrot = wd->vw->irot;
	dd.fov = wd->vw->fov;
	dd.pgc = wd->vw->gc;
	dd.rot = wd->vw->qrot;
	DrawTeline3D(gibs, &dd);
}

void WarSpace::drawtra(wardraw_t *wd){
	tent3d_line_drawdata dd;
	dd.viewdir = -wd->vw->rot.vec3(2);
	dd.viewpoint = wd->vw->pos;
	dd.invrot = wd->vw->irot;
	dd.fov = wd->vw->fov;
	dd.pgc = wd->vw->gc;
	dd.rot = wd->vw->qrot;
	DrawTeline3D(tell, &dd);
	DrawTefpol3D(tepl, wd->vw->pos, &static_cast<glcull>(*wd->vw->gc));

	for(int i = 0; i < 2; i++)
	for(Entity *pe = this->*list[i]; pe; pe = pe->next) if(pe->w == this/* && wd->vw->zslice == (pl->chase && pl->mover == &Player::freelook && pl->chase->getUltimateOwner() == pe->getUltimateOwner() ? 0 : 1)*/){
		try{
			pe->drawtra(wd);
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::drawtra(): %s\n", __LINE__, pe, pe->idname(), e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception in %p->%s::drawtra(): ?\n", __LINE__, pe, pe->idname());
		}
	}

	if(g_debugdraw_bullet && bdw){
		static class DebugDraw : public btIDebugDraw{
			// Override only drawLine method for the moment.
			virtual void drawLine(const btVector3& from,const btVector3& to,const btVector3& color){
				glColor3fv(btvc(color).cast<float>());
				glBegin(GL_LINES);
				glVertex3dv(btvc(from));
				glVertex3dv(btvc(to));
				glEnd();
			}
			virtual void	drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color){}
			virtual void	reportErrorWarning(const char* warningString){}
			virtual void	draw3dText(const btVector3& location,const char* textString){}
			virtual void	setDebugMode(int debugMode){}
			virtual int		getDebugMode() const{return 1;}
		}debugDrawer;
		bdw->getCollisionWorld()->setDebugDrawer(&debugDrawer);
		bdw->debugDrawWorld();
	}

	if(g_otdrawflags)
		ot_draw(this, wd);
}

void WarSpace::drawOverlay(wardraw_t *wd){
	Player *ppl = getPlayer();
	for(int i = 0; i < 2; i++)
	for(Entity *pe = this->*list[i]; pe; pe = pe->next) if(pe->w == this){
		double pixels;
		if(ppl && ppl->r_overlay && 0. < (pixels = wd->vw->gc->scale(pe->pos) * pe->hitradius()) && pixels * 20. < wd->vw->vp.m){
			Vec4d spos = wd->vw->trans.vp(Vec4d(pe->pos) + Vec4d(0,0,0,1));
			glPushMatrix();
			glLoadIdentity();
			glTranslated((spos[0] / spos[3] + 1.) * wd->vw->vp.w / 2., (1. - spos[1] / spos[3]) * wd->vw->vp.h / 2., 0.);
			glScaled(20, 20, 1);
			glColor4f(pe->race % 2, 1, (pe->race + 1) % 2, 1. - pixels * 20. / wd->vw->vp.m);
			try{
				pe->drawOverlay(wd);
			}
			catch(std::exception e){
				fprintf(stderr, __FILE__"(%d) Exception in %p->%s::drawOverlay(): %s\n", __LINE__, pe, pe->idname(), e.what());
			}
			catch(...){
				fprintf(stderr, __FILE__"(%d) Exception in %p->%s::drawOverlay(): ?\n", __LINE__, pe, pe->idname());
			}
			glPopMatrix();
		}
	}
}

struct tent3d_line_list *WarSpace::getTeline3d(){return tell;}
struct tent3d_fpol_list *WarSpace::getTefpol3d(){return tepl;}
WarSpace::operator WarSpace *(){return this;}
