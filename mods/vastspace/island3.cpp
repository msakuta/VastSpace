/** \file
 * \brief Implementation of Island3 class.
 */
#include "island3.h"
#include "vastspace.h"
#include "EntityRegister.h"
#include "ContainerHead.h"
#include "SpacePlane.h"
#include "astrodraw.h"
#include "CoordSys.h"
#include "Universe.h"
#include "Player.h"
#include "war.h"
#include "glsl.h"
#include "glstack.h"
#include "draw/material.h"
#include "cmd.h"
#include "btadapt.h"
#include "judge.h"
#include "bitmap.h"
#include "Game.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/ShadowMap.h"
#include "draw/ShaderBind.h"
#include "draw/mqoadapt.h"
#include "glw/PopupMenu.h"
#include "serial_util.h"
#include "msg/GetCoverPointsMessage.h"
#include "tent3d.h"
extern "C"{
#include <clib/c.h>
#include <clib/gl/multitex.h>
#include <clib/amat3.h>
#include <clib/suf/suf.h>
#include <clib/suf/sufbin.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/cull.h>
#include <clib/lzw/lzw.h>
#include <clib/timemeas.h>
}
#include <gl/glu.h>
#include <gl/glext.h>
#include <assert.h>
#include "BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h"

using namespace gltestp;

#define ISLAND3_RAD 3250. ///< outer radius
#define ISLAND3_INRAD 3200. ///< inner radius
#define ISLAND3_GRAD 3240. ///< inner glass radius
#define ISLAND3_HALFLEN 16000.
#define ISLAND3_MIRRORTHICK 100. ///< Thickness of the mirrors
#define ISLAND3_FARMRAD 20000.
#define MIRROR_LENGTH (ISLAND3_HALFLEN * 2.)
#define BRIDGE_HALFWID 10.
#define BRIDGE_THICK 10.


static int spacecolony_rotation(const struct coordsys *, aquat_t retq, const avec3_t pos, const avec3_t pyr, const aquat_t srcq);


class Island3WarSpace : public WarSpace{
public:
	typedef WarSpace st;
	Island3WarSpace(Game *game) : st(game), bbody(NULL){}
	Island3WarSpace(Island3 *cs);
	virtual Vec3d accel(const Vec3d &srcpos, const Vec3d &srcvelo)const;
	virtual Quatd orientation(const Vec3d &pos)const;
	virtual btRigidBody *worldBody();
	virtual bool sendMessage(Message &);

protected:
	btRigidBody *bbody;
};

class Island3Building : public Entity{
public:
	typedef Entity st;
	static const unsigned classid;
	static EntityRegister<Island3Building> entityRegister;
	Vec3d halfsize;
	friend Island3;
	Island3 *host;
	Island3Building(Game *game) : st(game){}
	Island3Building(WarField *w, Island3 *host);
	Island3Building(WarField *w) : st(w){}
	virtual const char *classname()const{return "Island3Building";}
	virtual double getMaxHealth()const{return 10000.;}
	virtual void enterField(WarField *);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual double getHitRadius()const{return halfsize.len();}
	virtual bool isTargettable()const{return true;}
	virtual bool isSelectable()const{return false;}
	virtual void draw(wardraw_t *);
};


//const unsigned Island3::classid = registerClass("Island3", Serializable::Conster<Island3>);
ClassRegister<Island3> Island3::classRegister("Island3");
const unsigned Island3Building::classid = registerClass("Island3Building", Conster<Island3Building>);
Entity::EntityRegister<Island3Building> Island3Building::entityRegister("Island3Building");
int &Island3::g_shader_enable = ::g_shader_enable;
GLuint Island3::walllist = 0;
GLuint Island3::walltex = 0;
Model *Island3::bridgetower = NULL;

Island3::Island3(Game *game) : st(game){
	init();
}

Island3::Island3(const char *path, CoordSys *root) : st(path, root){
	st::init(path, root);
	init();
	race = 0;
}

Island3::~Island3(){
	if(ent)
		delete ent;
}

void Island3::init(){
	rotation = 0.;
	sun_phase = 0.;
	ent = NULL;
	headToSun = false;
	setAbsMag(30.);
	rad = 100000.;
	orbit_home = NULL;
	mass = 1e10;
	basecolor = Vec4f(1., .5, .5, 1.);
	omg.clear();
	
	// All Island3's have a WarSpace by default.
	w = new Island3WarSpace(this);
	for(int i = 0; i < numof(bldgs); i++){
		bldgs[i] = new Island3Building(w, this);
		w->addent(bldgs[i]);
	}

	gases = 100;
	solids = 100;
	people = 100000;
}

void Island3::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << ent;
	sc.o << rotation;
	sc.o << gases;
	sc.o << solids;
	sc.o << people;
}

void Island3::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> ent;
	sc.i >> rotation;
	sc.i >> gases;
	sc.i >> solids;
	sc.i >> people;
}

void Island3::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
//	ent->dive(sc, method);
}


bool Island3::belongs(const Vec3d &lpos)const{
	double sdxy;
	sdxy = lpos[0] * lpos[0] + lpos[2] * lpos[2];
	return sdxy < ISLAND3_RAD * ISLAND3_RAD && -ISLAND3_HALFLEN - ISLAND3_INRAD < lpos[1] && lpos[1] < ISLAND3_HALFLEN;
}

static const Vec3d joint(3250. * cos(M_PI * 3. / 6. + M_PI / 6.), ISLAND3_HALFLEN, ISLAND3_RAD * sin(M_PI * 3. / 6. + M_PI / 6.));

void Island3::calcWingTrans(int i, Quatd &rot, Vec3d &pos){
	double brightness = (sin(sun_phase) + 1.) / 2.;
	Quatd rot0 = Quatd::rotation((i*2*M_PI/3.+M_PI/3.), 0, 1, 0);
	Quatd rot1 = Quatd::rotation(brightness * 2. * M_PI * 30. / 360., -1, 0, 0);
	rot = rot0 * rot1;
	pos = rot.trans(Vec3d(0, 0, ISLAND3_RAD + ISLAND3_MIRRORTHICK / 2.));
	Vec3d pos1 = rot0.trans(rot1.trans(-joint) + joint);
	pos += pos1;
}

Mat4d Island3::transform(const Viewer *vw)const{
	Mat4d ret;
	if(vw->zslice != 0){
		Quatd rot = vw->cs->tocsq(parent).cnj();
		ret = rot.tomat4();
		Vec3d delta = vw->cs->tocs(pos, parent) - vw->pos;
		ret = ret * this->rot.tomat4();
		ret.vec3(3) = delta;
	}
	else{
		ret = mat4_u;
		if(vw->cs == this || !ent || !ent->bbody){
			Mat4d rot = vw->cs->tocsim(this);
			ret.translatein(vw->cs->tocs(vec3_000, this));
			ret = ret * rot;
		}
		else{
			Mat4d rot = vw->cs->tocsim(parent);
			Mat4d btrot;
			ent->bbody->getWorldTransform().getOpenGLMatrix(btrot);
			ret.translatein(vw->cs->tocs(vec3_000, parent));
			ret = ret * rot * btrot;
		}
	}
	return ret;
}

void Island3::anim(double dt){
	Astrobj *sun = findBrightest();

	// Head toward sun
	if(sun){
		if(!headToSun){
			headToSun = true;
			rot = Quatd::direction(parent->tocs(pos, sun)).rotate(-M_PI / 2., avec3_100);
		}
		Vec3d sunpos = parent->tocs(pos, sun);
		CoordSys *top = findcspath("/");
//		double phase = omg[1] * (!top || !top->toUniverse() ? 0. : top->toUniverse()->global_time);
//		Quatd qrot = Quatd::direction(sunpos);
//		Quatd qrot1 = qrot.rotate(-M_PI / 2., avec3_100);
		double omg = sqrt(.0098 / 3.25);
		Vec3d vomg = Vec3d(0, omg, 0);
		this->rotation += omg * dt;
		this->omg = this->rot.trans(vomg) + sunpos.norm().vp(rot.trans(Vec3d(0,1,0))) * .1;
//		this->rot = Quatd::rotation(this->omg.len() * dt, this->omg.norm()) * this->rot;
//		this->rot = this->rot.quatrotquat(this->omg * dt);
//		this->rot = qrot1.rotate(phase, avec3_010);
//		this->omg.clear();
	}

	// Calculate phase of the simulated sun.
	sun_phase += 2 * M_PI / 24. / 60. / 60. * dt;

	if(!parent->w){
		if(game->isServer())
			parent->w = new WarSpace(parent);
		else // Client won't create any Entity.
			return;
	}
	WarSpace *ws = *parent->w;
	if(ws && !ent && game->isServer()){
		ent = new Island3Entity(ws, *this);
		ws->addent(ent);
	}
	if(ws && ent && game->isServer()){
		ent->pos = this->pos;
		ent->rot = this->rot;
		ent->velo = this->velo;
		ent->omg = this->omg;
		ent->mass = this->mass;

		RandomSequence rs((unsigned long)this + (unsigned long)(ws->war_time() / .0001));

		// Randomly create container heads
		// temporarily disabled until being accepted in the server-client model.
		if(true && floor(ws->war_time()) < floor(ws->war_time() + dt) && rs.nextd() < 0.02){
			Entity *ch = rs.next() % 2 ? (Entity*)(new ContainerHead(this->ent)) : new SpacePlane(this->ent);
			ch->race = race;
			ws->addent(ch);
			Vec3d rpos = this->rot.trans(Vec3d(0, -ISLAND3_HALFLEN - ISLAND3_RAD, 0.));
			ch->pos = rpos + this->pos + .1 * Vec3d(rs.nextGauss(), rs.nextGauss(), rs.nextGauss());
			ch->rot = this->rot.rotate(-M_PI / 2., Vec3d(1,0,0));
			ch->velo = this->velo + this->omg.vp(rpos);
			ch->omg = this->omg;
			ch->setPosition(&ch->pos, &ch->rot, &ch->velo, &ch->omg);
			ch->undock(this->ent->docker);
		}
	}
	if(ws && ws->bdw && ent) if(!game->isServer() && !ent->bbody){
		// Client won't create bbody, just assign identity transform to wingtrans.
		for(int n = 0; n < 3; n++){
			ent->wingtrans[n] = btTransform::getIdentity();
		}
	}
	else{
		if(!ent->bbody)
			ent->buildShape();
		if(ent->bbody){
			btRigidBody *bbody = ent->bbody;
			bbody->setWorldTransform(btTransform(btqc(rot), btvc(pos)));
			bbody->setAngularVelocity(btvc(omg));
			int i;
			for(int n = 0; n < 3; n++) for(i = 0; i < ent->btshape->getNumChildShapes(); i++) if(ent->wings[n] == ent->btshape->getChildShape(i)){
				Quatd rot;
				Vec3d pos;
				calcWingTrans(i, rot, pos);
				ent->wingtrans[i] = btTransform(btqc(rot), btvc(pos));
				ent->btshape->updateChildTransform(i, ent->wingtrans[i]);
			}
		}
	}
	// Super class's methods do not assume a client can run.
	if(game->isServer())
		st::anim(dt);
}

void Island3::clientUpdate(double dt){
	anim(dt);
}


struct randomness{
	GLubyte bias, range;
};

#define DETAILSIZE 64
static const gltestp::TexCacheBind *createdetail(unsigned long seed, int level, const struct randomness (*ran)[3]){
	// Explicitly name the texture procedural to avoid possible name collision.
	static const gltestp::dstring roadtexpath("proc/road.bmp");

	const gltestp::TexCacheBind *stc;
	stc = gltestp::FindTexture(roadtexpath);
	if(stc)
		return stc;

	int n;
	GLuint list;
	struct random_sequence rs;
	init_rseq(&rs, seed);
/*	list = base ? base : glGenLists(level);*/
	for(n = 0; n < level; n++){
		int i, j;
		int tsize = DETAILSIZE;
		static GLubyte tbuf[DETAILSIZE][DETAILSIZE][3], tex[DETAILSIZE][DETAILSIZE][3];

		/* the higher you go, the less the ground details are */
		for(i = 0; i < tsize; i++) for(j = 0; j < tsize; j++){
			int buf[3] = {0};
			int k;
			for(k = 0; k < n*n+1; k++){
				buf[0] += (*ran)[0].bias + rseq(&rs) % (*ran)[0].range;
				buf[1] += (*ran)[1].bias + rseq(&rs) % (*ran)[1].range;
				buf[2] += (*ran)[2].bias + rseq(&rs) % (*ran)[2].range;
			}
			tbuf[i][j][0] = (GLubyte)(buf[0] / (n*n+1));
			tbuf[i][j][1] = (GLubyte)(buf[1] / (n*n+1));
			tbuf[i][j][2] = (GLubyte)(buf[2] / (n*n+1));
		}

		/* average surrounding 8 texels to smooth */
		for(i = 0; i < tsize; i++) for(j = 0; j < tsize; j++){
			int k, l;
			int buf[3] = {0};
			for(k = -1; k <= 1; k++) for(l = -1; l <= 1; l++)/* if(k != 0 || l != 0)*/{
				int x = (i + k + DETAILSIZE) % DETAILSIZE, y = (j + l + DETAILSIZE) % DETAILSIZE;
				buf[0] += tbuf[x][y][0];
				buf[1] += tbuf[x][y][1];
				buf[2] += tbuf[x][y][2];
			}
			if(tsize / 4 / 3 <= i % (tsize / 4) && i % (tsize / 4) < tsize / 4 * 2 / 3 && (tsize / 3) * 4.5 / 10 <= j % (tsize / 3) && j % (tsize / 3) < (tsize / 3) * 5.5 / 10){
				tex[i][j][0] = MIN(255, buf[0] / 9 / 2 + 191);
				tex[i][j][1] = MIN(255, buf[1] / 9 / 2 + 191);
				tex[i][j][2] = MIN(255, buf[2] / 9 / 2 + 191);
			}
			else{
				tex[i][j][0] = buf[0] / 9 / 2 + 31;
				tex[i][j][1] = buf[1] / 9 / 2 + 31;
				tex[i][j][2] = buf[2] / 9 / 2 + 31;
			}
		}

		static struct BMIhack{
			BITMAPINFOHEADER bmiHeader;
			GLubyte buf[DETAILSIZE][DETAILSIZE][3];
		} bmi;
		suftexparam_t stp;
		bmi.bmiHeader.biSize = 0;
		bmi.bmiHeader.biSizeImage = sizeof bmi.buf;
		bmi.bmiHeader.biBitCount = 24;
		bmi.bmiHeader.biClrUsed = 0;
		bmi.bmiHeader.biClrImportant = 0;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biHeight = bmi.bmiHeader.biWidth = DETAILSIZE;
		memcpy(bmi.buf, tex, sizeof bmi.buf);
		stp.flags = STP_ENV | STP_MIPMAP;
		stp.bmi = (BITMAPINFO*)&bmi;
		stp.env = GL_MODULATE;
		stp.mipmap = 1;
		gltestp::CacheSUFMTex(roadtexpath, &gltestp::TextureKey(&stp), &gltestp::TextureKey(&stp));
	}
	return gltestp::FindTexture(roadtexpath);
}


