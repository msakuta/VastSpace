#include "effects.h"
#include "../bitmap.h"
#include "../material.h"
#include "../tent3d.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufvbo.h>
#include <clib/GL/gldraw.h>
#include <clib/rseq.h>
}
#include <cpplib/RandomSequence.h>
#include <math.h>
#include <gl/glext.h>

void sparkdraw(const tent3d_line_callback *p, const tent3d_line_drawdata *dd, void *private_data){
	double width = p->len;
	Vec3d start = p->pos;
	Vec3d end = p->pos;
	end += p->velo / 10. /** -(runlength / velolen < length ? runlength / velolen : length)*/;
	double pixels = fabs(dd->pgc->scale(p->pos)) * width;
	float alpha = MIN(1., p->life / .25);
	if(pixels < 2.){
		glBegin(GL_LINES);
		glColor4f(1., 1., .5, 0.);
		glVertex3dv(start);
		glColor4f(1., 1., .5, pixels / 2. * alpha);
		glVertex3dv(end);
		glEnd();
		return;
	}
	static GLuint texname = 0;
	static const GLfloat envcolor[4] = {.5,0,0,1};
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
	if(!texname){
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		texname = CallCacheBitmap5("textures/ricochet.bmp", "textures/ricochet.bmp", &stp, NULL, NULL);
	}
	glCallList(texname);
	glColor4f(1,1,1, alpha);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Add blend
	gldTextureBeam(dd->viewpoint, end, start, width);
	glPopAttrib();
}

void sparkspritedraw(const tent3d_line_callback *p, const tent3d_line_drawdata *dd, void *private_data){
	double width = p->len;
	Vec3d start = p->pos;
	double pixels = fabs(dd->pgc->scale(p->pos)) * width;
	float alpha = MIN(1., p->life / .25);
	if(pixels < 2.){
		return;
	}
	static GLuint texname = 0;
	static const GLfloat envcolor[4] = {.5,0,0,1};
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
	if(!texname){
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		texname = CallCacheBitmap5("textures/spark.bmp", "textures/spark.bmp", &stp, NULL, NULL);
	}
	glCallList(texname);
	glColor4f(1,1,1, alpha);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Add blend
	glPushMatrix();
	gldTranslate3dv(p->pos);
	gldScaled(p->len);
	glMultMatrixd(dd->invrot);
	gldMultQuat(p->rot);
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(-1,-1);
	glTexCoord2i(1, 0); glVertex2i( 1,-1);
	glTexCoord2i(1, 1); glVertex2i( 1, 1);
	glTexCoord2i(0, 1); glVertex2i(-1, 1);
	glEnd();
	glPopMatrix();
	glPopAttrib();
}

static void smokedraw_int(const struct tent3d_line_callback *p, COLOR32 col, float alpha){
	static GLuint list = 0;
	if(!list){
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_ALPHA | STP_ALPHATEX | STP_MAGFIL | STP_MINFIL;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		list = CallCacheBitmap5("textures/smoke2.jpg", "textures/smoke2.jpg", &stp, NULL, NULL);
	}
	glCallList(list);
	glColor4f(COLOR32R(col) / 255., COLOR32G(col) / 255., COLOR32B(col) / 255., MIN(p->life * .25, 1.));
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Vec4<float>(.5f, .5f, .5f, alpha));
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Vec4<float>(.5f, .5f, .5f, alpha));
/*	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex2f(-1, -1);
	glTexCoord2f(1,0); glVertex2f(+1, -1);
	glTexCoord2f(1,1); glVertex2f(+1, +1);
	glTexCoord2f(0,1); glVertex2f(-1, +1);
	glEnd();*/
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f( .5,  .5); glNormal3f( 0,  0, 1); glVertex2f( 0,  0);
	glTexCoord2f( .0,  .0); glNormal3f(-1, -1, 0); glVertex2f(-1, -1);
	glTexCoord2f( 1.,  .0); glNormal3f( 1, -1, 0); glVertex2f( 1, -1);
	glTexCoord2f( 1.,  1.); glNormal3f( 1,  1, 0); glVertex2f( 1,  1);
	glTexCoord2f( 0.,  1.); glNormal3f(-1,  1, 0); glVertex2f(-1,  1);
	glTexCoord2f( 0.,  0.); glNormal3f(-1, -1, 0); glVertex2f(-1, -1);
	glEnd();
}


void smokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data){
	glPushMatrix();
	gldTranslate3dv(p->pos);
//	glMultMatrixd(dd->invrot);
	{
		Quatd rot;
		rot = dd->rot;
		Vec3d delta = dd->viewpoint - p->pos;
		Vec3d ldelta = rot.trans(delta);
		Quatd qirot = Quatd::direction(ldelta);
		Quatd qret = rot.cnj() * qirot;
		gldMultQuat(qret);
	}
	gldScaled(p->len);
	struct random_sequence rs;
	init_rseq(&rs, (long)p);
	glRotated(rseq(&rs) % 360, 0, 0, 1);
//	gldMultQuat(Quatd::direction(Vec3d(p->pos) - Vec3d(dd->viewpoint)));
	glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
	float alpha = MIN(p->life * .25, 1.);
	smokedraw_int(p, (COLOR32)private_data, alpha);
	glPopAttrib();
	glPopMatrix();
}

void smokedraw_swirl(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data){
	const smokedraw_swirl_data *data = (const smokedraw_swirl_data*)private_data;
	glPushMatrix();
	gldTranslate3dv(p->pos);
//	glMultMatrixd(dd->invrot);
	{
		Quatd rot;
		rot = dd->rot;
		Vec3d delta = dd->viewpoint - p->pos;
		Vec3d ldelta = rot.trans(delta);
		Quatd qirot = Quatd::direction(ldelta);
		Quatd qret = rot.cnj() * qirot;
		struct random_sequence rs;
		init_rseq(&rs, (unsigned long)p);
		double randy = p->omg[2] * -p->life + 2. * M_PI * (drseq(&rs) - .5);
		gldMultQuat(qret * Quatd(0, 0, sin(randy), cos(randy)));
	}
	if(data->expand)
		gldScaled((2. - p->life) * p->len);
	else
		gldScaled(p->len);
	glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
	float alpha = MIN(p->life / 1.5, 1.);
	smokedraw_int(p, data->col, alpha);
	glPopAttrib();
	glPopMatrix();
}


static suf_t *sufs[5] = {NULL};
static VBO *vbo[5];
static GLuint lists[numof(sufs)] = {0};

static void debrigib_init(){
	if(!sufs[0]){
		char buf[64];
		for(int i = 0; i < numof(sufs); i++){
			sprintf(buf, "models/debris%d.bin", i);
			sufs[i] = CallLoadSUF(buf);
			vbo[i] = CacheVBO(sufs[i]);
		}
	}
}

void debrigib(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	if(dd->pgc && (dd->pgc->cullFrustum(pl->pos, .01) || (dd->pgc->scale(pl->pos) * .01) < 5))
		return;

	debrigib_init();
	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
//	glBindTexture(GL_TEXTURE_2D, texname);
	glPushMatrix();
	gldTranslate3dv(pl->pos);
	gldMultQuat(pl->rot);
	gldScaled(.0001);
	struct random_sequence rs;
	initfull_rseq(&rs, 13230354, (unsigned long)pl);
	unsigned id = rseq(&rs) % numof(sufs);
//	if(!lists[id]){
//		glNewList(lists[id] = glGenLists(1), GL_COMPILE_AND_EXECUTE);
//		DrawSUF(sufs[id], SUF_ATR, NULL);
		DrawVBO(vbo[id], SUF_ATR, NULL);
//		glEndList();
//	}
//	else
//		glCallList(lists[id]);
	glPopMatrix();
	glPopAttrib();
}

// Multiple gibs drawn at once. Total number of polygons remains same, but switchings of context variables are
// reduced, therefore performance is gained compared to the one above.
void debrigib_multi(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	debrigib_init();
	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
//	glBindTexture(GL_TEXTURE_2D, texname);
	for(int i = 0; i < (int)pv; i++){
		RandomSequence rs(pl + i);
		double t = 20. - pl->life;
		Vec3d pos = pl->pos
			+ .100 * Vec3d(rs.nextGauss(), rs.nextGauss(), rs.nextGauss()) // origin
			+ t * .025 * Vec3d(rs.nextGauss(), rs.nextGauss(), rs.nextGauss()); // linear motion
		if(dd->pgc && (dd->pgc->cullFrustum(pos, .01) || (dd->pgc->scale(pos) * .01) < 5) || pl->life < rs.nextd() * 2.)
			continue;
		glPushMatrix();
		gldTranslate3dv(pos);
		double angle = t * 2. * M_PI * rs.nextd() / 2.;
		Quatd rot = sin(angle) * Vec3d(rs.nextGauss(), rs.nextGauss(), rs.nextGauss());
		rot[3] = cos(angle);
		gldMultQuat(pl->rot * rot);
		gldScaled(.0001);
		unsigned id = rs.next() % numof(sufs);
		DrawVBO(vbo[id], SUF_ATR, NULL);
		glPopMatrix();
	}
	glPopAttrib();
}

GLuint muzzle_texture(){
	static GLuint texname = 0;
	static bool init = false;
	if(!init){
		init = true;
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL | STP_WRAP_S;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.wraps = GL_CLAMP_TO_BORDER;
		texname = CallCacheBitmap5("textures/muzzle.bmp", "textures/muzzle.bmp", &stp, NULL, NULL);
	}
	return texname;
}
