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
#include <math.h>
//#include <gl/glext.h>

static void smokedraw_int(const struct tent3d_line_callback *p, COLOR32 col, float alpha){
	static GLuint list = 0;
	if(!list){
		suftexparam_t stp;
		stp.flags = STP_ENV | STP_ALPHA | STP_ALPHATEX | STP_MAGFIL | STP_MINFIL;
		stp.env = GL_MODULATE;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		list = CallCacheBitmap5("smoke2.jpg", "smoke2.jpg", &stp, NULL, NULL);
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
		double randy = 2. * M_PI * p->life * (drseq(&rs) - .5);
		gldMultQuat(qret * Quatd(0, 0, sin(randy), cos(randy)));
	}
	gldScaled((2. - p->life) * p->len);
	glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
	float alpha = MIN(p->life / 1.5, 1.);
	smokedraw_int(p, (COLOR32)private_data, alpha);
	glPopAttrib();
	glPopMatrix();
}


void debrigib(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	if(dd->pgc && (dd->pgc->cullFrustum(pl->pos, .01) || (dd->pgc->scale(pl->pos) * .01) < 5))
		return;

	static suf_t *sufs[5] = {NULL};
	static VBO *vbo[5];
	static GLuint lists[numof(sufs)] = {0};
	if(!sufs[0]){
		char buf[64];
		for(int i = 0; i < numof(sufs); i++){
			sprintf(buf, "models/debris%d.bin", i);
			sufs[i] = CallLoadSUF(buf);
			vbo[i] = CacheVBO(sufs[i]);
		}
	}
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