static const gltestp::TexCacheBind *generate_road_texture(){
		struct randomness ran[3] = {{192, 64}, {192, 64}, {192, 64}};
#if 1
/*		list = glGenLists(1);*/
		return createdetail(1, 1, &ran);
#else
		list = glGenLists(MIPMAPS * 2);
		createdetail(1, MIPMAPS, list, &ran);
		ran[0].bias = ran[1].bias = 0;
		ran[1].range = 32;
		ran[2].bias = 192;
		ran[2].range = 64;
		ran[0].bias = ran[1].bias = ran[2].bias = 128;
		ran[0].range = ran[1].range = ran[2].range = 128;
		createdetail(1, MIPMAPS, list + MIPMAPS, &ran);
#endif
}

static GLuint cubetex = 0;
int g_reflesh_cubetex = 0;

#define CUBESIZE 512
#define SQRT2P2 (1.4142135623730950488016887242097/2.)

GLuint Island3MakeCubeMap(const Viewer *vw, const Astrobj *ignored, const Astrobj *sun){
	static const GLenum target[] = {
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	};
	static const Quatd dirs[] = {
		Quatd(SQRT2P2,0,SQRT2P2,0), /* {0,-SQRT2P2,0,SQRT2P2} */
		Quatd(SQRT2P2,0,0,SQRT2P2),
		Quatd(0,0,1,0), /* {0,0,0,1} */
		Quatd(-SQRT2P2,0,SQRT2P2,0), /*{0,SQRT2P2,0,SQRT2P2},*/
		Quatd(-SQRT2P2,0,0,SQRT2P2), /* ??? {0,-SQRT2P2,SQRT2P2,0},*/
		Quatd(-1,0,0,0), /* {0,1,0,0}, */
	};
	aquat_t omg0 = {0, 0, 1, 0};
	int i, g;
	Viewer v;
	struct viewport resvp = vw->vp;
	if(!g_reflesh_cubetex && cubetex)
		return cubetex;
	g_reflesh_cubetex = 0;

	v = *vw;
	v.detail = 0;
	v.fov = 1.;
	v.cs = vw->cs->findcspath("/");
	v.pos = v.cs->tocs(vw->pos, vw->cs);
	v.velo.clear();
	v.qrot = quat_u;
	v.relative = 0;
	v.relirot = mat4_u;
	v.relrot = mat4_u;
	v.velolen = 0.;
	v.vp.h = v.vp.m = v.vp.w = CUBESIZE;
	v.dynamic_range = 1e-4;
	glViewport(0,0,v.vp.w, v.vp.h);

	if(!cubetex){
		glGenTextures(1, &cubetex);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
	GLcull g_glcull(v.fov, vec3_000, v.relirot, 1e-15, 1e+15);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, 0);
	GLpmatrix pmat;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-g_glcull.getNear(), g_glcull.getNear(), -g_glcull.getNear(), g_glcull.getNear(), g_glcull.getNear(), g_glcull.getFar());
	glMatrixMode (GL_MODELVIEW);
	for (i = 0; i < 6; ++i) {
		// Omitting static keyword at the beginning of next line won't let us debug this function, due to probably too large stack frame.
		static GLubyte buf[CUBESIZE][CUBESIZE][3];
		int mi = CUBESIZE;
		glPushMatrix();
		v.irot = (v.qrot = dirs[i]).cnj().tomat4();
		v.rot = dirs[i].tomat4();
		v.relrot = v.rot;
		v.relirot = v.irot;
		Mat4d proj;
		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		v.trans = v.rot * proj;
		glLoadMatrixd(v.rot);
#if 0 && defined _DEBUG
		glClearColor(0,.5f,0,0);
#else
		glClearColor(0,0,0,0);
#endif
		glClear(GL_COLOR_BUFFER_BIT);
		drawstarback(&v, v.cs, NULL, sun);

		glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
		glTexImage2D(target[i], 0, GL_RGB, mi, mi, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		g = glGetError();
		// Temporalily disabled because it causes runtime error in Radeon HD 6870.
//		glCopyTexSubImage2D(target[i], 0, 0, 0, 0, 0, CUBESIZE, CUBESIZE);
		if(!g)
			g = glGetError();
		if(g){
			CmdPrint((const char*)gluErrorString(g));
			glReadPixels(0, 0, CUBESIZE, CUBESIZE, GL_RGB, GL_UNSIGNED_BYTE, buf);
			glTexImage2D(target[i], 0, GL_RGB, mi, mi, 0,
				GL_RGB, GL_UNSIGNED_BYTE, buf);
		}
		glPopMatrix();
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	glPopAttrib();
	glViewport(0,0,vw->vp.w, vw->vp.h);
	return cubetex;
}

void Island3::predraw(const Viewer *vw){
	static bool cubeMapped = false;
	if(!cubeMapped){
		cubeMapped = true;
		Island3MakeCubeMap(vw, this, findBrightest());
	}
	st::predraw(vw);
}


static GLint invEyeMat3Loc, textureLoc, cubeEnvLoc, fracLoc, lightLoc;

GLuint Reflist(){
	static GLuint reflist = 0;
	if(reflist)
		return reflist + !!Island3::g_shader_enable * 2;
	static const GLenum target[] = {
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	};
	static double genfunc[][4] = {
	{ 1.0, 0.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0, 0.0 },
	{ 0.0, 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 0.0, 1.0 },
	};
	static GLuint texnames[6];
	reflist = glGenLists(4);

	// Allocate list to enable mirror material when shader is enabled.
	glNewList(reflist + 2, GL_COMPILE);
	do{
		GLuint shaders[2], &vtx = shaders[0], &frg = shaders[1];
		GLuint shader;
		vtx = glCreateShader(GL_VERTEX_SHADER);
		frg = glCreateShader(GL_FRAGMENT_SHADER);
		if(!glsl_load_shader(vtx, "shaders/mirror.vs") || !glsl_load_shader(frg, "shaders/mirror.fs"))
			break;
		shader = glsl_register_program(shaders, 2);

		textureLoc = glGetUniformLocation(shader, "texture");
		cubeEnvLoc = glGetUniformLocation(shader, "envmap");
		invEyeMat3Loc = glGetUniformLocation(shader, "invEyeRot3x3");

		glUseProgram(shader);
		glUniform1i(textureLoc, 1);
		glUniform1i(cubeEnvLoc, 0);
	}while(0);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glEndList();

	// Disable mirror
	glNewList(reflist + 3, GL_COMPILE);
	glUseProgram(0);
	glEndList();

	// Allocate list to enable mirror material when shader is disabled.
	glNewList(reflist, GL_COMPILE);
	glDisable(GL_TEXTURE_2D);
	glPushAttrib(GL_TEXTURE_BIT);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEndList();

	// Disable mirror
	glNewList(reflist + 1, GL_COMPILE);
	glPopAttrib();
	glEndList();
	return reflist + !!Island3::g_shader_enable * 2;
}

static GLint bumpInvEyeMat3Loc, bumpTextureLoc, /*bumpCubeEnvLoc,*/ bumpFracLoc, bumpLightLoc, bumpAmbientLoc, bumpNormMapLoc;

static GLuint BumpList(){
	static GLuint bumplist = 0;
	if(bumplist)
		return bumplist + !!Island3::g_shader_enable * 2;
/*	static const GLenum target[] = {
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	};
	static double genfunc[][4] = {
	{ 1.0, 0.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0, 0.0 },
	{ 0.0, 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 0.0, 1.0 },
	};
	static GLuint texnames[6];*/
	bumplist = glGenLists(4);

	// Allocate list to enable mirror material when shader is enabled.
	glNewList(bumplist + 2, GL_COMPILE);
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
	do{
		GLuint shaders[2], &vtx = shaders[0], &frg = shaders[1];
		GLuint shader;
		vtx = glCreateShader(GL_VERTEX_SHADER);
		frg = glCreateShader(GL_FRAGMENT_SHADER);
		if(!glsl_load_shader(vtx, "shaders/bump.vs") || !glsl_load_shader(frg, "shaders/bump.fs"))
			break;
		shader = glsl_register_program(shaders, 2);

		bumpTextureLoc = glGetUniformLocation(shader, "texture");
//		bumpCubeEnvLoc = glGetUniformLocation(shader, "envmap");
		bumpInvEyeMat3Loc = glGetUniformLocation(shader, "invEyeRot3x3");
		bumpAmbientLoc = glGetUniformLocation(shader, "ambient");
		bumpNormMapLoc = glGetUniformLocation(shader, "nrmmap");

		glUseProgram(shader);
		glUniform1i(bumpTextureLoc, 0);
//		glUniform1i(bumpCubeEnvLoc, 0);
		glUniform1f(bumpAmbientLoc, .1f);
	}while(0);
//	glDisable(GL_TEXTURE_2D);
//	glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
//	glEnable(GL_TEXTURE_CUBE_MAP);
//	glDisable(GL_TEXTURE_GEN_S);
//	glDisable(GL_TEXTURE_GEN_T);
//	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEndList();

	// Disable mirror
	glNewList(bumplist + 3, GL_COMPILE);
	glUseProgram(0);
	glPopAttrib();
	glEndList();

	// Allocate list to enable mirror material when shader is disabled.
	glNewList(bumplist, GL_COMPILE);
	glEnable(GL_TEXTURE_2D);
	glEndList();

	// Disable mirror
	glNewList(bumplist + 1, GL_COMPILE);
	glDisable(GL_TEXTURE_2D);
	glEndList();
	return bumplist + !!Island3::g_shader_enable * 2;
}

static GLuint wallbumptex = 0, nrmmap = 0;

GLuint Island3::compileWallTexture(){
	if(!gltestp::FindTexture("bricks.bmp") || !gltestp::FindTexture("bricks_bump.bmp")){
		suftexparam_t stp;
		stp.flags = STP_WRAP_S | STP_WRAP_T | STP_MAGFIL | STP_MINFIL | STP_NORMALMAP;
		stp.wraps = GL_REPEAT;
		stp.wrapt = GL_REPEAT;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.normalfactor = 2.;
		walllist = CallCacheBitmap("bricks.bmp", "models/bricks.bmp", NULL, NULL);
		if(const TexCacheBind *stc = FindTexture("bricks.bmp"))
			walltex = stc->getTex(0);
		CallCacheBitmap5("bricks_bump.bmp", "textures/bricks.bmp", NULL, "textures/bricks_hgt.bmp", &stp);
		if(const TexCacheBind *stc = FindTexture("bricks_bump.bmp")){
			wallbumptex = stc->getTex(0);
			nrmmap = stc->getTex(1);
		}
	}
	return walllist;
}

void Island3::beginWallTexture(const Viewer *vw){
	GLfloat dif[4] = {.75f, .75f, .75f, 1.}, amb[4] = {.2f, .2f, .2f, 1.};
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	glFrontFace(GL_CCW);
	glCallList(BumpList());
	Astrobj *sun = findBrightest(vec3_000);
	if(g_shader_enable && sun){
		Quatd rot = quat_u/*vw->cs->tocsq(sun)*/;
		Mat4d rot2 = (rot * vw->qrot/*.cnj()*/).tomat4();
		Mat3<float> irot3 = rot2.tomat3().cast<float>();
		glUniformMatrix3fv(bumpInvEyeMat3Loc, 1, GL_FALSE, irot3);
		glUniform1i(bumpNormMapLoc, 1);
		glBindTexture(GL_TEXTURE_2D, wallbumptex);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, nrmmap);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glUniform1f(bumpAmbientLoc, .2f);
	}
	else
		glCallList(compileWallTexture());
}

void Island3::endWallTexture(){
	glCallList(BumpList() + 1);
}

static const avec3_t pos0[] = {
	{0., 17., 0.},
	{3.25, 16., 0.},
	{3.25, 12., 0.},
	{3.25, 8., 0.},
	{3.25, 4., 0.},
	{3.25, .0, 0.},
	{3.25, -4., 0.},
	{3.25, -8., 0.},
	{3.25, -12., 0.},
	{3.25, -16., 0.},
	{0., -17., 0.},
};

int Island3::getCutnum(const Viewer *vw)const{
	int cutnum = 48 * 4;
	double pixels = fabs(vw->gc->scale(pos)) * ISLAND3_HALFLEN;

	if(pixels < 1)
		return cutnum;
	else if(pixels < 50)
		cutnum /= 16;
	else if(pixels < 125)
		cutnum /= 8;
	else if(pixels < 250)
		cutnum /= 4;
	else if(pixels < 500)
		cutnum /= 2;
	return cutnum;
}

bool Island3::cullQuad(const Vec3d (&pos)[4], const GLcull *gc2, const Mat4d &mat){
	return cullLevel == 0;
/*	int k;
	for(k = 0; k < 4; k++){
		Vec3d viewpos = mat.vp3(pos[k]);
		if(gc2->getFar() < -viewpos[2])
			break;
	}
	return 4 == k;*/
}


