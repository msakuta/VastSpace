/** \file
 * \brief Implementation of WarField and WarSpace.
 */
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
#include "draw/ShadowMap.h"
#include "draw/OpenGLState.h"
#include "draw/ShaderBind.h"
#include "glsl.h"
#include "msg/Message.h"
#include "msg/GetCoverPointsMessage.h"
#include "Game.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/timemeas.h>
}
#include <GL/glext.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h>

int WarSpace::g_otdrawflags = 0;

void WarField::draw(wardraw_t *wd){}
void WarField::drawtra(wardraw_t *wd){}
void WarField::drawOverlay(wardraw_t *){}


//static double gradius = 1.;
static int g_debugdraw_bullet = 0;
static int g_player_viewport = 0;
bool WarDraw::r_move_path = false;
bool WarDraw::r_attack_path = false;
bool WarDraw::r_overlay = false;

template<typename T>
int cmd_cvar(int argc, char *argv[], void *pv){
	T *pt = (T*)pv;
	if(argc < 2)
		CmdPrint(cpplib::dstring(argv[0]) << " = " << *pt);
	else if(toupper(argv[1][0]) == 'T')
		*pt = !*pt;
	else
		*pt = !!atoi(argv[1]);
	return 0;
}

static void init_gsc(){
//	CvarAdd("gradius", &gradius, cvar_double);
	CvarAdd("g_debugdraw_bullet", &g_debugdraw_bullet, cvar_int);
	CvarAdd("g_player_viewport", &g_player_viewport, cvar_int);
	CmdAddParam("r_move_path", cmd_cvar<bool>, &WarDraw::r_move_path);
	CmdAddParam("r_attack_path", cmd_cvar<bool>, &WarDraw::r_attack_path);
	CmdAddParam("r_overlay", cmd_cvar<bool>, &WarDraw::r_overlay);
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


static WarField::EntityList WarField::*const list[2] = {&WarField::el, &WarField::bl};

void WarSpace::draw(wardraw_t *wd){
	for(int i = 0; i < 2; i++)
	for(WarField::EntityList::iterator it = (this->*list[i]).begin(); it != (this->*list[i]).end(); it++){
		if(!*it)
			continue;
		Entity *pe = *it;
		if(pe->w == this/* && wd->vw->zslice == (pl->chase && pl->mover == &Player::freelook && pl->chase->getUltimateOwner() == pe->getUltimateOwner() ? 0 : 1)*/){
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
	for(WarField::EntityList::iterator it = (this->*list[i]).begin(); it != (this->*list[i]).end(); it++){
		if(!*it)
			continue;
		Entity *pe = *it;
		if(pe->w == this/* && wd->vw->zslice == (pl->chase && pl->mover == &Player::freelook && pl->chase->getUltimateOwner() == pe->getUltimateOwner() ? 0 : 1)*/){
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

	if(g_player_viewport){
		Game *game = wd->w->getGame();
		for(Game::PlayerList::iterator it = game->players.begin(); it != game->players.end(); ++it){
			Player *player = *it;
			if(player == game->player || player->cs != cs)
				continue;
			double f = player->fov;
			const Vec3d pos = player->getpos();
			const Quatd irot = player->getrot().cnj();
			glBegin(GL_LINES);
			for(int level = 4; 0 <= level; --level){
				double l = double(1 << level);
				glColor4f(!!((1 << level) & 8), !!((1 << level) & 5), !!((1 << level) & 19), 1);
				glVertex3dv(pos);
				glVertex3dv(pos + irot.trans(l * Vec3d(0,0,-1)));
				for(int y = -1; y <= 1; y++){
					glVertex3dv(pos + irot.trans(l * Vec3d(-f, y * f, -1)));
					glVertex3dv(pos + irot.trans(l * Vec3d(f, y * f, -1)));
				}
				for(int x = -1; x <= 1; x++){
					glVertex3dv(pos + irot.trans(l * Vec3d(x * f, -f,-1)));
					glVertex3dv(pos + irot.trans(l * Vec3d(x * f, f,-1)));
				}
				glVertex3dv(pos);
				glVertex3dv(pos + irot.trans(l * Vec3d(-1,-1,-1)));
				glVertex3dv(pos);
				glVertex3dv(pos + irot.trans(l * Vec3d(1,-1,-1)));
				glVertex3dv(pos);
				glVertex3dv(pos + irot.trans(l * Vec3d(-1,1,-1)));
				glVertex3dv(pos);
				glVertex3dv(pos + irot.trans(l * Vec3d(1,1,-1)));
			}
			glEnd();
		}
	}

	if(g_otdrawflags)
		ot_draw(this, wd);
}

void WarSpace::drawOverlay(wardraw_t *wd){
	Player *ppl = getPlayer();
	for(int i = 0; i < 2; i++)
	for(WarField::EntityList::iterator it = (this->*list[i]).begin(); it != (this->*list[i]).end(); it++){
		if(!*it)
			continue;
		Entity *pe = *it;
		double pixels;
		if(ppl && WarDraw::r_overlay && 0. < (pixels = wd->vw->gc->scale(pe->pos) * pe->hitradius()) && pixels * 20. < wd->vw->vp.m){
			Vec4d spos = wd->vw->trans.vp(Vec4d(pe->pos) + Vec4d(0,0,0,1));
			glPushMatrix();
			glLoadIdentity();
			glTranslated((spos[0] / spos[3] + 1.) * wd->vw->vp.w / 2., (1. - spos[1] / spos[3]) * wd->vw->vp.h / 2., 0.);
			glScaled(20, 20, 1);
			glColor4f(GLfloat(pe->race % 2), 1, GLfloat((pe->race + 1) % 2), GLfloat(1. - pixels * 20. / wd->vw->vp.m));
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


static OpenGLState::weak_ptr<AdditiveShaderBind> additiveShaderBind;

void WarDraw::init(){
	if(g_shader_enable){
		if(!additiveShaderBind){
			additiveShaderBind.create(*openGLState);
			additiveShaderBind->build();
			additiveShaderBind->getUniformLocations();
		}
	}
}

void WarDraw::setAdditive(bool b){
	if(vw->shadowmap){
		vw->shadowmap->setAdditive(b);
		return;
	}
	additive = b;
	if(additive){
		if(g_shader_enable){
			additiveShaderBind->use();
			shader = additiveShaderBind->shader;
		}
	}
	else{
		glUseProgram(0);
		shader = 0;
	}
}

const ShaderBind *WarDraw::getShaderBind(){
	if(!g_shader_enable)
		return NULL;
	else if(shadowMap)
		return vw->shadowmap->getShader();
	else
		return additiveShaderBind;
}

const AdditiveShaderBind *WarDraw::getAdditiveShaderBind(){
	if(!g_shader_enable)
		return NULL;
	else if(vw->shadowmap)
		return vw->shadowmap->getAdditive();
	else
		return additiveShaderBind;
}