void Island3::draw(const Viewer *vw){
	if(1 < vw->zslice) // No way we can draw it without z buffering.
		return;
	if(vw->shadowmap && vw->shadowmap->isDrawingShadow())
		return;
	struct ShaderReserver{
		const Viewer *vw;
		ShaderReserver(const Viewer *vw) : vw(vw){}
		~ShaderReserver(){
			if(vw->shadowmap){
				const ShaderBind *shaderBind = vw->shadowmap->getShader();
				if(shaderBind)
					shaderBind->use();
			}
		}
	} sr(vw);

	bool farmap = !!vw->zslice;
	GLcull *gc2 = vw->gclist[0];

	if(vw->gc->cullFrustum(vw->cs->tocs(pos, parent), 50000.))
		return;

	if(vw->shadowmap)
		vw->shadowmap->disableShadows();

	// If any part of the colony has chance to go beyond far clipping plane of z slice of 0,
	// it's enough to decide cullLevel to 1.
	if(gc2->cullFar(vw->cs->tocs(pos, parent), -40000.))
		cullLevel = 1;
	else
		cullLevel = 0;

	static bool multitex = false;
	if(!multitex){
		multitex = true;
		MultiTextureInit();
	}

	Vec3d pos = vw->cs->tocs(vec3_000, this);

	static GLuint groundtex = 0, groundlist, noisetex;
	GLuint roadtex = 0, roadlist = 0;
	const gltestp::TexCacheBind *stc;
	stc = generate_road_texture();
	if(stc){
		roadtex = stc->getTex(0);
		roadlist = stc->getList();
	}

	static const gltestp::dstring glassname = "grass.jpg";
	stc = gltestp::FindTexture(glassname);
	if(!stc){
		suftexparam_t stp;
		stp.flags = STP_WRAP_S | STP_WRAP_T | STP_MAGFIL | STP_MINFIL;
		stp.wraps = GL_MIRRORED_REPEAT;
		stp.wrapt = GL_MIRRORED_REPEAT;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		groundlist = CallCacheBitmap("grass.jpg", "textures/grass.jpg", &stp, NULL);
		if(const gltestp::TexCacheBind *stc = gltestp::FindTexture(glassname))
			groundtex = stc->getTex(0);
	}

	static const gltestp::dstring noisename = "noise.jpg";
	stc = gltestp::FindTexture(noisename);
	if(!stc){
		suftexparam_t stp;
		stp.flags |= STP_NORMALMAP;
		stp.normalfactor = 1.;
		CallCacheBitmap("noise.jpg", "textures/noise.jpg", &stp, NULL);
		if(const gltestp::TexCacheBind *stc = gltestp::FindTexture(noisename))
			noisetex = stc->getTex(0);
	}

	/* Shader is togglable on running */
	GLuint reflist = Reflist();

	static const avec3_t norm0[][2] = {
		{{0., 1., 0.}, {0., 1., 0.}},
		{{1., 0., 0.}, {1., 0., 0.}},
		{{1., 0., 0.}, {1., 0., 0.}},
		{{1., 0., 0.}, {1., 0., 0.}},
		{{1., 0., 0.}, {1., 0., 0.}},
		{{1., 0., 0.}, {1., 0., 0.}},
		{{1., 0., 0.}, {1., 0., 0.}},
		{{1., 0., 0.}, {1., 0., 0.}},
		{{1., 0., 0.}, {1., 0., 0.}},
		{{0., -1., 0.}, {0., -1., 0.}},
	};
	const double sufrad = ISLAND3_RAD;
	int n, i;
	int finecuts[48 * 4]; /* this buffer must be greater than cuts */

	double pixels = fabs(vw->gc->scale(pos)) * ISLAND3_HALFLEN;
	int cutnum = getCutnum(vw);

	double (*cuts)[2] = CircleCuts(cutnum);

	Vec3d vpos = tocs(vw->pos, vw->cs);

	GLmatrix glma;
	GLattrib glatt(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_TRANSFORM_BIT);

//	brightness = Island3Light();
	double brightness = (sin(sun_phase) + 1.) / 2.;
	{
		GLfloat dif[4] = {1., 1., 1., 1.}, amb[4] = {1., 1., 1., 1.};
		Astrobj *sun = findBrightest(vec3_000);
		if(sun){
			Vec4<float> lightpos;
			glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
			glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
			Vec3d sunpos = vw->cs->tocs(sun->pos, sun->parent);
			lightpos = sunpos.norm().cast<float>();
			lightpos[3] = 0.f;
			glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
		}
	}
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glColor4ub(255,255,255,255);
	{
		GLfloat spe[4] = {0., 0., 0., 1.};
		glMaterialfv(GL_FRONT, GL_SPECULAR, spe);
	}

	int defleap = 2, leap = defleap;

	/* pre-calculate LOD information to reduce checking */
	{
		Mat4d rot2 = mat4_u;
		Mat4d trans = vw->cs->tocsm(this);
		trans.vec3(3) = vw->cs->tocs(vec3_000, this);
		for(i = 0; i < cutnum; i += defleap){
			rot2[0] = cuts[i][1];
			rot2[2] = -cuts[i][0];
			rot2[8] = cuts[i][0];
			rot2[10] = cuts[i][1];
//			Vec3d pos = trans.vp3(rot2.vp3(pos0[1]));
			Vec3d pos = trans.vp3(rot2.vp3(Vec3d(ISLAND3_INRAD, 0, 0)));
			if((vw->pos[0] - pos[0]) * (vw->pos[0] - pos[0]) + (vw->pos[2] - pos[2]) * (vw->pos[2] - pos[2]) < 300. * 300.){
				finecuts[i] = 1;
			}
			else
				finecuts[i] = 0;
		}
	}

	Mat4d transmat = transform(vw);
	glMultMatrixd(transmat);
	Mat4d mat;
	glGetDoublev(GL_MODELVIEW_MATRIX, mat);
	glDisable(GL_BLEND);

	int northleap = -ISLAND3_HALFLEN - 3. * ISLAND3_RAD < vpos[1] && vpos[1] < -ISLAND3_HALFLEN + ISLAND3_RAD ? 1 : 2;

	Mat4d rot2 = mat4_u;

	/* hull draw, n == 0 when drawing inside, n == 1 when outside. */
	for(n = 0; n < 2; n++){
		const double hullrad = n ? ISLAND3_RAD : ISLAND3_INRAD;
		if(n){
			beginWallTexture(vw);
		}
		else{
			GLfloat dif[4] = {.3f, .3f, .3f, 1.}, amb[4] = {.7f, .7f, .7f, 1.};
			glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
			glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
			glFrontFace(GL_CW);
			glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT);
			glCallList(groundlist);
		}
		Mat4d rot = mat4_u;
		for(int i = 0; i < cutnum; i += leap){
			int i1, j;
			Mat4d *prot[4];

			/* leap adjust for dynamic LOD */
			if(i % defleap || finecuts[i])
				leap = defleap / 2;
			else
				leap = defleap;

			i1 = (i+leap) % cutnum;

			/* set rotation matrices cyclically */
			prot[0] = prot[3] = &rot2;
			prot[1] = prot[2] = &rot;
			glBegin(GL_QUADS);

			/* northan hemisphere. North means closer to the sun. */
			if(n == 0){
			}
			else for(j = 0; j < cutnum / 4 - 2 /*/ (i % (leap * 2) + 1)*/; j += defleap * northleap){
				int j1 = (j + defleap * northleap) % cutnum, k, i2 = /*i % (leap * 2) == 0 && !(j1 < cutnum / 8) ? (i+leap*2) % cutnum :*/ i1;
				Vec3d pos[4], pos1[2];
				rot2[0] = cuts[i2][1];
				rot2[2] = -cuts[i2][0];
				rot2[8] = cuts[i2][0];
				rot2[10] = cuts[i2][1];
				pos1[0].Vec3d::Vec3(hullrad * cuts[j][1],
					-ISLAND3_HALFLEN - hullrad * cuts[j][0],
					0.);
				pos1[1].Vec3d::Vec3(hullrad * cuts[j1][1],
					-ISLAND3_HALFLEN - hullrad * cuts[j1][0],
					0.);
				for(k = 0; k < 4; k++)
					pos[k] = prot[k]->dvp3(pos1[k/2]);
				if(!farmap ^ cullQuad(pos, gc2, mat))
					continue;
				pos1[0][1] += ISLAND3_HALFLEN;
				pos1[0] /= hullrad;
				pos1[1][1] += ISLAND3_HALFLEN;
				pos1[1] /= hullrad;
				for(k = 0; k < 4; k++){
					double st[2];
					Vec3d norm = prot[k]->dvp3(pos1[k / 2]);
					glNormal3dv(n ? norm : -norm);
					st[0] = pos1[k / 2][1];
					st[1] = (i % 16 + leap * !!(9 & (1<<k))) / 8.;
					glTexCoord2dv(st);
					if(glMultiTexCoord2fARB)
						glMultiTexCoord2fARB(GL_TEXTURE1_ARB, GLfloat(st[0] * 16.), GLfloat(st[1] * 16.));
					glVertex3dv(pos[k]);
				}
			}

			rot2[0] = cuts[i1][1];
			rot2[2] = -cuts[i1][0];
			rot2[8] = cuts[i1][0];
			rot2[10] = cuts[i1][1];

			for(j = 0; j < 8; j++) if(i % (cutnum / 3) < cutnum / 6){
				int k;
				Vec3d pos[4], pos001[2];
//				pos001[0] = pos0[j];
//				pos001[1] = pos0[j+1];
				pos001[0] = Vec3d(!n ? ISLAND3_INRAD : ISLAND3_RAD, ISLAND3_HALFLEN * 2. * -(j - 4) / 8, 0);
				pos001[1] = Vec3d(j == 8 ? 0. : !n ? ISLAND3_INRAD : ISLAND3_RAD, ISLAND3_HALFLEN * 2. * -(j - 4 + 1) / 8, 0);
#if 0
				if(0 < j && j < numof(pos0)-3){
					if(!n){
						pos001[0][0] = pos001[1][0] = ISLAND3_INRAD;
					}
					else{
						pos001[0][0] = pos001[1][0] = ISLAND3_RAD;
					}
				}
				else{
					if(j == 0)
						pos001[1][0] = hullrad/*ISLAND3_INRAD*/;
					else if(j == numof(pos0)-2)
						pos001[0][0] = ISLAND3_INRAD;
				}
#endif
				pos[0] = rot2.dvp3(pos001[0]);
				pos[1] = rot.dvp3(pos001[0]);
				pos[2] = rot.dvp3(pos001[1]);
				pos[3] = rot2.dvp3(pos001[1]);
				for(k = 0; k < 4; k++){
					Vec3d viewpos = mat.vp3(pos[k]);
					if(gc2->getFar() < -viewpos[2])
						break;
				}
				if(farmap ? 4 == k : 4 != k)
					continue;
				for(k = 0; k < 4; k++){
					double st[2];
					Vec3d norm = prot[k]->dvp3(norm0[j][k / 2]);
					if(n)
						glNormal3dv(norm);
					else{
						norm *= -1.;
						glNormal3dv(norm);
					}
					st[0] = pos0[j + k / 2][1];
					st[1] = (i % 16 + leap * !!(9 & (1<<k))) / 8.;
					glTexCoord2dv(st);
					if(glMultiTexCoord2fARB)
						glMultiTexCoord2fARB(GL_TEXTURE1_ARB, GLfloat(st[0] * 16.), GLfloat(st[1] * 16.));
					glVertex3dv(pos[k]);
				}
			}

			// Draw southan seal
			if(n == 1) do{
				Vec3d pos[4], pos001[2];
				pos001[0] = Vec3d(0., ISLAND3_HALFLEN + 1., 0);
				pos001[1] = Vec3d(ISLAND3_RAD, ISLAND3_HALFLEN, 0);
				pos[0] = rot2.dvp3(pos001[0]);
				pos[1] = rot.dvp3(pos001[0]);
				pos[2] = rot.dvp3(pos001[1]);
				pos[3] = rot2.dvp3(pos001[1]);
				int k;
				for(k = 0; k < 4; k++){
					Vec3d viewpos = mat.vp3(pos[k]);
					if(gc2->getFar() < -viewpos[2])
						break;
				}
				if(farmap ? 4 == k : 4 != k)
					continue;
				for(k = 0; k < 4; k++){
					double st[2];
					Vec3d norm = prot[k]->dvp3(norm0[j][k / 2]);
					glNormal3d(0, 1, 0);
					st[0] = !!(12 & (1 << k));
					st[1] = (i % 16 + leap * !!(9 & (1<<k))) / 8.;
					glTexCoord2dv(st);
					if(glMultiTexCoord2fARB)
						glMultiTexCoord2fARB(GL_TEXTURE1_ARB, GLfloat(st[0] * 16.), GLfloat(st[1] * 16.));
					glVertex3dv(pos[k]);
				}
			} while(0);

			glEnd();

			rot = rot2;
		}
		if(n)
			endWallTexture();
		else{
			glPopAttrib();

			static const Vec3d pos00[4][2] = {
				{Vec3d(ISLAND3_INRAD, ISLAND3_HALFLEN, 0), Vec3d(.0, ISLAND3_HALFLEN + 200., 0)},
				{Vec3d(ISLAND3_INRAD, -ISLAND3_HALFLEN, 0), Vec3d(200., -ISLAND3_HALFLEN, 0)},
				{Vec3d(200., -ISLAND3_HALFLEN, 0), Vec3d(200., -ISLAND3_HALFLEN - ISLAND3_RAD, 0)},
				{Vec3d(200., -ISLAND3_HALFLEN - ISLAND3_RAD, 0), Vec3d(300., -ISLAND3_HALFLEN - ISLAND3_RAD, 0)},
			};
			for(int h = 0; h < 2; h++){
				glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT);
				glCallList(compileWallTexture());
				glLightfv(GL_LIGHT0, GL_AMBIENT, Vec4f(.75f, .75f, .75f, 1.));

				Mat4d rot = mat4_u;

				for(int k = 0; k < 4; k++){
					for(int i = 0; i < cutnum; i += leap){
						leap = defleap;
						leap <<= k;

						int i1 = (i+leap) % cutnum;

						rot2[0] = cuts[i1][1];
						rot2[2] = -cuts[i1][0];
						rot2[8] = cuts[i1][0];
						rot2[10] = cuts[i1][1];

						int i3 = (i + (leap >> 1)) % cutnum;
						Mat4d rot3 = rot2;
						rot3[0] = cuts[i3][1];
						rot3[2] = -cuts[i3][0];
						rot3[8] = cuts[i3][0];
						rot3[10] = cuts[i3][1];

						/* set rotation matrices cyclically */
						Mat4d *prot[5] = {h ? &rot2 : &rot, &rot3, h ? &rot : &rot2, h ? &rot : &rot2, h ? &rot2 : &rot};

						Vec3d pos[5], pos001[2];
						pos001[1] = (pos00[h][0] * (4 - (k + 1)) + pos00[h][1] * (k + 1)) / 4;
						pos001[0] = (pos00[h][0] * (4 - k) + pos00[h][1] * k) / 4;
						pos[0] = prot[0]->dvp3(pos001[0]);
						pos[1] = prot[1]->dvp3(pos001[0]);
						pos[2] = prot[2]->dvp3(pos001[0]);
						pos[3] = prot[3]->dvp3(pos001[1]);
						pos[4] = prot[4]->dvp3(pos001[1]);
						int l = 0;
						for(l = 0; l < 5; l++){
							Vec3d viewpos = mat.vp3(pos[l]);
							if(gc2->getFar() < -viewpos[2])
								break;
						}
						if(farmap ? 5 == l : 5 != l)
							continue;

						static const double phase0[5] = {0., .5, 1., 1., 0.};
						glBegin(GL_POLYGON);
						for(l = 0; l < 5; l++){
							double st[2];
							Vec3d norm = Vec3d(1, 0, 0);
							norm *= -1.;
							glNormal3dv(norm);
							st[0] = !!(7 & (1 << l));
							st[1] = (i % 16 + leap * phase0[l]) / 8.;
							glTexCoord2dv(st);
							if(glMultiTexCoord2fARB)
								glMultiTexCoord2fARB(GL_TEXTURE1_ARB, GLfloat(st[0] * 16.), GLfloat(st[1] * 16.));
							glVertex3dv(pos[l]);
						}
						glEnd();

						rot = rot2;
					}
				}

				glLightfv(GL_LIGHT0, GL_AMBIENT, Vec4f(.5f, .5f, .5f, 1.));
				glBegin(GL_QUADS);
				for(int h = 0; h < 2; h++) for(int i = 0; i < cutnum; i += leap){
					leap = defleap;
					leap <<= 3;

					int i1 = (i+leap) % cutnum;

					rot2[0] = cuts[i1][1];
					rot2[2] = -cuts[i1][0];
					rot2[8] = cuts[i1][0];
					rot2[10] = cuts[i1][1];

					Vec3d pos[4], pos001[2];
					pos001[0] = pos00[2 + h][0];
					pos001[1] = pos00[2 + h][1];
					pos[0] = rot2.dvp3(pos001[0]);
					pos[1] = rot.dvp3(pos001[0]);
					pos[2] = rot.dvp3(pos001[1]);
					pos[3] = rot2.dvp3(pos001[1]);
					int l = 0;
					for(l = 0; l < 4; l++){
						Vec3d viewpos = mat.vp3(pos[l]);
						if(gc2->getFar() < -viewpos[2])
							break;
					}
					if(farmap ? 4 == l : 4 != l)
						continue;

					static const double phase0[4] = {1., 0., 0., 1.};
					for(l = 0; l < 4; l++){
						double st[2];
						Vec3d norm = Vec3d(-1, 0, 0);
						norm *= -1.;
						glNormal3dv(rot.dvp3(norm));
						st[0] = !!(12 & (1 << l));
						st[1] = (i % 16 + leap * phase0[l]) / 8.;
						glTexCoord2dv(st);
						if(glMultiTexCoord2fARB)
							glMultiTexCoord2fARB(GL_TEXTURE1_ARB, GLfloat(st[0] * 16.), GLfloat(st[1] * 16.));
						glVertex3dv(pos[l]);
					}

					rot = rot2;
				}
				glEnd();

				glPopAttrib();
			}
		}
	}

	// Docking bays
	if(0 && vw->zslice == 0){
		static bool init = false;
		static suf_t *sufs[3] = {NULL};
		static VBO *vbo[3] = {NULL};
		static suftex_t *pst[3] = {NULL};
		if(!init){
			static const char *names[1] = {"models/island3dock.bin"};
			for(int i = 0; i < 1; i++){
				sufs[i] = CallLoadSUF(names[i]);
				vbo[i] = CacheVBO(sufs[i]);
				CacheSUFMaterials(sufs[i]);
				pst[i] = gltestp::AllocSUFTex(sufs[i]);
			}
			init = true;
		}
		static const double normal[3] = {0., 1., 0.};
		double scale = .01;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,0,-1,0,
			0,-1,0,0,
			0,0,0,1,
		};

		class IntDraw{
		public:
			void drawModel(suf_t *suf, VBO *vbo, suftex_t *tex){
				if(vbo)
					DrawVBO(vbo, SUF_ATR /*& ~SUF_TEX/*/| SUF_TEX, tex);
				else if(suf)
					DecalDrawSUF(suf, SUF_ATR | SUF_TEX, NULL, tex, NULL, NULL);
			}
			void glTranslated(double x, double y, double z){
				::glTranslated(x, y, z);
			}
		} id;

		glPushMatrix();
//		glMultMatrixd(mat);

		GLattrib gla(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

		glPushMatrix();
		glTranslated(0, -ISLAND3_HALFLEN - ISLAND3_RAD, 0.);
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		id.drawModel(sufs[0], vbo[0], pst[0]);
		glPopMatrix();

/*		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);*/

		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Buildings will draw themselves in Entity drawing pass.
/*	if(vw->zslice == 0){
		wardraw_t wd;
		wd.lightdraws = 0;
		wd.vw = (Viewer*)vw;
		wd.w = NULL;
		for(i = 0; i < numof(bldgs); i++)
			bldgs[i]->draw(&wd);
	}*/

	/* seal at glass bondaries */
	if(100 < pixels && !farmap ^ cullLevel){
		int n;
		beginWallTexture(vw);
		Mat3d rot2 = mat3d_u();
		for(n = 0; n < 2; n++){
			int m;
			Vec3d bpos0[2], norm;
			glFrontFace(n ? GL_CCW : GL_CW);
			bpos0[0][0] = ISLAND3_INRAD;
			bpos0[1][0] = ISLAND3_RAD;
			bpos0[0][1] = bpos0[1][1] = n ? -ISLAND3_HALFLEN : ISLAND3_HALFLEN;
			bpos0[0][2] = bpos0[1][2] = 0.;
			norm = (1 - n * 2) * rot2.vec3(2);
			glNormal3dv(norm);
			for(m = 0; m < 3; m++){
				int i;
				glBegin(GL_QUAD_STRIP);
				for(i = 0; i <= cutnum / 6; i += leap){
					int i1 = (i + m * cutnum / 3 + cutnum / 6) % cutnum;
					rot2[0] = cuts[i1][1];
					rot2[2] = -cuts[i1][0];
					rot2[6] = cuts[i1][0];
					rot2[8] = cuts[i1][1];
					if(i1 % defleap || finecuts[i1])
						leap = defleap / 2;
					else
						leap = defleap;
					Vec3d bpos = rot2 * bpos0[0];
					glTexCoord2d(0., i);
					glVertex3dv(bpos);
					bpos = rot2 * bpos0[1];
					glTexCoord2d(1., i);
					glVertex3dv(bpos);
				}
				glEnd();
			}
		}
		endWallTexture();
	}

	/* bridges */
	if(100 < pixels){
		int n, j;
		int cc = 0;
		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT);
		glCallList(roadlist);
		glMaterialfv(GL_FRONT, GL_AMBIENT, GLfloat(brightness) * Vec4<GLfloat>(.85f, .85f, .85f, 1.));
		Mat4d rot2 = mat4_u;
		for(n = 0; n < 2; n++){
			if(n){
				glFrontFace(GL_CCW);
			}
			else{
				glFrontFace(GL_CW);
			}
			for(j = 1; j < numof(pos0)-1; j++){
				int i, m;
				static const avec3_t bbpos0[4] = {
					{ISLAND3_INRAD + BRIDGE_THICK, BRIDGE_HALFWID, 0.},
					{ISLAND3_INRAD + .001, BRIDGE_HALFWID, 0.},
					{ISLAND3_INRAD + BRIDGE_THICK, -BRIDGE_HALFWID, 0.},
					{ISLAND3_INRAD + .001, -BRIDGE_HALFWID, 0.},
				};
				Vec3d bpos0[2] = {
					Vec3d(n ? ISLAND3_INRAD + .001/*BRIDGE_THICK*/ : ISLAND3_INRAD,
						pos0[j][1] + BRIDGE_HALFWID, 0.),
					Vec3d(n ? ISLAND3_INRAD + .001/*BRIDGE_THICK*/ : ISLAND3_INRAD,
						pos0[j][1] - BRIDGE_HALFWID, 0.)
				};
				Vec3d spos0[2] = {
					Vec3d(ISLAND3_INRAD,
						pos0[j][1] + (n * 2 - 1) * BRIDGE_HALFWID,
						0.),
					Vec3d(ISLAND3_INRAD + 1./*BRIDGE_THICK*/,
						pos0[j][1] + (n * 2 - 1) * BRIDGE_HALFWID,
						0.)
				};
				for(m = 0; m < 3; m++){

					/* cull */
					{
						int i1 = m * cutnum / 3 + cutnum / 6 + cutnum / 12;
						rot2[13] = 0.;
						rot2[0] = cuts[i1][1];
						rot2[2] = -cuts[i1][0];
						rot2[8] = cuts[i1][0];
						rot2[10] = cuts[i1][1];
						Vec3d bpos = rot2.vp3(bpos0[0]);
						Vec3d bpos2 = vw->cs->tocs(bpos, this);
/*							{
							avec3_t vec, delta;
							VECSUB(delta, bpos, gc2->viewpoint);
							if(gc2->zfar - 1.5 < VECSP(gc2->viewdir, delta))
								ret = 1;
							else
								ret = 0;
						}*/
						if(/*!farmap ^ !gc2->cullFar(bpos2, -2.) ||*/ farmap || vw->gc->cullFrustum(bpos2, 3.))
							continue;
						cc++;
					}

/*						(!n ? glEnable : glDisable)(GL_TEXTURE_2D);*/
					glBindTexture(GL_TEXTURE_2D, !n ? roadtex : walltex);
					glBegin(GL_QUAD_STRIP);
					for(i = 0; i <= cutnum / 6; i += leap){
						int i1 = (i + m * cutnum / 3 + cutnum / 6) % cutnum;
						avec3_t bpos, norm;
						rot2[0] = cuts[i1][1];
						rot2[2] = -cuts[i1][0];
						rot2[8] = cuts[i1][0];
						rot2[10] = cuts[i1][1];
						if(i1 % defleap || finecuts[i1])
							leap = defleap / 2;
						else
							leap = defleap;
						if(n)
							VECCPY(norm, &rot2[8]);
						else
							VECSCALE(norm, &rot2[8], -1.);
						glNormal3dv(norm);
						mat4dvp3(bpos, rot2, bpos0[0]);
						glTexCoord2d(0., i);
						glVertex3dv(bpos);
						mat4dvp3(bpos, rot2, bpos0[1]);
						glTexCoord2d(1., i);
						glVertex3dv(bpos);
					}
					glEnd();

/*						glDisable(GL_TEXTURE_2D);*/
					glBindTexture(GL_TEXTURE_2D, walltex);
					glBegin(GL_QUAD_STRIP);
					for(i = 0; i <= cutnum / 6; i += leap){
						int i1 = (i + m * cutnum / 3 + cutnum / 6) % cutnum;
						avec3_t bpos, norm;
						rot2[0] = cuts[i1][1];
						rot2[2] = -cuts[i1][0];
						rot2[8] = cuts[i1][0];
						rot2[10] = cuts[i1][1];
						if(i1 % defleap || finecuts[i1])
							leap = defleap / 2;
						else
							leap = defleap;
						if(n)
							VECCPY(norm, &rot2[4]);
						else
							VECSCALE(norm, &rot2[4], -1.);
						glNormal3dv(norm);
						mat4dvp3(bpos, rot2, spos0[0]);
						glTexCoord2d(0., i);
						glVertex3dv(bpos);
						mat4dvp3(bpos, rot2, spos0[1]);
						glTexCoord2d(1., i);
						glVertex3dv(bpos);
					}
					glEnd();

					/* Bridge towers */
					if(n && vw->zslice == 0){
						struct gldCache gldcache;
						gldcache.valid = 0;
						glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
						glEnable(GL_NORMALIZE);
						if(!bridgetower)
							bridgetower = LoadMQOModel("models/bridgetower.mqo");
						if(bridgetower) for(i = 1; i < 4; i++){
							int i1 = (i * cutnum / (4 * 6) + m * cutnum / 3 + cutnum / 6) % cutnum;
							Vec3d bpos0(ISLAND3_GRAD, pos0[j][1] + .0, .0);
							rot2[0] = cuts[i1][1];
							rot2[2] = -cuts[i1][0];
							rot2[8] = cuts[i1][0];
							rot2[10] = cuts[i1][1];
							Vec3d bpos = rot2.dvp3(bpos0);
							Vec3d lpos = vw->cs->tocs(rot2.dvp3(bpos0), this);
							if(vw->gc->cullFar(lpos, .5) || vw->cs != this && vw->gc->scale(lpos) * .1 < 10.)
								continue;

							glPushMatrix();
							{
								Mat4d trans;
								trans.vec4(0) = rot2.vec4(1) * 1e-3;
								trans.vec4(1) = rot2.vec4(0) * -1e-3;
								trans.vec4(2) = rot2.vec4(2) * 1e-3;
								trans.vec4(3) = bpos;
								trans[15] = 1.;
								glMultMatrixd(trans);
							}
							DrawMQOPose(bridgetower, NULL);

							if(10. < .01 * fabs(gc2->scale(bpos))){
								glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
								glDisable(GL_TEXTURE_2D);
								glDisable(GL_LIGHTING);
								glDepthMask(GL_FALSE);
								glColor4ub(0,0,0,255);
								gltestp::dstring buf = gltestp::dstring() << j << "-" << 'A' + m << i;
								glTranslated(buf.len() * -.5 * 2., 187, 1.2);
								glScaled(2., 2., 1.);
								gldPutTextureString(buf);
								glRotated(180, 0, 1, 0);
								glTranslated(0, 0, 2.4);
								gldPutTextureString(buf);
								glPopAttrib();
							}

							glPopMatrix();
						}
						glPopAttrib();
					}
					{
						static GLuint texbb = 0;
						glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);

						const gltestp::TexCacheBind *stc;
						stc = gltestp::FindTexture("bbrail.bmp");
						if(!stc){
							suftexparam_t stp;
							stp.flags = STP_ALPHA | STP_TRANSPARENTCOLOR | STP_ENV | STP_MAGFIL;
							stp.alphamap = 1;
							stp.transparentColor = 0;
							stp.env = GL_MODULATE;
							stp.magfil = GL_NEAREST;
							texbb = CallCacheBitmap("bbrail.bmp", "models/bbrail.bmp", &stp, NULL);
						}
						glCallList(texbb);
/*							glDisable(GL_CULL_FACE);*/
						glEnable(GL_ALPHA_TEST);
/*							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);*/
						glBegin(GL_QUADS);
						rot2[13] = pos0[j][1];
						for(i = 0; i < cutnum / 6; i += leap){
							int k;
							int i1 = (i + m * cutnum / 3 + cutnum / 6) % cutnum;
							int i2 = (i + ((i1 % defleap || finecuts[i1]) ? (leap = defleap / 2) : (leap = defleap)) + m * cutnum / 3 + cutnum / 6) % cutnum;
							double di1, di2;
							di1 = i * ISLAND3_RAD / cutnum * M_PI / (BRIDGE_THICK - 1.);
							di1 -= floor(di1);
							di2 = di1 + leap * ISLAND3_RAD / cutnum * M_PI / (BRIDGE_THICK - 1.);
							for(k = 0; k < 2; k++){
								glNormal3d(0, 1 - n * 2, 0);
								rot2[0] = cuts[i1][1];
								rot2[2] = -cuts[i1][0];
								rot2[8] = cuts[i1][0];
								rot2[10] = cuts[i1][1];
								Vec3d bpos = rot2.vp3(bbpos0[k*2+0]);
								glTexCoord2d(di1, 0.);
								glVertex3dv(bpos);
								bpos = rot2.vp3(bbpos0[k*2+1]);
								glTexCoord2d(di1, 1.);
								glVertex3dv(bpos);
								rot2[0] = cuts[i2][1];
								rot2[2] = -cuts[i2][0];
								rot2[8] = cuts[i2][0];
								rot2[10] = cuts[i2][1];
								bpos = rot2.vp3(bbpos0[k*2+1]);
								glTexCoord2d(di2, 1.);
								glVertex3dv(bpos);
								bpos = rot2.vp3(bbpos0[k*2+0]);
								glTexCoord2d(di2, 0.);
								glVertex3dv(bpos);
							}
						}
						glEnd();
						glPopAttrib();
					}
				}
			}
		}
/*			printf("cc(%d) = %d\n", farmap, cc);*/
		glPopAttrib();
	}

	/* walls between inner/outer cylinder */
	if(100 < pixels && !farmap ^ cullLevel) for(int i = 0; i < cutnum; i += cutnum / 6){
		int i1 = i;
		Mat3d rot = mat3d_u();
		rot[0] = cuts[i1][1];
		rot[2] = -cuts[i1][0];
		rot[6] = cuts[i1][0];
		rot[8] = cuts[i1][1];
		glNormal3dv(i % (cutnum / 3) == 0 ? rot.vec3(2) : -rot.vec3(2));
		glBegin(GL_QUAD_STRIP);
		for(int j = 1; j < numof(pos0)-1; j++){
			Vec3d pos00 = pos0[j];
			pos00[0] = i % (cutnum / 3) == 0 ? ISLAND3_RAD : ISLAND3_INRAD;
			Vec3d pos = rot * pos00;
			glTexCoord3dv(pos00);
			glVertex3dv(pos);
			pos00[0] = i % (cutnum / 3) == 0 ? ISLAND3_INRAD : ISLAND3_RAD;
			pos = rot * pos00;
			glTexCoord3dv(pos00);
			glVertex3dv(pos);
		}
		glEnd();
	}
	leap = defleap;


	/* solar mirror wings */
	for(n = 0; n < 2; n++){
		if(n == 0){
			GLfloat dif[4] = {.010f, .01f, .02f, 1.}, amb[4] = {.005f, .005f, .01f, 1.}, black[4] = {0,0,0,1}, spc[4] = {.5, .5, .5, .55f};
			amb[1] = GLfloat((1. - brightness) * .005);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
			glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
			glMaterialfv(GL_FRONT, GL_EMISSION, black);
			glMaterialfv(GL_FRONT, GL_SPECULAR, spc);
			glPushAttrib(GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
			glCallList(reflist);
			glActiveTextureARB(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, noisetex);
			glActiveTextureARB(GL_TEXTURE0);
			Mat4d rot = vw->cs->tocsm(vw->cs->findcspath("/"));

			if(g_shader_enable){
				Mat4d rot2 = rot * vw->irot;
				Mat3<float> irot3 = rot2.tomat3().cast<float>();
				glUniformMatrix3fv(invEyeMat3Loc, 1, GL_FALSE, irot3);
//					glUniform1i(cubeEnvLoc, 0);
//					glUniform1f(fracLoc, g_shader_frac);
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);
			}
			else{
				glMatrixMode(GL_TEXTURE);
				glPushMatrix();
				glMultMatrixd(rot);
				glMultMatrixd(vw->irot);
				glMatrixMode(GL_MODELVIEW);
			}
			glFrontFace(GL_CW);
		}
		else{
/*			GLfloat dif[4] = {.75f, .75f, .75f, 1.}, amb[4] = {.2f, .2f, .2f, 1.}, spc[4] = {0., 0., 0., .15f};
			glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
			glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
			glMaterialfv(GL_FRONT, GL_SPECULAR, spc);*/
			beginWallTexture(vw);
			glFrontFace(GL_CCW);
		}
		for(int m = 0; m < 3; m++){
			glPushMatrix();
/*			glRotated(m * 120. + 60., 0., 1., 0.);
			gldTranslate3dv(joint);
			glRotated(brightness * 30., -1., 0., 0.);
			gldTranslaten3dv(joint);*/
			Mat4d localmat;
			if(ent)
				ent->wingtrans[m].getOpenGLMatrix(localmat);
			else{
				Vec3d pos;
				Quatd rot;
				calcWingTrans(m, rot, pos);
				localmat = rot.tomat4();
				localmat.vec3(3) = pos;
			}
			glMultMatrixd(localmat);
			glGetDoublev(GL_MODELVIEW_MATRIX, mat);
			glColor4ub(0,0,0,63);
			for(i = 0; i < 1; i += leap){
				int i1 = cutnum / 6;
				glBegin(GL_QUADS);
				glNormal3d(0., 0., n ? 1 : -1.);
				for(int j = 1; j < numof(pos0)-2; j++){
					Vec3d pos[4];
					int k;
					for(k = 0; k < 4; k++){
						pos[k].Vec3d::Vec3(
							((6 & (1<<k) ? i : i1) - cutnum / 12) * ISLAND3_RAD * 2 / (cutnum / 6) / sqrt(3.),
							pos0[j+k/2][1],
							/*ISLAND3_RAD +*/ (n * 2 - 1) / 2. * ISLAND3_MIRRORTHICK);
					}
					if(!farmap ^ cullQuad(pos, gc2, mat))
						continue;
					for(k = 0; k < 4; k++){
						glTexCoord2dv((n ? 1. : .25) * pos[k]);
						glVertex3dv(pos[k]);
					}
				}
				glEnd();
			}

			glPopMatrix();
		}
		if(!n){
			glCallList(reflist + 1);
			glPopAttrib();
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
		}
		else
			endWallTexture();
	}

	// Mirror thickness
	if(100 < pixels){
	beginWallTexture(vw);
	for(int m = 0; m < 3; m++){
		glPushMatrix();
/*		glRotated(m * 120. + 60., 0., 1., 0.);
		gldTranslate3dv(joint);
		glRotated(brightness * 30., -1., 0., 0.);
		gldTranslaten3dv(joint);*/
		Mat4d localmat;
		if(ent)
			ent->wingtrans[m].getOpenGLMatrix(localmat);
		else{
			Vec3d pos;
			Quatd rot;
			calcWingTrans(m, rot, pos);
			localmat = rot.tomat4();
			localmat.vec3(3) = pos;
		}
		glMultMatrixd(localmat);
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		glColor4ub(0,0,0,63);
		for(i = 0; i < 2; i++){
			int i1 = i * cutnum / 6 - cutnum / 12;
			glBegin(GL_QUADS);
			glNormal3d(i ? 1 : -1, 0., 0.);
			for(int j = 1; j < numof(pos0)-2; j++){
				Vec3d pos[4];
				int k;
				for(k = 0; k < 4; k++){
					pos[k].Vec3d::Vec3(i1 * ISLAND3_RAD * 2 / (cutnum / 6) / sqrt(3.),
						pos0[j+k/2][1],
						((i ^ !(6 & (1<<k))) * 2 - 1) / 2. * ISLAND3_MIRRORTHICK);
				}
				if(!farmap ^ cullQuad(pos, gc2, mat))
					continue;
				for(k = 0; k < 4; k++){
					glTexCoord2d(pos[k][2] - ISLAND3_RAD - ISLAND3_MIRRORTHICK, pos[k][1]);
					glVertex3dv(pos[k]);
				}
			}
			glEnd();
		}
		glBegin(GL_QUADS);
		Vec3d pos[4];
		pos[0].Vec3d::Vec3(-ISLAND3_RAD / sqrt(3.), pos0[1][1], -ISLAND3_MIRRORTHICK / 2.);
		pos[1].Vec3d::Vec3(-ISLAND3_RAD / sqrt(3.), pos0[1][1], ISLAND3_MIRRORTHICK / 2.);
		pos[2].Vec3d::Vec3(ISLAND3_RAD / sqrt(3.), pos0[1][1], ISLAND3_MIRRORTHICK / 2.);
		pos[3].Vec3d::Vec3(ISLAND3_RAD / sqrt(3.), pos0[1][1], -ISLAND3_MIRRORTHICK / 2.);
		if(farmap ^ cullQuad(pos, gc2, mat)) for(int k = 0; k < 4; k++){
			glTexCoord2d(pos[k][0], pos[k][2] - ISLAND3_RAD - ISLAND3_MIRRORTHICK);
			glVertex3dv(pos[k]);
		}
		pos[0].Vec3d::Vec3(ISLAND3_RAD / sqrt(3.), pos0[numof(pos0)-2][1], -ISLAND3_MIRRORTHICK / 2.);
		pos[1].Vec3d::Vec3(ISLAND3_RAD / sqrt(3.), pos0[numof(pos0)-2][1], ISLAND3_MIRRORTHICK / 2.);
		pos[2].Vec3d::Vec3(-ISLAND3_RAD / sqrt(3.), pos0[numof(pos0)-2][1], ISLAND3_MIRRORTHICK / 2.);
		pos[3].Vec3d::Vec3(-ISLAND3_RAD / sqrt(3.), pos0[numof(pos0)-2][1], -ISLAND3_MIRRORTHICK / 2.);
		if(farmap ^ cullQuad(pos, gc2, mat)) for(int k = 0; k < 4; k++){
			glTexCoord2d(pos[k][0], pos[k][2] - ISLAND3_RAD - ISLAND3_MIRRORTHICK);
			glVertex3dv(pos[k]);
		}
		glEnd();

		glPopMatrix();
	}
	endWallTexture();
	}

	/* farm ring */
	glEnable(GL_TEXTURE_2D);
	if(vw->zslice == 0){
		int i, c = 0;
		double rad_0 = ISLAND3_FARMRAD - 200., rad_1 = ISLAND3_FARMRAD, rad_2 = ISLAND3_FARMRAD + 200.; /* silly that "rad1" is used by windows. */

		double phase = rotation;
		const double suby = -(ISLAND3_HALFLEN + ISLAND3_RAD + 200.);

		Mat4d rotmat = Quatd::rotation(-phase, 0, 1, 0).tomat4();

		glPushMatrix();
		glRotated(-phase * deg_per_rad, 0., 1., 0.);
		Quatd qrot2 = rot.rotate(-phase, Vec3d(0,1,0));
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		for(i = 0; i < cutnum; i++){
			int i1 = (i+1) % cutnum;
			Vec3d org(rad_1 * cuts[i][0], suby, rad_1 * cuts[i][1]);
			Vec3d viewpos = rotmat.vp3(org);
//			viewpos[2] -= 2.;

			/* culling is done for 4 polygons at once. */
//			if(!(farmap ? gc2->getFar() < -viewpos[2] : -viewpos[2] <= gc2->getFar()))
//				continue;

//			viewpos = qrot2.trans(org) + pos;
			if(vw->gc->cullFrustum(vw->cs->tocs(viewpos, this), 2000.))
				continue;

			glBegin(GL_QUAD_STRIP);
			glNormal3d(-cuts[i][0], 0., -cuts[i][1]);
			glTexCoord2d(0., 0.);
			glVertex3d(rad_0 * cuts[i][0], suby, rad_0 * cuts[i][1]);
			glTexCoord2d(1., 0.);
			glVertex3d(rad_0 * cuts[i1][0], suby, rad_0 * cuts[i1][1]);
			glNormal3d(0., -1., 0.);
			glTexCoord2d(0., 1.);
			glVertex3d(rad_1 * cuts[i][0], suby - .2, rad_1 * cuts[i][1]);
			glTexCoord2d(1., 1.);
			glVertex3d(rad_1 * cuts[i1][0], suby - .2, rad_1 * cuts[i1][1]);
			glNormal3d(cuts[i][0], 0., cuts[i][1]);
			glTexCoord2d(0., 2.);
			glVertex3d(rad_2 * cuts[i][0], suby, rad_2 * cuts[i][1]);
			glTexCoord2d(1., 2.);
			glVertex3d(rad_2 * cuts[i1][0], suby, rad_2 * cuts[i1][1]);
			glNormal3d(0., 1., 0.);
			glTexCoord2d(0., 1.);
			glVertex3d(rad_1 * cuts[i][0], suby + .2, rad_1 * cuts[i][1]);
			glTexCoord2d(1., 1.);
			glVertex3d(rad_1 * cuts[i1][0], suby + .2, rad_1 * cuts[i1][1]);
			glNormal3d(-cuts[i][0], 0., -cuts[i][1]);
			glTexCoord2d(0., 0.);
			glVertex3d(rad_0 * cuts[i][0], suby, rad_0 * cuts[i][1]);
			glTexCoord2d(1., 0.);
			glVertex3d(rad_0 * cuts[i1][0], suby, rad_0 * cuts[i1][1]);
			glEnd();
			c++;
		}
		const double subfarmrad = 750. - ISLAND3_FARMRAD;
		const double rr = 200.; // Ring Radius
		for(i = 0; i < 3; i++){
			int j, leaps = 12;
			glPushMatrix();
			glRotated(i * 120., 0., 1., 0.);
			glTranslated(ISLAND3_FARMRAD, -(ISLAND3_HALFLEN + ISLAND3_RAD + 200.), 0.);
			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUAD_STRIP);
			glNormal3d(0., 1., 0.);
			glTexCoord2d(0., .2);
			glVertex3d(0., rr, 0.);
			glTexCoord2d(subfarmrad, .2);
			glVertex3d(subfarmrad, rr, 0.);
			glNormal3d(0., 0., 1.);
			glTexCoord2d(0., .0);
			glVertex3d(0., .0, rr);
			glTexCoord2d(subfarmrad, .0);
			glVertex3d(subfarmrad, .0, rr);
			glNormal3d(0., -1., 0.);
			glTexCoord2d(0., -.2);
			glVertex3d(0., -rr, 0.);
			glTexCoord2d(subfarmrad, -.2);
			glVertex3d(subfarmrad, -rr, 0.);
			glNormal3d(0., 0., -1.);
			glTexCoord2d(0., 0.);
			glVertex3d(0., .0, -rr);
			glTexCoord2d(subfarmrad, .0);
			glVertex3d(subfarmrad, .0, -rr);
			glNormal3d(0., 1., 0.);
			glTexCoord2d(0., .2);
			glVertex3d(0., rr, 0.);
			glTexCoord2d(subfarmrad, .2);
			glVertex3d(subfarmrad, rr, 0.);
			glEnd();
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_QUAD_STRIP);
			for(j = 0; j <= cutnum; j += leaps){
				int jj = (j) % (cutnum);
				glVertex3d(cuts[jj][0], 500., cuts[jj][1]);
				glVertex3d(cuts[jj][0], -500., cuts[jj][1]);
			}
			glEnd();
			glBegin(GL_POLYGON);
			glNormal3d(0., -1., 0.);
			for(j = 0; j < cutnum; j += leaps){
				glVertex3d(-cuts[j][0], -500., cuts[j][1]);
			}
			glEnd();
			glBegin(GL_POLYGON);
			glNormal3d(0., 1., 0.);
			for(j = 0; j < cutnum; j += leaps){
				glVertex3d(cuts[j][0], 500., cuts[j][1]);
			}
			glEnd();
			glPopMatrix();
		}
		glPopMatrix();
/*			printf("farmpoly: %d\n", c);*/
	}

	if(vw->shadowmap)
		vw->shadowmap->enableShadows();
}

#ifndef NDEBUG
static void hitbox_draw(const double sc[3], int hitflags){
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
/*	for(int ix = 0; ix < 2; ix++) for(int iy = 0; iy < 2; iy++) for(int iz = 0; iz < 2; iz++){
		glColor4fv(hitflags & (1 << (ix * 4 + iy * 2 + iz)) ? Vec4<float>(1,0,0,1) : Vec4<float>(0,1,1,1));
		glVertex3dv(vec3_000);
		glVertex3dv(1.2 * Vec3d(ix * 2 - 1, iy * 2 - 1, iz * 2 - 1));
	}*/
	glEnd();
	glPopAttrib();
	glPopMatrix();
}
#endif

void Island3::drawtra(const Viewer *vw){
	GLcull *gc = vw->gc, *gc2 = vw->gclist[0];
	double x0, srad;
	double pixels;

/*	for(csint = children; csint; csint = csint->next) if(csint->vft == &g_spacecolony_s){
		break;
	}
	if(!csint || !csint->w)
		return;*/
	Vec3d pos = vw->cs->tocs(avec3_000, this);
	if(gc->cullFrustum(pos, 50000.))
		return;
	pixels = fabs(gc->scale(pos)) * ISLAND3_RAD;
	if(pixels < 1)
		return;

	srad = vw->pos[0] * vw->pos[0] + vw->pos[2] * vw->pos[2];
	x0 = (srad < ISLAND3_INRAD * ISLAND3_INRAD ? ISLAND3_INRAD : ISLAND3_RAD) * sin(M_PI / 6.);
	if(vw->cs == this) for(int n = 0; n < 3; n++){
		amat4_t rot;
		avec3_t plpos;
		MAT4ROTY(rot, mat4identity, n * 2. * M_PI / 3.);
		MAT4DVP3(plpos, rot, vw->pos);
		if(-x0 < plpos[0] && plpos[0] < x0 && (plpos[2] < 0. || srad < ISLAND3_RAD * ISLAND3_RAD)){
			int i;
			double brightness, mirrorslope, rayslope;
			double as = .2, sas, cas;
			double (*cuts)[2];
			brightness = (sin(sun_phase) + 1.) / 2.;
			as = brightness * M_PI / 6.;
			sas = sin(as);
			cas = cos(as);
			mirrorslope = sas / cas;
			plpos[1] -= -cas * MIRROR_LENGTH + ISLAND3_HALFLEN;
			plpos[2] -= -sas * MIRROR_LENGTH - ISLAND3_RAD;
			if(plpos[2] < mirrorslope * plpos[1])
				continue;
			rayslope = tan(M_PI - 2. * (M_PI / 2. - as));
			if(plpos[1] * rayslope < plpos[2])
				continue;
			cuts = CircleCuts(10);

			glPushMatrix();
			glLoadMatrixd(vw->rot);
			glRotated(n * 120., 0., -1., 0.);
			glRotated(90. + 2. * brightness * 30., 1., 0., 0.);
			glBegin(GL_TRIANGLE_FAN);
			glColor4ub(255, 255, 255, (unsigned char)(brightness * 192.));
			glVertex3d(0., 0., 1.);
			glColor4ub(255, 255, 255, 0);
			for(i = 0; i <= 10; i++){
				int k = i % 10;
				glVertex3d(cuts[k][0] * sas, cuts[k][1] * sas, cas);
			}
			glEnd();
			glPopMatrix();
		}
	}

	// Glasses 
	bool farmap = !!vw->zslice;
	double brightness = (sin(sun_phase) + 1.) / 2.;
	int cutnum = getCutnum(vw);
	int defleap = 2, leap = defleap;
	double (*cuts)[2] = CircleCuts(cutnum);
	Mat4d rot2 = mat4_u;
	{
	GLattrib gla(GL_TEXTURE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LESS);
	glPushMatrix();
/*	Mat4d mat = vw->cs->tocsim(this);
	mat.vec3(3) = (vw->cs->tocs(vec3_000, this));
	glMultMatrixd(mat);*/
	glMultMatrixd(transform(vw));
	Mat4d mat;
	glGetDoublev(GL_MODELVIEW_MATRIX, mat);
/*		glDepthFunc(vpos[0] * vpos[0] + vpos[2] * vpos[2] < ISLAND3_RAD * ISLAND3_RAD ? GL_LEQUAL : GL_LESS);*/
	{
//		GLfloat dif[4] = {.35, .4, .45, .5}, spc[4] = {.1, .1, .1, 1.};
//		VECSCALEIN(dif, brightness * .75 + .25);
//		glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
//		glMaterialfv(GL_FRONT, GL_AMBIENT, dif);
/*			glMaterialfv(GL_FRONT, GL_SPECULAR, spc);*/
//			glFrontFace(GL_CCW);
		glCallList(walllist);
		glEnable(GL_CULL_FACE);
	}
	for(int n = 0; n < 2; n++){
		if(n){
			glFrontFace(GL_CCW);
			if(g_shader_enable){
				glActiveTextureARB(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, walltex);
				glActiveTextureARB(GL_TEXTURE0);
				glCallList(Reflist());
			}
//			glUniform1i(cubeEnvLoc, 1);
			glColor4f(.3f, .3f, .3f, .3f);
		}
		else{
			glFrontFace(GL_CW);
			glColor4f(.5, .5, .5, .3f);
		}
		for(int m = 0; m < 3; m++){
			int i0 = (cutnum * m / 3 + cutnum / 6) % cutnum;
/*				glPushMatrix();*/
/*				glRotated(m * 120. + 60., 0., 1., 0.);*/
			glGetDoublev(GL_MODELVIEW_MATRIX, mat);
			Mat4d rot = mat4_u;
			rot[0] = cuts[i0][1];
			rot[2] = -cuts[i0][0];
			rot[8] = cuts[i0][0];
			rot[10] = cuts[i0][1];
			for(int i = 0; i < cutnum / 6; i += leap){
				int i1, j;
				if(2 <= defleap){
					Vec3d pos = rot.vp3(pos0[1]);
					if(i % defleap == 0 && (vw->pos[0] - pos[0]) * (vw->pos[0] - pos[0]) + (vw->pos[2] - pos[2]) * (vw->pos[2] - pos[2]) < 300. * 300.){
						leap = defleap / 2;
					}
					else if(leap != defleap && (i) % defleap == 0)
						leap = defleap;
				}
				i1 = (i0+i+leap) % cutnum;
				rot2[0] = cuts[i1][1];
				rot2[2] = -cuts[i1][0];
				rot2[8] = cuts[i1][0];
				rot2[10] = cuts[i1][1];
/*					MAT4ROTY(rot2, rot, 2 * M_PI / cutnum);*/
				glBegin(GL_QUADS);
				for(j = 1; j < numof(pos0)-2; j++){
					int k;
					Vec3d pos[4];
					Vec3d pos00 = pos0[j], pos01 = pos0[j+1];
					if(0 < j && j < numof(pos0)-2){
						pos00[0] = !n ? ISLAND3_GRAD : ISLAND3_RAD;
						pos01[0] = !n ? ISLAND3_GRAD : ISLAND3_RAD;
					}
					pos[0] = rot2.vp3(pos00);
					pos[1] = rot.vp3(pos00);
					pos[2] = rot.vp3(pos01);
					pos[3] = rot2.vp3(pos01);
					for(k = 0; k < 4; k++){
						Vec3d viewpos = mat.vp3(pos[k]);
						if(gc2->getFar() < -viewpos[2])
							break;
					}
					if(farmap ? 4 == k : 4 != k)
						continue;
					for(k = 0; k < 4; k++){
						Vec3d norm;
						if(n){
							norm[0] = pos[k][0];
							norm[1] = 0.;
							norm[2] = pos[k][2];
						}
						else{
							norm[0] = -pos[k][0];
							norm[1] = 0.;
							norm[2] = -pos[k][2];
						}
						glNormal3dv(norm);
						glTexCoord2d(pos0[j + k / 2][1], (i % 8 + leap * !!(9 & (1<<k))) / 8.);
						glVertex3dv(pos[k]);
					}
				}
				glEnd();
				rot = rot2;
			}
/*				glPopMatrix();*/
		}
		if(n && g_shader_enable)
			glCallList(Reflist() + 1);

	}
	glPopMatrix();
	}

	// Docking nav lights
	Quatd vrot = rot.rotate(-rotation, /*rot.trans*/(Vec3d(0,1,0)));
	for(int n = 0; n < 2; n++){
		for(int i = 0; i < 16; i++){
			gldSpriteGlow(vrot.trans(Vec3d((n * 2 - 1) * -50., -ISLAND3_HALFLEN - ISLAND3_RAD - i * 100., 0)) + pos, 10., Vec4<GLubyte>(255,127,127, 255 * (1. - fmod(vw->viewtime + (16 - i) * .1, 1.))), vw->irot);
		}
	}


#if 0
	/* Bridge tower navlights */
	if(vw->zslice == 0){
		GLubyte col[4] = {255,0,0,255};
		amat4_t rot2;
		double t;
		double (*cuts)[2];
		int cutnum = 48 * 4;
		cuts = CircleCuts(cutnum);
		t = fmod(csint->w->war_time, 10.);
		col[3] = t < 5 ? t / 5. * 255 : (10. - t) / 5. * 255;
		glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		for(m = 0; m < 3; m++){
			int j, i, n;
			MAT4IDENTITY(rot2);
			for(j = 1; j < numof(pos0)-1; j++) for(i = 1; i < 4; i++){
				int i1 = (i * cutnum / (4 * 6) + m * cutnum / 3 + cutnum / 6) % cutnum;
				avec3_t bpos, bpos0;
				rot2[0] = cuts[i1][1];
				rot2[2] = -cuts[i1][0];
				rot2[8] = cuts[i1][0];
				rot2[10] = cuts[i1][1];
				for(n = 0; n < 2; n++){
					bpos0[0] = ISLAND3_GRAD - .201;
					bpos0[1] = pos0[j][1] + (BRIDGE_HALFWID + .001) * (n * 2 - 1);
					bpos0[2] = 0.;
					mat4dvp3(bpos, rot2, bpos0);
		/*			if(glcullFar(bpos, .5, gc2))
						continue;*/
					gldSpriteGlow(bpos, .002, col, vw->irot);
				}
			}
		}
		glPopAttrib();
	}
#endif
}







Entity::EntityRegisterNC<Island3Entity> Island3Entity::entityRegister("Island3Entity");

Model *Island3Entity::dockModel = NULL;

void Island3Entity::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << astro;
	sc.o << docker;
}

void Island3Entity::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> astro;
	sc.i >> docker;
}

void Island3Entity::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	docker->dive(sc, method);
}

void Island3Entity::enterField(WarField *w){
	WarSpace *ws = *w;
	if(ws && ws->bdw){
		buildShape();
		// Adding the body to the dynamics world will be done in st::enterField().
//		if(bbody)
//			ws->bdw->addRigidBody(bbody);
	}
	st::enterField(w);
}

void Island3Entity::drawOverlay(wardraw_t *){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.10,  .00);
	glVertex2d(-.09, -.02);
	glVertex2d( .09, -.02);
	glVertex2d( .10,  .00);
	glVertex2d( .09,  .02);
	glVertex2d(-.09,  .02);
	glEnd();
	glBegin(GL_LINES);
	glVertex2d( .09,  .02);
	glVertex2d(-.09,  .10);
	glVertex2d( .09, -.02);
	glVertex2d(-.09, -.10);
	glEnd();
}

Entity::Props Island3Entity::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("Gases: ") << astro->gases);
	ret.push_back(gltestp::dstring("Solids: ") << astro->solids);
	ret.push_back(gltestp::dstring("Population: ") << astro->people);
	return ret;		
}

void Island3Entity::buildShape(){
	WarSpace *ws = *w;
	if(ws){
		if(!btshape){
			btshape = new btCompoundShape();
			for(int i = 0; i < 3; i++){
				Vec3d sc = Vec3d(ISLAND3_RAD / sqrt(3.), ISLAND3_HALFLEN, ISLAND3_MIRRORTHICK / 2.);
				Quatd rot;
				Vec3d pos;
				astro->calcWingTrans(i, rot, pos);
				btBoxShape *box = new btBoxShape(btvc(sc));
				wings[i] = box;
				btTransform trans = btTransform(btqc(rot), btvc(pos));
				wingtrans[i] = trans;
				btshape->addChildShape(trans, box);
			}
			btCylinderShape *cyl = new btCylinderShape(btVector3(ISLAND3_RAD + 10., ISLAND3_HALFLEN, ISLAND3_RAD + 10.));
			btshape->addChildShape(btTransform(btqc(Quatd(0,0,0,1)), btvc(Vec3d(0,0,0))), cyl);

			// Adding surface model (mesh) itself as collision model, but it doesn't seem to work.
#if 0
			if(suf){
				static std::vector<Vec3i> tris;
				if(!tris.size()) for(int i = 0; i < suf->np; i++) for(int j = 2; j < suf->p[i]->p.n; j++){
					Vec3i intVec;
					for(int k = 0; k < 3; k++){
						int k1 = k == 0 ? 0 : j + k - 2;
						if(suf->p[i]->t == suf_t::suf_prim_t::suf_poly)
							intVec[k] = (suf->p[i]->p.v[k1][0]);
						else
							intVec[k] = (suf->p[i]->uv.v[k1].p);
					}
					tris.push_back(intVec);
				}
				btTriangleIndexVertexArray *mesh = new btTriangleIndexVertexArray(tris.size(), &tris[0][0], sizeof(tris[0]), suf->nv, suf->v[0], sizeof *suf->v);
				btBvhTriangleMeshShape *meshShape = new btBvhTriangleMeshShape(mesh, true);
				mesh->setScaling(btVector3(10., 10., 10.));
				btshape->addChildShape(btTransform(btqc(Quatd(sqrt(2.)/2.,0,0,sqrt(2.)/2.)), btVector3(0, -ISLAND3_HALFLEN - ISLAND3_RAD,0)), meshShape);
			}
#endif
		}
		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btvc(pos));

		mass = 1e10;

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			btshape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,btshape,localInertia);
//		rbInfo.m_linearDamping = .5;
//		rbInfo.m_angularDamping = .5;
		bbody = new btRigidBody(rbInfo);

//		bbody->setSleepingThresholds(.0001, .0001);

		// Adding the body to the dynamics world will be done in st::enterField().
//			ws->bdw->addRigidBody(bbody, 1, ~2);
//		ws->bdw->addRigidBody(bbody);
	}
}




#if 0
static void Island3DrawGrass(const Viewer *vw){
	const Mat4d &rotmat = vw->rot;
	const Vec3d &view = vw->pos;
	const Mat4d &inv_rot = vw->irot;
	int i, count = 1000, total = 0;
	struct random_sequence rs;
	timemeas_t tm;
	const double destvelo = .01; /* destination velocity */
	const double width = .00002 * 1000. / vw->vp.m; /* line width changes by the viewport size */
	double plpos[3];
	avec3_t ortpos;
	avec3_t nh0 = {0., 0., -1.}, nh;
	amat4_t mat;
	int level;
	int tris = 0, fan = 0;
	Universe *univ = findcspath("/")->toUniverse();
	double t = univ ? univ->astro_time : 0.;
/*	GLfloat rotaxisf[16] = {
		1., 0., 0., 0.,
		0., 0., -1., 0.,
		0., 1., 0., 0.,
		0., 0., 0., 1.,
	};
	GLfloat irotaxisf[16];*/
/*	if(!grassing)
		return;*/

	Mat3d ort3 = this->rotation(pos, Vec3d(0,0,0), rot).tomat4().tomat3();
//	cs->w->vft->orientation(cs->w, &ort3, &vw->pos);
	Mat3d iort3 = ort3.transpose();
	Mat4<float> rotaxisf = ort3.cast<float>();
	Mat4<float> irotaxisf = iort3.cast<float>();

	{
		double phase;
		phase = atan2(vw->pos[0], vw->pos[2]);
		MATVP(ortpos, iort3, vw->pos);
		ortpos[0] += phase * 3.2;
		ortpos[1] += ISLAND3_INRAD;
	}

/*	TimeMeasStart(&tm);*/
	glPushMatrix();
	glLoadMatrixd(rotmat);
	glMultMatrixf(rotaxisf);
	{
		amat4_t modelmat, persmat, imodelmat;
		glGetDoublev(GL_MODELVIEW_MATRIX, modelmat);
		glGetDoublev(GL_PROJECTION_MATRIX, persmat);
		persmat[14] = 0.;
		mat4mp(mat, persmat, modelmat);
		MAT4TRANSPOSE(imodelmat, modelmat);
		MAT4DVP3(nh, imodelmat, nh0);
	}
/*	MAT4VP3(nh, *inv_rot, nh0);*/
/*	glTranslated(-(*view)[0], -(*view)[1], -(*view)[2]);*/
	glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
/*	glDisable(GL_LIGHTING);*/
	glEnable(GL_NORMALIZE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
	for(level = 5; level; level--){
		double cellsize;
		double disp[3];
		if(0. + .05 * (1<<level) < ortpos[1])
			break;
		init_rseq(&rs, level + 23342);
		cellsize = .005 * (1<<level);
		count = level == 5 ? 50 : 100;
		for(i = count; i; i--){
			double pos[3], lpos[3], dst[3], x0, x1, z0, z1, base;
			double phase;
			double phase2;
			unsigned long red;

			pos[0] = ortpos[0] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[0] = /*pl.pos[0] +*/ +(floor(pos[0] / cellsize) * cellsize + cellsize / 2. - pos[0]);
			pos[1] = .0001 + (.00003 + drseq(&rs) * .00003) * (4. + (1<<level));
			pos[2] = ortpos[2] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[2] = /*pl.pos[2] +*/ +(floor(pos[2] / cellsize) * cellsize + cellsize / 2. - pos[2]);
			x0 = (.00001 + (drseq(&rs) - .5) * .00005) * (1+level);
			x1 = (.00001 + (drseq(&rs) - .5) * .00005) * (1+level);
			z0 = (.00001 + (drseq(&rs) - .5) * .00005) * (1+level);
			z1 = (.00001 + (drseq(&rs) - .5) * .00005) * (1+level);
			red = rseq(&rs);
/*			if((-1. < pos[0] && pos[0] < 1. && -1. < pos[2] && pos[2] < 1.
				? -.01 < pos[0] && pos[0] < .01
				: (pos[2] < -.01 || .01 < pos[2])))
				continue;*/
			base = 0. - ortpos[1];
			pos[1] += base;
	#if 0
			if(glcullFrustum(&pos, .005, &g_glcull))
				continue;
	#elif 1
			if(pos[2] + ortpos[2] < -ISLAND3_HALFLEN || ISLAND3_HALFLEN < pos[2] + ortpos[2])
				continue;
			{
				avec4_t vec;
				double d, period = ISLAND3_INRAD * M_PI * 2. / 3.;
				d = pos[0] + ortpos[0] + ISLAND3_INRAD * M_PI / 6.;
				d /= period;
				if(.5 < d - floor(d))
					continue;
				VECCPY(dst, pos);
				VECSADD(dst, nh, .003 / vw->gc->getFov());
				MAT4VP3(vec, mat, dst);
				if(vec[2] < 0. || vec[0] < -vec[2] || vec[2] < vec[0] || vec[1] < -vec[2] || vec[2] < vec[1])
					continue;
			}
	#else
			VECSUB(dst, pos, pl.pos);
	/*			VECSADD(dst, velo, -.08);*/
			if(VECSP(pos, nh) < -0.005)
				continue;
	#endif
			{
				double apos[3];
				VECADD(apos, pos, vw->pos);
				phase = (apos[0] + apos[2]) / .00241 + t / .2;
				phase2 = 1.232123 * ((apos[0] - apos[2]) / .005 + t / .2);
				disp[0] = pos[0] + (pos[1] - base) * .15 * (sin(phase) + sin(phase2));
				disp[1] = pos[1];
				disp[2] = pos[2] + (pos[1] - base) * .15 * (cos(phase) + cos(phase2));
			}
			if(level == 5){
				struct random_sequence rs;
				double org[2][3];
				int j, c;
				init_rseq(&rs, i);
				memcpy(org[0], pos, sizeof org[0]);
				org[0][1] = base;
				memcpy(org[1], pos, sizeof org[0]);
				org[1][0] += x1 * 2;
				org[1][1] = base;
				org[1][2] += z1 * 2;
				frexp(1e2 * VECLEN(org[0]), &c);
				c = 4 - c;
				if(4 < c) c = 4;
				if(0 <= c){
					GLubyte col[4] = {128, 0, 0, 255};
					avec3_t norm;
					if(tris || fan){
						tris = fan = 0;
						glEnd();
					}
					VECSCALE(norm, pos, -1.);
					glNormal3dv(norm);
					glBegin(GL_QUADS);
					for(j = 0; j < 4; j++){
//						glColor4ubv(BlendLight(col));
//						drawtree(org, 4, 4 - c, &rs);
					}
					glEnd();
				}
			}
#define flowermod 47
			if(red % flowermod < 3){
				int j, k;
				double (*cuts)[2];
				GLdouble v[3][3] = {
					{0., 0.0001, 0.},
					{.0001, .00015, .00005},
					{.0001, .00015, -.00005},
				};
				GLubyte cc[4], oc[4];
				if(tris){
					tris = 0;
					glEnd();
				}
				if(!fan){
					glBegin(GL_TRIANGLE_FAN);
				}
				if(red % flowermod == 0)
					cc[0] = 191, cc[1] = 128, cc[2] = 0, cc[3] = 255, oc[0] = 255, oc[1] = 255, oc[2] = 0, oc[3] = 255;
				else if(red % flowermod == 1)
					cc[0] = 128, cc[1] = 0, cc[2] = 191, cc[3] = 255, oc[0] = 255, oc[1] = 0, oc[2] = 255, oc[3] = 255;
				else if(red % flowermod == 2)
					cc[0] = 128, cc[1] = 128, cc[2] = 191, cc[3] = 255, oc[0] = 255, oc[1] = 255, oc[2] = 255, oc[3] = 255;
//				glColor4ubv(BlendLight(cc));
				glVertex3dv(disp);
				cuts = CircleCuts(5);
//				glColor4ubv(BlendLight(oc));
				for(j = 0; j <= 5; j++){
					k = j % 5;
					glVertex3d(disp[0] + .0001 * cuts[k][0], disp[1] + .00003, disp[2] + .0001 * cuts[k][1]);
				}
				glEnd();
			}
			{
				double disp2[3];
				if(fan){
					fan = 0;
					glEnd();
				}
/*				if(!tris){
					tris = 1;
					glBegin(GL_TRIANGLES);
				}*/

				disp2[0] = pos[0] + (pos[1] - base) * .15 * (sin(phase + .1 * M_PI) + sin(phase2 + .1 * M_PI)) / 2.;
				disp2[1] = (pos[1] + base) / 2.;
				disp2[2] = pos[2] + (pos[1] - base) * .15 * (cos(phase + .1 * M_PI) + cos(phase2 + .1 * M_PI)) / 2.;

				glBegin(GL_TRIANGLES);
				glColor4ub(red % 50 + 31,255,0,255);
				glVertex3d(disp[0], disp[1], disp[2]);
				glColor4ub(red % 50 + 31,191,0,255);
				glVertex3d(disp2[0] + x0 / 2., disp2[1], disp2[2] + z0 / 2.);
				glVertex3d(disp2[0] + x1 / 2., disp2[1], disp2[2] + z1 / 2.);
				glEnd();

				glBegin(GL_QUADS);
				glVertex3d(disp2[0] + x0 / 2., disp2[1], disp2[2] + z0 / 2.);
				glVertex3d(disp2[0] + x1 / 2., disp2[1], disp2[2] + z1 / 2.);
				glColor4ub(red % 50 + 31,127,0,255);
				glVertex3d(pos[0] + x1, base, pos[2] + z1);
				glVertex3d(pos[0] + x0, base, pos[2] + z0);
				glEnd();
			}
			if(red % 2423 < 513){
				avec3_t v0, v1, v2, v01, v02, vp;
				glBegin(GL_TRIANGLES);
				v0[0] = disp[0] - z0, v0[1] = disp[1], v0[2] = disp[2] + x0;
				v1[0] = pos[0] + x0, v1[1] = base, v1[2] = pos[2] + z0;
				v2[0] = pos[0] + x1, v2[1] = base, v2[2] = pos[2] + z1;
				VECSUB(v01, v1, v0);
				VECSUB(v02, v2, v0);
				VECVP(vp, v01, v02);
				glNormal3dv(vp);
				glColor4ub(red % 50 + 31,255,0,255);
				glVertex3dv(v0);
				glColor4ub(red % 50 + 31,127,0,255);
				glVertex3dv(v1);
				glVertex3dv(v2);
				glColor4ub(red % 50 + 31,255,0,255);
				glVertex3d(disp[0] + z0, disp[1], disp[2] - x0);
				glColor4ub(red % 50 + 31,127,0,255);
				glVertex3d(pos[0] + x0, base, pos[2] + z0);
				glVertex3d(pos[0] + x1, base, pos[2] + z1);
				glEnd();
			}
			total++;
		}
	}
	if(tris || fan)
		glEnd();
	glPopAttrib();
	glPopMatrix();
/*	fprintf(stderr, "grass[%3d]: %lg sec\n", total, TimeMeasLap(&tm));*/
}
#endif


void Island3::onChangeParent(CoordSys *oldParent){
	if(ent){
		if(!parent->w)
			parent->w = new WarSpace(parent);
		parent->w->addent(ent);
	}
}

bool Island3::readFile(StellarContext &sc, int argc, const char *argv[]){
	if(!strcmp(argv[0], "race")){
		if(1 < argc)
			race = atoi(argv[1]);
		return true;
	}
	else
		return st::readFile(sc, argc, argv);
}


/*
CoverPointVector Island3::getCoverPoint(const Vec3d &org, double radius){
	CoverPointVector ret;
	for(int i = 0; i < numof(bldgs); i++){
		Island3Building *b = bldgs[i];
		double dist = (b->pos - org).len();
		if(dist < radius) for(int ix = -1; ix < 2; ix += 2) for(int iz = -1; iz < 2; iz += 2){
			CoverPoint cp;
			cp.pos = b->pos + b->rot.trans(Vec3d(ix * b->halfsize[0], 0, iz * b->halfsize[2]));
			cp.rot = b->rot;
			cp.type = ix < 0 ? cp.LeftEdge : cp.RightEdge;
			ret.push_back(cp);
		}
	}
	return ret;
}*/


Island3Entity::Island3Entity(Game *game) : st(game), btshape(NULL){
}

Island3Entity::Island3Entity(WarField *w, Island3 &astro) : st(w), astro(&astro), btshape(NULL){
	health = getMaxHealth();
	race = astro.race;
	docker = new Island3Docker(this);
}

Island3Entity::~Island3Entity(){
	astro->ent = NULL;
	delete btshape;
}

bool singleObjectRaytest(btRigidBody *bbody, const btVector3& rayFrom,const btVector3& rayTo,btScalar &fraction,btVector3& worldNormal,btVector3& worldHitPoint)
{
	btScalar closestHitResults = 1.f;

	btCollisionWorld::ClosestRayResultCallback resultCallback(rayFrom,rayTo);

	bool hasHit = false;
	btConvexCast::CastResult rayResult;
//	btSphereShape pointShape(0.0f);
	btTransform rayFromTrans;
	btTransform rayToTrans;

	rayFromTrans.setIdentity();
	rayFromTrans.setOrigin(rayFrom);
	rayToTrans.setIdentity();
	rayToTrans.setOrigin(rayTo);

	//do some culling, ray versus aabb
	btVector3 aabbMin,aabbMax;
	bbody->getCollisionShape()->getAabb(bbody->getWorldTransform(),aabbMin,aabbMax);
	btScalar hitLambda = 1.f;
	btVector3 hitNormal;
	btCollisionObject	tmpObj;
	tmpObj.setWorldTransform(bbody->getWorldTransform());


//	if (btRayAabb(rayFrom,rayTo,aabbMin,aabbMax,hitLambda,hitNormal))
	{
		//reset previous result

		btCollisionWorld::rayTestSingle(rayFromTrans,rayToTrans, &tmpObj, bbody->getCollisionShape(), bbody->getWorldTransform(), resultCallback);
		if (resultCallback.hasHit())
		{
			//float fog = 1.f - 0.1f * rayResult.m_fraction;
			resultCallback.m_hitNormalWorld.normalize();//.m_normal.normalize();
			worldNormal = resultCallback.m_hitNormalWorld;
			//worldNormal = transforms[s].getBasis() *rayResult.m_normal;
			worldNormal.normalize();
			worldHitPoint = resultCallback.m_hitPointWorld;
			hasHit = true;
			fraction = resultCallback.m_closestHitFraction;
		}
	}

	return hasHit;
}

int Island3Entity::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
#if 1
	double best = dt, retf;
	int reti = 0, n;
	for(n = 0; n < 3; n++){
		Vec3d org;
		Quatd rot;
		org = this->rot.trans(btvc(wingtrans[n].getOrigin())) + this->pos;
		rot = this->rot * btqc(wingtrans[n].getRotation());
		Vec3d sc = btvc(wings[n]->getHalfExtentsWithMargin()) + rad * Vec3d(1,1,1);
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = n + 1;
		}
	}
	if(reti)
		return reti;
#endif
	if(w && bbody){
		btScalar btfraction;
		btVector3 btnormal, btpos;
		btVector3 from = btvc(src);
		btVector3 to = btvc(src + (dir - velo) * dt);
		if((from - to).length() < 1e-13);
		else if(WarSpace *ws = *w){
			btCollisionWorld::ClosestRayResultCallback callback(from, to);
			ws->bdw->rayTest(from, to, callback);
			if(callback.hasHit() && callback.m_collisionObject == bbody){
				if(ret) *ret = callback.m_closestHitFraction * dt;
				if(retp) *retp = btvc(callback.m_hitPointWorld);
				if(retn) *retn = btvc(callback.m_hitNormalWorld);
				return 1;
			}
		}
		else if(singleObjectRaytest(bbody, from, to, btfraction, btnormal, btpos)){
			if(ret) *ret = btfraction * dt;
			if(retp) *retp = btvc(btpos);
			if(retn) *retn = btvc(btnormal);
			return 1;
		}
	}
	return 0;
}

int Island3Entity::takedamage(double damage, int hitpart){
	int ret = 1;
	if(0 < health && health - damage <= 0){
		ret = 0;
		WarSpace *ws = *w;
		if(ws){
			AddTeline3D(ws->tell, this->pos, vec3_000, 30., quat_u, vec3_000, vec3_000, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
		}
		astro->flags |= CS_DELETE;
		w = NULL;
	}
	health -= damage;
	return ret;
}

Model *Island3Entity::loadModel(){
	static OpenGLState::weak_ptr<bool> init;
	if(!init){
		dockModel = LoadMQOModel(modPath() + "models/island3dock.mqo");
		init.create(*openGLState);
	}
	return dockModel;
}

// Docking bays
void Island3Entity::draw(WarDraw *wd){
#if 1
	Model *model = loadModel();
	if(model){
		static const double normal[3] = {0., 1., 0.};
		static const double dscale = 10.;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,0,-1,0,
			0,-1,0,0,
			0,0,0,1,
		};

		glPushMatrix();
		gldTranslate3dv(pos);
		gldMultQuat(rot);

		GLattrib gla(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

		glPushMatrix();
		glTranslated(0, -ISLAND3_HALFLEN - ISLAND3_RAD, 0.);
		gldScaled(dscale);
		glRotatef(-astro->rotation * deg_per_rad, 0, 1, 0);
		glMultMatrixd(rotaxis);
		DrawMQOPose(model, NULL);
		glPopMatrix();

		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
	}
#endif
}

Docker *Island3Entity::getDockerInt(){
	return docker;
}


static const double cutheight = 200.;
static const double floorHeight = 3.75;
static const double floorStep = 0.5;

static const GLdouble vertex[][3] = {
  { -1.0, 0.0, -1.0 },
  { 1.0, 0.0, -1.0 },
  { 1.0, 1.0, -1.0 },
  { -1.0, 1.0, -1.0 },
  { -1.0, 0.0, 1.0 },
  { 1.0, 0.0, 1.0 },
  { 1.0, 1.0, 1.0 },
  { -1.0, 1.0, 1.0 }
};

static const int edges[][2] = {
  { 0, 1 },
  { 1, 2 },
  { 2, 3 },
  { 3, 0 },
  { 4, 5 },
  { 5, 6 },
  { 6, 7 },
  { 7, 4 },
  { 0, 4 },
  { 1, 5 },
  { 2, 6 },
  { 3, 7 }
};

static const int face[][4] = {
	{2,3,7,6},
/*	{0,1,5,4},*/

	{0,3,2,1},
	{5,6,7,4},
	{4,7,3,0},
	{1,2,6,5},

	{1,2,3,0},
	{4,7,6,5},
	{0,3,7,4},
	{5,6,2,1},
};
static const double normal[][3] = {
	{ 0.,  1.,  0.},
/*	{ 0., -1.,  0.},*/

	{ 0.,  0., -1.},
	{ 0.,  0.,  1.},
	{-1.,  0.,  0.},
	{ 1.,  0.,  0.},

	{ 0.,  0.,  1.},
	{ 0.,  0., -1.},
	{ 1.,  0.,  0.},
	{-1.,  0.,  0.},

	{ 0., -1.,  0.},
};
static const double texcoord[][3] = {
	{ 0.,  0., 0.},
	{ 0.,  1., 0.},
	{ 1.,  1., 0.},
	{ 1.,  0., 0.},
};

Island3Building::Island3Building(WarField *w, Island3 *host) : st(w), host(host){
	health = getMaxHealth();
	race = -1;
	RandomSequence rs((unsigned long)this);
	double phase = rs.nextd() * M_PI;
	phase += floor(phase / (M_PI / 3.)) * M_PI / 3. - M_PI / 6.;
	pos[0] = sin(phase) * ISLAND3_INRAD;
	pos[1] = 2. * ISLAND3_HALFLEN * (rs.nextd() - .5);
	pos[2] = cos(phase) * ISLAND3_INRAD;
	rot = Quatd::rotation(phase, 0, 1, 0).rotate(M_PI / 2., -1, 0, 0);
	halfsize[0] = rs.nextd() * .10 + 10.;
	halfsize[1] = rs.nextd() * .30 + 10.;
	halfsize[1] -= fmod(halfsize[1], floorHeight);
	halfsize[2] = rs.nextd() * .10 + 10.;
}


void Island3Building::enterField(WarField *target){
	WarSpace *ws = *target;
	if(ws && ws->bdw){
		if(!bbody){
			btBoxShape *shape = new btBoxShape(btvc(halfsize));

			btTransform startTransform;
			startTransform.setIdentity();
			startTransform.setOrigin(btvc(pos));
			startTransform.setRotation(btqc(rot));

			//rigidbody is dynamic if and only if mass is non zero, otherwise static
			bool isDynamic = false && (mass != 0.f);

			btVector3 localInertia(0,0,0);
			if (isDynamic)
				shape->calculateLocalInertia(mass,localInertia);

			//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
			btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
			btRigidBody::btRigidBodyConstructionInfo rbInfo(0*mass,myMotionState,shape,localInertia);
	//		rbInfo.m_linearDamping = .5;
	//		rbInfo.m_angularDamping = .5;
			bbody = new btRigidBody(rbInfo);

	//		bbody->setSleepingThresholds(.0001, .0001);
		}

		//add the body to the dynamics world
		ws->bdw->addRigidBody(bbody);
	}
}

int Island3Building::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double retf;
	int reti = 0, n;
	Vec3d org = this->pos;
	Quatd rot = this->rot;

	double sc[3];
	for(int j = 0; j < 3; j++)
		sc[j] = halfsize[j] + rad;
	if((jHitBox(org, sc, rot, src, dir, 0., dt, &retf, retp, retn)) && retf < dt){
		if(ret) *ret = retf;
		reti = 1;
	}
	return reti;
}

void Island3Building::draw(wardraw_t *wd){
	static const gltestp::dstring bldgname("bldg.jpg");
	if(!gltestp::FindTexture(bldgname)){
		suftexparam_t stp;
		stp.flags = STP_ALPHA | STP_MIPMAP | STP_MINFIL | STP_MAGFIL;
		stp.alphamap = STP_MASKTEX;
		stp.minfil = GL_LINEAR_MIPMAP_LINEAR;
		stp.magfil = GL_LINEAR;
		stp.bmiMask = ReadBitmap("textures/building3mask.bmp");
		CallCacheBitmap("bldg.jpg", "textures/building3.jpg", &stp, NULL);
		LocalFree((void*)stp.bmiMask);
	}

	if(wd->vw->gc->cullFrustum(wd->vw->cs->tocs(pos, host), getHitRadius() + halfsize[1]))
		return;

	{
		const double scale = 1;
		const double n[3] = {0., 1., 0.};
		const double texHeight = floorHeight * 2.;
		int i;
		int heightcuts = int(halfsize[1] / cutheight) + 1;
		glPushMatrix();
		gldTranslate3dv(pos);
		gldMultQuat(rot);
		gldScaled(scale);
		glScaled(halfsize[0], halfsize[1], halfsize[2]);
		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
		static const GLfloat mat_specular[4] = {0.f, 0.f, 0.f, 1.f};
		static const GLfloat mat_diffuse[4] = {.8f, .8f, .8f, 1.f};
		static const GLfloat mat_ambient[4] = {.6f, .6f, .6f, 1.f};
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialf(GL_FRONT, GL_SHININESS, 50.f);
		glDisable(GL_TEXTURE_2D);
		if(const TexCacheBind *stc = FindTexture(bldgname))
			glCallList(stc->getList());
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);
//		glEnable(GL_ALPHA_TEST);
//		glAlphaFunc(GL_GREATER, .5f);
		glBegin(GL_QUADS);
		for(i = 0; i < 5; i++){
			int j, k;
			glNormal3dv(normal[i]);
			if(!i || heightcuts == 1) for(j = 0; j < 4; j++){
				if(!normal[i][1])
					glTexCoord2d(texcoord[j][0] * halfsize[2*!!normal[i][0]] * 2 / .01, texcoord[j][1] * halfsize[1] / texHeight);
				else
					glTexCoord2i(0,0);
				glVertex3dv(vertex[face[i][j]]);
			}
			else for(k = 0; k < heightcuts; k++){
				double base;
				j = 0;
				base = floor((texcoord[j][1] * (heightcuts - k - 1) + texcoord[j+1][1] * k) / heightcuts * halfsize[1] / texHeight);
				glTexCoord2d(texcoord[j][0] * halfsize[2*!!normal[i][0]] * 2 / .01, ((texcoord[j][1] * (heightcuts - k - 1) + texcoord[j+1][1] * k) / heightcuts * halfsize[1] / texHeight - base));
				glVertex3d(vertex[face[i][j]][0], (vertex[face[i][j]][1] * (heightcuts - k - 1) + vertex[face[i][j+1]][1] * k) / heightcuts, vertex[face[i][j]][2]);
				j = 1;
				glTexCoord2d(texcoord[j][0] * halfsize[2*!!normal[i][0]] * 2 / .01, ((texcoord[j-1][1] * (heightcuts - k)  + texcoord[j][1] * (k+1)) / heightcuts * halfsize[1] / texHeight - base));
				glVertex3d(vertex[face[i][j]][0], (vertex[face[i][j-1]][1] * (heightcuts - k) + vertex[face[i][j]][1] * (k+1)) / heightcuts, vertex[face[i][j]][2]);
				j = 2;
				glTexCoord2d(texcoord[j][0] * halfsize[2*!!normal[i][0]] * 2 / .01, ((texcoord[j+1][1] * (heightcuts - k) + texcoord[j][1] * (k+1)) / heightcuts * halfsize[1] / texHeight - base));
				glVertex3d(vertex[face[i][j]][0], (vertex[face[i][j+1]][1] * (heightcuts - k) + vertex[face[i][j]][1] * (k+1)) / heightcuts, vertex[face[i][j]][2]);
				j = 3;
				glTexCoord2d(texcoord[j][0] * halfsize[2*!!normal[i][0]] * 2 / .01, ((texcoord[j][1] * (heightcuts - k - 1) + texcoord[j-1][1] * k) / heightcuts * halfsize[1] / texHeight - base));
				glVertex3d(vertex[face[i][j]][0], (vertex[face[i][j]][1] * (heightcuts - k - 1) + vertex[face[i][j-1]][1] * k) / heightcuts, vertex[face[i][j]][2]);
			}
		}
		glEnd();

#if 0 // Disable internal structure drawing for the moment.
		if(0 || pos[0] - halfsize[0] < wd->vw->pos[0] && wd->vw->pos[0] < pos[0] + halfsize[0]
		&& pos[2] - halfsize[2] < wd->vw->pos[2] && wd->vw->pos[2] < pos[2] + halfsize[2]
		&& pos[1] < wd->vw->pos[1] && wd->vw->pos[1] < pos[1] + halfsize[1]){
			int floor, totalFloors = halfsize[1] / floorHeight;
			int plfloor;
			plfloor = (wd->vw->pos[1] - pos[1]) / floorHeight; 

			glPushMatrix();
			glTranslated(0, plfloor * floorHeight / halfsize[1], 0);
			glScaled(1, floorHeight / halfsize[1], 1);
			glEnable(GL_ALPHA_TEST);
			glBegin(GL_QUADS);
			for(i = 5; i < numof(face); i++){
				int j;
				glNormal3dv(normal[i]);
				for(j = 0; j < 4; j++){
					if(!normal[i][1])
						glTexCoord2d(texcoord[j][0] * halfsize[2*!!normal[i][0]] * 2 / .01, texcoord[j][1] * .5/* * p->halfsize[1] / texHeight*/);
					glVertex3dv(vertex[face[i][j]]);
				}
			}
			glEnd();
			glPopMatrix();

			glCallList(CallCacheBitmap("bricks.bmp", "models/bricks.bmp", NULL, NULL));
			glBegin(GL_QUADS);
			for(floor = plfloor; floor <= MIN(plfloor + 1, totalFloors); floor++){
				glNormal3dv(normal[0]);
				glTexCoord2d(0, 0); glVertex3d(-1, (floor * floorHeight + floorStep) / halfsize[1], -1);
				glTexCoord2d(0, 1); glVertex3d(-1, (floor * floorHeight + floorStep) / halfsize[1],  1);
				glTexCoord2d(1, 1); glVertex3d( 1, (floor * floorHeight + floorStep) / halfsize[1],  1);
				glTexCoord2d(1, 0); glVertex3d( 1, (floor * floorHeight + floorStep) / halfsize[1], -1);
				glNormal3dv(normal[numof(face)]);
				glTexCoord2d(0, 0); glVertex3d(-1, (floor * floorHeight) / halfsize[1], -1);
				glTexCoord2d(1, 0); glVertex3d( 1, (floor * floorHeight) / halfsize[1], -1);
				glTexCoord2d(1, 1); glVertex3d( 1, (floor * floorHeight) / halfsize[1],  1);
				glTexCoord2d(0, 1); glVertex3d(-1, (floor * floorHeight) / halfsize[1],  1);
			}
			glEnd();
		}
#endif
		glPopAttrib();
		glPopMatrix();
	}
}

int Island3Entity::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append("Dock Window", 'd', "dockmenu");
	return ret;
}

bool Island3Entity::command(EntityCommand *com){
	if(TransportResourceCommand *trc = InterpretCommand<TransportResourceCommand>(com)){
		astro->gases += trc->gases;
		astro->solids += trc->solids;
		return true;
	}
	if(TransportPeopleCommand *tpc = InterpretCommand<TransportPeopleCommand>(com)){
		astro->people += tpc->people;
		return true;
	}
	else		
		return st::command(com);
}



const unsigned Island3Docker::classid = registerClass("Island3Docker", Conster<Island3Docker>);

Island3Docker::Island3Docker(Island3Entity *ent) : st(ent){
}

const char *Island3Docker::classname()const{return "Island3Docker";}


void Island3Docker::dock(Dockable *d){
	d->dock(this);
	if(!strcmp(d->idname(), "ContainerHead") || !strcmp(d->idname(), "SpacePlane"))
		d->w = NULL;
	else{
		if(e->w)
			e->w = this;
		else{
			e->w = this;
			addent(e);
		}
	}
}

Vec3d Island3Docker::getPortPos(Dockable *)const{
	Vec3d yhat = e->rot.trans(Vec3d(0,1,0));
	return yhat * (-16. - 3.25) + e->pos;
}

Quatd Island3Docker::getPortRot(Dockable *)const{
	return e->rot;
}


Island3WarSpace::Island3WarSpace(Island3 *cs) : st(cs), bbody(NULL){
	if(bdw){
		static btCompoundShape *shape = NULL;
		if(!shape){
			shape = new btCompoundShape();

			static const double radius[2][2] = {
				{200. + ISLAND3_RAD / 2., 200. + ISLAND3_RAD / 2.},
				{ISLAND3_GRAD + 500. - 1., ISLAND3_INRAD + 500. - 1.},
			};
			static const double thickness[2] = {
				ISLAND3_RAD / 2. + 9.,
				500.,
			};
			static const double widths[2] = {
				130. * 8,
				130.,
			};
			static const double lengths[2][2] = {
				{-ISLAND3_HALFLEN - ISLAND3_RAD, -ISLAND3_HALFLEN},
				{-ISLAND3_HALFLEN, ISLAND3_HALFLEN},
			};
			static const int cuts_n[2] = {12, 96};

			for(int n = 0; n < 2; n++){
				const int cuts = cuts_n[n];
				for(int i = 0; i < cuts; i++){
					const Vec3d sc(widths[n], (lengths[n][1] - lengths[n][0]) / 2., thickness[n]);
					const Quatd rot = Quatd::rotation(2 * M_PI * (i + .5) / cuts, 0, 1, 0);
					const Vec3d pos = rot.trans(Vec3d(0, (lengths[n][1] + lengths[n][0]) / 2., radius[n][(i + cuts / 12) % (cuts / 3) < cuts / 6]));
					btBoxShape *box = new btBoxShape(btvc(sc));
					btTransform trans = btTransform(btqc(rot), btvc(pos));
					shape->addChildShape(trans, box);
				}
			}

			btBoxShape *endbox = new btBoxShape(btVector3(ISLAND3_RAD, ISLAND3_RAD / 2., ISLAND3_RAD));
			shape->addChildShape(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, ISLAND3_HALFLEN + ISLAND3_RAD / 2., 0)), endbox);
		}
		btTransform startTransform;
		startTransform.setIdentity();
//		startTransform.setOrigin(btvc(pos));

		// Assume Island hull is static
		btVector3 localInertia(0,0,0);

		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(0,myMotionState,shape,localInertia);
		bbody = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		bdw->addRigidBody(bbody);
	}
}

Vec3d Island3WarSpace::accel(const Vec3d &srcpos, const Vec3d &srcvelo)const{
	double omg2 = cs->omg.slen();

	/* centrifugal force */
	Vec3d ret(srcpos[0] * omg2, 0., srcpos[2] * omg2);

	/* coriolis force */
	Vec3d coriolis = srcvelo.vp(sqrt(omg2) * Vec3d(0,1,0));
	ret += coriolis;
	return ret;
}

Quatd Island3WarSpace::orientation(const Vec3d &pos)const{
	static const Quatd org(-sqrt(2.) / 2., 0, 0, sqrt(2.) / 2.);
	double phase = atan2(pos[0], pos[2]);
	return org.rotate(phase, 0, 0, 1);
}

btRigidBody *Island3WarSpace::worldBody(){
	return bbody;
}

bool Island3WarSpace::sendMessage(Message &com){
	if(GetCoverPointsMessage *p = InterpretMessage<GetCoverPointsMessage>(com)){
		CoverPointVector &ret = p->cpv;
		Island3 *i3 = static_cast<Island3*>(cs);
		for(int i = 0; i < numof(i3->bldgs); i++){
			Island3Building *b = i3->bldgs[i];
			double dist = (b->pos - p->org).len();
			if(dist < p->radius) for(int ip = -1; ip < 2; ip += 2){
				int adder = (ip - 1) / 2;
				for(int ir = 0; ir < 4; ir++){
					Quatd tr = b->rot * Quatd::rotation(M_PI / 2. * (ir + adder), 0, ip, 0);
					CoverPoint cp;

					// Position is determined in the Entity's local frame of reference.
					cp.pos = b->pos + b->rot.rotate(M_PI / 2. * ir, 0, ip, 0).trans(Vec3d(b->halfsize[ir % 2 * 2], 0, b->halfsize[(ir + 1) % 2 * 2]));

					// Rotation is made by ir
					cp.rot = tr;

					cp.type = ip < 0 ? cp.LeftEdge : cp.RightEdge;
					ret.push_back(cp);
				}
			}
		}
		return true;
	}
	else
		return false;
}


