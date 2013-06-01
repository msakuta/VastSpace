
#define USEWIN 1 /* whether use windows api (wgl*) */

#if !USEWIN
#include <GL/glut.h>
#else
#include "antiglut.h"
#include <windows.h>
#endif
#include "ysdnmmot.h"
#include "mqo.h"
extern "C"{
#include "yssurf.h"
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/suf/suf.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufbin.h>
#include <clib/rseq.h>
#include <clib/avec3.h>
#include <clib/amat3.h>
#include <clib/amat4.h>
#include <clib/aquat.h>
#include <clib/aquat.h>
#include <clib/aquatrot.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/cull.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <clib/wavsound.h>
#include <clib/timemeas.h>
#include <clib/sound/wavemixer.h>
}
#include <cpplib/vec3.h>
#include <cpplib/vec4.h>
#include <cpplib/quat.h>
#include <cpplib/dstring.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <fstream>
#include <sstream>
#define exit something_meanless
/*#include <windows.h>*/
#undef exit
#include <GL/glext.h>

#define DELETEKEY 0x7f
#define ESC 0x1b

static HWND hWnd;

static aquat_t viewrot = {0,0,0,1};
static double dist = 1000.;
static int selectbone = -1;
static int boneview = 0;
static double translate_unit = .1;
static struct ysdnm_motion *motions[16] = {NULL}, *&motion = motions[0];
int nmotions = 0, curmotion = 0;
static double motion_time[16] = {0.};

static int g_width, g_height, g_max;
static ysdnm_t *dnm;
static ysdnmv_t *dnmv = NULL;
//static suf_t **sufs = NULL;
//static char **names = NULL;
//static Bone **bones = NULL;
//static int nsufs = 0;
static Model *model = NULL;

static HWND hList = NULL;
static HWND hListMotion = NULL;
static HWND hListKeyframe = NULL;
static HWND hEditMoveUnit = NULL;


static const char *GetSrfName(int boneid);
static void UpdateMotionList();


inline void interp(){
//	YSDNM_MotionInterpolateFree(dnmv);
	dnmv = YSDNM_MotionInterpolate(motions, motion_time, nmotions);
}

static void bone_ysdnm_node(ysdnm_t *dnm, struct ysdnm_srf *srf, ysdnmv_t *v0, const Mat4d &pmat){
	int i;
	ysdnmv_t *v;
	for(v = v0; v; v = v->next){
		const char **skipnames = v->skipnames;
		int skips = v->skips;
		for(i = 0; i < skips; i++) if(!strcmp(skipnames[i], srf->name)){
			return;
		}
	}
	Mat4d mat = pmat;
	for(v = v0; v; v = v->next){
		ysdnm_bone_var *bonevar = v->bonevar;
		int bones = v->bones;
		for(i = 0; i < bones; i++) if(!strcmp(bonevar[i].name, srf->name)){
			Mat4d rotmat = bonevar[i].rot.tomat4();
	/*		MAT4TRANSLATE(srf->pos[0], srf->pos[1], srf->pos[2]);*/
			Mat4d amat = mat;
			amat.translatein(srf->pos);
			amat.translatein(bonevar[i].pos);
			Mat4d rmat = amat * rotmat;
			rmat.translatein(-srf->pos);
			mat = rmat;
		}
	#if 0
		if(v->fcla & (1 << srf->cla) && 2 <= srf->nst){
			double f = v->cla[srf->cla];
			avec3_t pos;
			gldTranslate3dv(srf->pos);
			for(i = 0; i < 3; i++)
				pos[i] = srf->sta[1][i] * f + srf->sta[0][i] * (1. - f);
			gldTranslate3dv(pos);
			glRotated(srf->sta[1][3] * f + srf->sta[0][3] * (1. - f), 0, -1, 0); /* heading */
			glRotated(srf->sta[1][4] * f + srf->sta[0][4] * (1. - f), -1, 0, 0); /* pitch */
			glRotated(srf->sta[1][5] * f + srf->sta[0][5] * (1. - f), 0, 0, 1); /* bank */
			gldTranslaten3dv(srf->pos);
		}
	#endif
	}
	VECNULL(&mat[12]);
	mat4translate3dv(mat, srf->pos);
	for(i = 0; i < dnm->ns; i++) if(dnm->s[selectbone] == srf)
		break;
	if(i != dnm->ns)
		glColor4ub(255,255,255,255);
	else
		glColor4ub(0,255,0,255);
	glBegin(GL_LINES);
	glVertex3dv(pmat.vec3(3));
	glVertex3dv(mat.vec3(3));
	glEnd();
//	callback(srf->name, hint);
//	mat4translaten3dv(mat, srf->pos);
	for(i = 0; i < srf->nch; i++){
		if(srf->cld[i].srf){
			bone_ysdnm_node(dnm, srf->cld[i].srf, v0, mat);
		}
	}
}

void BoneYSDNM_V(ysdnm_t *dnm, ysdnmv_t *dnmv){
	if(!dnm->ns)
		return;
	bone_ysdnm_node(dnm, dnm->s[0], dnmv, mat4_u);
}




static ysdnm_bone_var *AddBone(int boneid){
	int i;
	const char *name = GetSrfName(selectbone);
	if(!name)
		return NULL;
	ysdnmv_t *dnmv = &motion->getKeyframe(motion_time[curmotion]);
	if(!dnmv){
		dnmv = new ysdnmv_t;
	}
	for(i = 0; i < dnmv->bones; i++) if(!strcmp(dnmv->bonevar[i].name, name)){
		return &dnmv->bonevar[i];
	}
	return dnmv->addBone(name);
}

static bool DelBone(int boneid){
	int i;
	ysdnmv_t *dnmv = &motion->getKeyframe(motion_time[curmotion]);
	const char *name;
	if(dnm)
		name = 0 <= selectbone && selectbone < dnm->ns && dnm->s[selectbone] ? dnm->s[boneid]->name : NULL;
	else if(model)
		name = 0 <= selectbone && selectbone < model->n && model->bones[selectbone] ? model->bones[selectbone]->name : NULL;
	if(!name || !dnmv)
		return false;
	for(i = 0; i < dnmv->bones; i++) if(!strcmp(dnmv->bonevar[i].name, name)){
		delete[] dnmv->bonevar[i].name;
		memmove(&dnmv->bonevar[i], &dnmv->bonevar[i+1], (dnmv->bones - i - 1) * sizeof *dnmv->bonevar);
		dnmv->bones--;
		return true;
	}
	return false;
}

static const double *FindBone(int boneid){
	const char *name = 0 <= selectbone && selectbone < dnm->ns && dnm->s[selectbone] ? dnm->s[boneid]->name : NULL;
	if(!name || !dnmv)
		return NULL;
	for(int i = 0; i < dnmv->bones; i++) if(!strcmp(dnmv->bonevar[i].name, name)){
		return dnmv->bonevar[i].rot;
	}
	return NULL;
}

static const char *GetSrfName(int boneid){
	if(dnm)
		return 0 <= selectbone && selectbone < dnm->ns && dnm->s[selectbone] ? dnm->s[boneid]->name : NULL;
	else if(model)
		return 0 <= selectbone && selectbone < model->n && model->bones[selectbone]->name ? model->bones[selectbone]->name : NULL;
	else
		return NULL;
}

struct Pose{

};

static void bone_mqo_node(suf_t **sufs, Bone **bones, ysdnmv_t *v0, const Mat4d &pmat){
	int i;
	ysdnmv_t *v;
/*	for(v = v0; v; v = v->next){
		const char **skipnames = v->skipnames;
		int skips = v->skips;
		for(i = 0; i < skips; i++) if(!strcmp(skipnames[i], srf->name)){
			return;
		}
	}*/
	Mat4d mat = pmat;
	for(v = v0; v; v = v->next){
		ysdnm_bone_var *bonevar = v->bonevar;
		int nbones = v->bones;
		for(i = 0; i < nbones; i++) if(!strcmp(bonevar[i].name, bones[i]->name)){
			Mat4d rotmat = bonevar[i].rot.tomat4();
	/*		MAT4TRANSLATE(srf->pos[0], srf->pos[1], srf->pos[2]);*/
			Mat4d amat = mat;
			amat.translatein(bones[i]->joint);
			amat.translatein(bonevar[i].pos);
			Mat4d rmat = amat * rotmat;
			rmat.translatein(-bones[i]->joint);
			mat = rmat;
		}
	#if 0
		if(v->fcla & (1 << srf->cla) && 2 <= srf->nst){
			double f = v->cla[srf->cla];
			avec3_t pos;
			gldTranslate3dv(srf->pos);
			for(i = 0; i < 3; i++)
				pos[i] = srf->sta[1][i] * f + srf->sta[0][i] * (1. - f);
			gldTranslate3dv(pos);
			glRotated(srf->sta[1][3] * f + srf->sta[0][3] * (1. - f), 0, -1, 0); /* heading */
			glRotated(srf->sta[1][4] * f + srf->sta[0][4] * (1. - f), -1, 0, 0); /* pitch */
			glRotated(srf->sta[1][5] * f + srf->sta[0][5] * (1. - f), 0, 0, 1); /* bank */
			gldTranslaten3dv(srf->pos);
		}
	#endif
	}
//	VECNULL(&mat[12]);
//	mat4translate3dv(mat, bones[i]->joint);
//	for(i = 0; i < dnm->ns; i++) if(dnm->s[selectbone] == srf)
//		break;
/*	if(i != dnm->ns)
		glColor4ub(255,255,255,255);
	else
		glColor4ub(0,255,0,255);*/
/*	glBegin(GL_LINES);
	glVertex3dv(&pmat[12]);
	glVertex3dv(&mat[12]);
	glEnd();*/
//	callback(srf->name, hint);
//	mat4translaten3dv(mat, srf->pos);
/*	for(i = 0; i < srf->nch; i++){
		if(srf->cld[i].srf){
			bone_ysdnm_node(dnm, srf->cld[i].srf, v0, mat);
		}
	}*/
}

void BoneMQO_V(suf_t **sufs, Bone **bones, ysdnmv_t *dnmv){
	bone_mqo_node(sufs, bones, dnmv, mat4_u);
}






















void display_func(void){
	static bool init = false;
	static timemeas_t tm;
	static double realtime = 0.;
	if(!init){
		init = true;
		TimeMeasStart(&tm);
	}

	double t1 = TimeMeasLap(&tm);
	double rdt, dt;
/*	if(g_fix_dt)
		rdt = g_fix_dt;
	else*/
		rdt = (t1 - realtime);

	realtime = t1;

	dt = !init ? 0. : rdt < 1. ? rdt : 1.;

	if(dnmv){
		if(fmod(realtime, .2) < .1){
			dnmv->target = GetSrfName(selectbone);
		}
		else
			dnmv->target = NULL;
	}

	glClearColor(0,0,0,0);
	glClearDepth(1.);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();
	glTranslated(0,0,-dist);
	gldMultQuat(viewrot);

	/* grid draw */
	glPushMatrix();
	gldScaled(100.);
	glBegin(GL_LINES);
	for(int i = 0; i <= 16; i++){
		glColor4ub(255,127,127,255);
		glVertex3d(-8, 0, i - 8);
		glVertex3d( 8, 0, i - 8);
		glColor4ub(127,127,255,255);
		glVertex3d(i - 8, 0, -8);
		glVertex3d(i - 8, 0,  8);
	}
	glColor4ub(255,0,0,255);
	glVertex3d(-100,0,0);
	glVertex3d( 100,0,0);
	glColor4ub(0,255,0,255);
	glVertex3d(0,-100,0);
	glVertex3d(0, 100,0);
	glColor4ub(0,0,255,255);
	glVertex3d(0,0,-100);
	glVertex3d(0,0, 100);
	glEnd();
	glPopMatrix();

	if(dnm){
		glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		{
			int i;
			GLfloat mat_specular[] = {0., 0., 0., 1.};
			GLfloat mat_diffuse[] = { .5, .5, .5, 1.0 };
			GLfloat mat_ambient_color[] = { 0.5, 0.5, 0.5, 1.0 };
			GLfloat mat_shininess[] = { 50.0 };
			GLfloat light_position[4] = { 1, 1, 1, 0.0 };
			GLfloat light_ambient_color[] = { 0.5, 0.5, 0.5, 1.0 };
			GLfloat light_diffuse[] = { 1., 1., 1., 1.0 };

			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
			glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);
			glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
			glLightfv(GL_LIGHT0, GL_POSITION, light_position);
			glLightfv(GL_LIGHT0, GL_SPECULAR, mat_specular);
			glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient_color);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		}
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_NORMALIZE);
		glEnable(GL_CULL_FACE);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		DrawYSDNM_V(dnm, dnmv);
		glPopAttrib();

		if(boneview){
			if(boneview == 2)
				glDisable(GL_DEPTH_TEST);
			glPushMatrix();
			BoneYSDNM_V(dnm, dnmv);
			glPopMatrix();
		}
	}
	else if(model){
		glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		{
			int i;
			GLfloat mat_specular[] = {0., 0., 0., 1.};
			GLfloat mat_diffuse[] = { .5, .5, .5, 1.0 };
			GLfloat mat_ambient_color[] = { 0.5, 0.5, 0.5, 1.0 };
			GLfloat mat_shininess[] = { 50.0 };
			GLfloat light_position[4] = { 1, 1, 1, 0.0 };
			GLfloat light_ambient_color[] = { 0.5, 0.5, 0.5, 1.0 };
			GLfloat light_diffuse[] = { 1., 1., 1., 1.0 };

			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
			glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);
			glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
			glLightfv(GL_LIGHT0, GL_POSITION, light_position);
			glLightfv(GL_LIGHT0, GL_SPECULAR, mat_specular);
			glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient_color);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		}
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_NORMALIZE);
		glEnable(GL_CULL_FACE);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

//		for(int i = 0; i < nsufs; i++){
//			DrawSUF(sufs[i], SUF_ATR, NULL);
//		}
//		BoneMQO_V(sufs, bones, dnmv);
		DrawMQO_V(model, dnmv);
		glPopAttrib();

/*		if(boneview){
			if(boneview == 2)
				glDisable(GL_DEPTH_TEST);
			glPushMatrix();
			glBegin(GL_LINES);
			for(int i = 0; i < nsufs; i++){
				glVertex3dv(bones[i]->parent ? bones[i]->parent->joint: Vec4d(0,0,0,0));
				glVertex3dv(bones[i]->joint);
			}
			glEnd();
			glPopMatrix();
		}*/
	}

	if(dnm){

		glDisable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);

		glLoadIdentity();
		glTranslated(0,0,-.5);

		glScaled(2. / g_width, 2. / g_height, 1.);
		glColor4ub(0,255,0,255);
		glRasterPos2d(-g_width / 2, -g_height / 2 + 32);
		gldprintf("bone: %d", selectbone);
		if(0 <= selectbone && selectbone < dnm->ns && dnm->s[selectbone]){
			glRasterPos2d(-g_width / 2, -g_height / 2 + 16);
			gldprintf("name: \"%s\" \"%s\"", dnm->s[selectbone]->name, dnm->s[selectbone]->pck->name);
		}
		const double (*pq)[7];
		if(pq = (double (*)[7])FindBone(selectbone)){
			glRasterPos2d(-g_width / 2, -g_height / 2 + 0);
			gldprintf("rot: (%lg,%lg,%lg,%lg),(%lg,%lg,%lg)", (*pq)[0], (*pq)[1], (*pq)[2], (*pq)[3], (*pq)[4], (*pq)[5], (*pq)[6]);
		}
		if(motion && motion->nkfl){
			glRasterPos2d(-g_width / 2, -g_height * 2 / 3);
			gldprintf("mot: (%d)", &motion->getKeyframe(motion_time[curmotion]) - motion->kfl);
		}

		for(int n = 0; n < nmotions; n++) if(motions[n] && motions[n]->nkfl){
			ysdnm_motion *mot = motions[n];
			double ttime = mot->totalTime();
			glColor4ub(0,n == curmotion ? 255 : 127,0,255);
			glBegin(GL_LINE_LOOP);
			glVertex2d(-g_width / 2 * .8, -g_height / 2 * (.7 + .1 * n));
			glVertex2d( g_width / 2 * .8, -g_height / 2 * (.7 + .1 * n));
			glVertex2d( g_width / 2 * .8, -g_height / 2 * (.6 + .1 * n));
			glVertex2d(-g_width / 2 * .8, -g_height / 2 * (.6 + .1 * n));
			glEnd();
			glBegin(GL_LINES);
			double tsum = 0.;
			for(int i = 0; i < mot->nkfl; i++){
				tsum += mot->kfl[i].dt;
				glVertex2d(-g_width / 2 * .8 + tsum * (g_width / 2 * 1.6) / ttime, -g_height / 2 * (.7 + .1 * n));
				glVertex2d(-g_width / 2 * .8 + tsum * (g_width / 2 * 1.6) / ttime, -g_height / 2 * (.6 + .1 * n));
			}
			glColor4ub(255,255,0,255);
			glVertex2d(-g_width / 2 * .8 + motion_time[n] * (g_width / 2 * 1.6) / ttime, -g_height / 2 * (.7 + .1 * n));
			glVertex2d(-g_width / 2 * .8 + motion_time[n] * (g_width / 2 * 1.6) / ttime, -g_height / 2 * (.6 + .1 * n));
			glEnd();
		}

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);

	}
}

#if !USEWIN
void idle(void){
	glutPostRedisplay();
}
#endif

/*
 *	ウインドウサイズ更新時の処理
 */
void reshape_func(int w, int h)
{
	int m = w < h ? h : w;
	glViewport(0, 0, w, h);
	g_width = w;
	g_height = h;
	g_max = m;

#if 0

#elif 1
  /* 変換行列の初期化 */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  /* スクリーン上の表示領域をビューポートの大きさに比例させる */
/*  glOrtho(-w / 200.0, w / 200.0, -h / 200.0, h / 200.0, -1.0, 1.0);*/

  gluPerspective(30.0, (double)w / (double)h, .10, 100000.0);
  glTranslated(0.0, 0.0, -5.0);
   glMatrixMode(GL_MODELVIEW);
 
#else

  /* 透視変換行列の設定 */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
 /* glTranslated(0.0, 0.0, -50.0);*/
  gluPerspective(30.0, (double)w / (double)h, 1.0, 100.0);

  /* モデルビュー変換行列の設定 */
  glMatrixMode(GL_MODELVIEW);
#endif
}

static void Rote(ysdnm_bone_var *aq, const Vec3d &axis, double angle){
	if(!aq)
		return;
	aq->rot = aq->rot.rotate(angle, axis);
//	quatrotquat(aq, axis.scale(angle), aq);
	interp();
	UpdateMotionList();
}

static void Ofs(ysdnm_bone_var *aq, const Vec3d &dir, double len){
	if(!aq)
		return;
	aq->pos.addin(dir.scale(len));
	interp();
	UpdateMotionList();
}

static void UpdateMotionList(){
	SendMessage(hListMotion, LB_RESETCONTENT, 0, 0);
	if(motions[curmotion]) for(int i = 0; i < motions[curmotion]->nkfl; i++){
		char buf[128];
		sprintf(buf, "Keyframe %g", motions[curmotion]->kfl[i].dt);
		SendMessage(hListMotion, LB_ADDSTRING, 0, (LPARAM)buf);
	}
	SendMessage(hListMotion, LB_SETCURSEL, motions[curmotion]->getKeyframeIndex(motion_time[curmotion]), 0);

	SendMessage(hListKeyframe, LB_RESETCONTENT, 0, 0);
	if(dnmv/*motions[curmotion]*/){
		ysdnm_var &v = *dnmv/*motions[curmotion]->getKeyframe(motion_time[curmotion])*/;
		for(int i = 0; i < v.bones; i++){
			ysdnm_bone_var &bv = v.bonevar[i];
			char buf[128];
			sprintf(buf, "\"%s\" [%g,%g,%g,%g][%g,%g,%g][%g]", bv.name,
				bv.rot[0], bv.rot[1], bv.rot[2], bv.rot[3],
				bv.pos[0], bv.pos[1], bv.pos[2],
				bv.visible);
			SendMessage(hListKeyframe, LB_ADDSTRING, 0, (LPARAM)buf);
		}
	}
}

static void key_func(unsigned char key, int x, int y){
	static double rotyank[7];
	static bool rotyank_ok = false;
	switch(key){
		case '+':
			selectbone++;
			SendMessage(hList, LB_SETCURSEL, selectbone, 0);
			break;
		case '-':
			selectbone--;
			SendMessage(hList, LB_SETCURSEL, selectbone, 0);
			break;
		case '8':
			Rote(AddBone(selectbone), vec3_100, M_PI * .01);
			break;
		case '2':
			Rote(AddBone(selectbone), vec3_100, M_PI * -.01);
			break;
		case '4':
			Rote(AddBone(selectbone), vec3_010, M_PI * .01);
			break;
		case '6':
			Rote(AddBone(selectbone), vec3_010, M_PI * -.01);
			break;
		case '7':
			Rote(AddBone(selectbone), vec3_001, M_PI * .01);
			break;
		case '9':
			Rote(AddBone(selectbone), vec3_001, M_PI * -.01);
			break;
		case 'w':
			Ofs(AddBone(selectbone), vec3_001, translate_unit);
			break;
		case 's':
			Ofs(AddBone(selectbone), vec3_001, -translate_unit);
			break;
		case 'a':
			Ofs(AddBone(selectbone), vec3_100, translate_unit);
			break;
		case 'd':
			Ofs(AddBone(selectbone), vec3_100, -translate_unit);
			break;
		case 'q':
			Ofs(AddBone(selectbone), vec3_010, translate_unit);
			break;
		case 'z':
			Ofs(AddBone(selectbone), vec3_010, -translate_unit);
			break;
		case 'r':
			DelBone(selectbone);
			break;
		case 'b':
			boneview = (boneview + 1) % 3;
			break;
		case 'g':
			if(!nmotions)
				motions[nmotions++] = new ysdnm_motion();
			YSDNM_MotionAddKeyframe(motions[curmotion], 10.);
			UpdateMotionList();
			break;
		case 't':
			if(curmotion + 1 < nmotions)
				curmotion++;
			UpdateMotionList();
			break;
		case 'y':
			if(0 <= curmotion - 1)
				curmotion--;
			UpdateMotionList();
			break;
		case '.':
			motion_time[curmotion]++;
			interp();
			UpdateMotionList();
			break;
		case ',':
			motion_time[curmotion]--;
			interp();
			UpdateMotionList();
			break;
		case 'o':
			{
				static const char filter[] = "YSFlight DNM Motion File (*.mot)\0*.mot\0";
				char buf[MAX_PATH] = "motion.mot";
				OPENFILENAME ofn = {sizeof(OPENFILENAME), hWnd, 0, filter,
					NULL, 0, 0, buf, sizeof buf, NULL, 0, NULL, NULL, OFN_OVERWRITEPROMPT/*OFN_DONTADDTORECENT*/,
				};
				if(GetSaveFileName(&ofn))
					YSDNM_MotionSave(ofn.lpstrFile, motions[curmotion]);
			}
			break;
		case 'p':
			{
				static const char filter[] = "YSFlight DNM Motion File (*.mot)\0*.mot\0";
				char buf[MAX_PATH] = "motion2.mot";
				OPENFILENAME ofn = {sizeof(OPENFILENAME), hWnd, 0, filter,
					NULL, 0, 0, buf, sizeof buf, NULL, 0, NULL, NULL, OFN_OVERWRITEPROMPT/*OFN_DONTADDTORECENT*/,
				};
				if(GetSaveFileName(&ofn))
					motions[curmotion]->save2(std::ofstream(ofn.lpstrFile));
			}
			break;
		case 'l': case 'k':
			{
				static const char filter[] = "YSFlight DNM Motion File (*.mot)\0*.mot\0";
				char buf[MAX_PATH] = "motion.mot";
				OPENFILENAME ofn = {sizeof(OPENFILENAME), hWnd, 0, filter,
					NULL, 0, 0, buf, sizeof buf, NULL, 0, NULL, NULL, 0/*OFN_DONTADDTORECENT*/,
				};
				if(GetOpenFileName(&ofn)){
					if(key == 'l'){
						motion = YSDNM_MotionLoad(ofn.lpstrFile);
						nmotions = 1;
					}
					else{
						motions[nmotions++] = YSDNM_MotionLoad(ofn.lpstrFile);
					}
					interp();
					UpdateMotionList();
				}
			}
			break;

		case';':
			{
				static const char filter[] = "Metasequoia Object (*.mqo)\0*.mqo\0";
				static char buf[MAX_PATH] = "a.mqo";
				OPENFILENAME ofn = {sizeof(OPENFILENAME), hWnd, 0, filter,
					NULL, 0, 0, buf, sizeof buf, NULL, 0, NULL, NULL, OFN_OVERWRITEPROMPT/*OFN_DONTADDTORECENT*/,
				};
				if(GetOpenFileName(&ofn)){
					SendMessage(hList, LB_RESETCONTENT, 0, 0);
//					nsufs = LoadMQO_Scale(ofn.lpstrFile, &sufs, &names, .1, &bones);
					model = LoadMQOModel(ofn.lpstrFile, 1., NULL);
					dnmv = new ysdnm_var;
					dnmv->bones = model->n;
					dnmv->bonevar = (ysdnm_bone_var *)malloc(model->n * sizeof(*dnmv->bonevar));
					for(int i = 0; i < model->n; i++){
						printf("bone %d: depth %d, parent %p\n", i, model->bones[i]->depth, model->bones[i]->parent);
						char buf[128];
						sprintf(buf, "%*.*c%s", model->bones[i]->depth + 1, model->bones[i]->depth + 1, ' ', model->bones[i]->name);
						SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)buf);
						char *newname = new char[::strlen(model->bones[i]->name)+1];
						::strcpy(newname, model->bones[i]->name);
						dnmv->bonevar[i].name = newname;
						dnmv->bonevar[i].rot = quat_u;
						dnmv->bonevar[i].pos.clear();
						dnmv->bonevar[i].visible = 1.;
					}
				}
			}
			break;

		case 'j':
			motions[nmotions++] = new ysdnm_motion;
			interp();
			UpdateMotionList();
			break;
		case 'c':
			{
				const double (*pq)[7];
				if(pq = (double (*)[7])FindBone(selectbone)){
					::memcpy(rotyank, *pq, sizeof rotyank);
					rotyank_ok = true;
				}
			}
			break;
		case 'v':
			if(rotyank_ok){
				double (*pq)[7];
				if(pq = (double (*)[7])AddBone(selectbone)){
					::memcpy(pq, rotyank, sizeof rotyank);
				}
			}
			break;
	}
}


#if (1 || !USEWIN) && defined _WIN32
void mouse_func(int button, int state, int x, int y){
	if(button == GLUT_WHEEL_UP)
		dist /= 1.1;
	else if(button == GLUT_WHEEL_DOWN)
		dist *= 1.1;
	return;
	aquat_t qx;
	qx[0] = 0;
	qx[1] = sin((x - g_width / 2) * M_PI / 2 / g_width);
	qx[2] = 0;
	qx[3] = cos((x - g_width / 2) * M_PI / 2 / g_width);
	aquat_t qr;
	QUATMUL(qr, viewrot, qx);
	QUATCPY(viewrot, qr);
	aquat_t qy = {
		sin((y - g_height / 2) * M_PI / 2 / g_height),
		0,
		0,
		cos((y - g_height / 2) * M_PI / 2 / g_height),
	};
	QUATMUL(qr, viewrot, qy);
	QUATCPY(viewrot, qr);
}
#endif


#if USEWIN
#ifdef _WIN32
static HGLRC wingl(HWND hWnd, HDC *phdc){
	PIXELFORMATDESCRIPTOR pfd = { 
		sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
		1,                     // version number 
		PFD_DRAW_TO_WINDOW |   // support window 
		PFD_SUPPORT_OPENGL |   // support OpenGL 
		PFD_DOUBLEBUFFER,      // double buffered 
		PFD_TYPE_RGBA,         // RGBA type 
		24,                    // 24-bit color depth 
		0, 0, 0, 0, 0, 0,      // color bits ignored 
		0,                     // no alpha buffer 
		0,                     // shift bit ignored 
		0,                     // no accumulation buffer 
		0, 0, 0, 0,            // accum bits ignored 
		32,                    // 32-bit z-buffer 
		0,                     // no stencil buffer 
		0,                     // no auxiliary buffer 
		PFD_MAIN_PLANE,        // main layer 
		0,                     // reserved 
		0, 0, 0                // layer masks ignored 
	}; 
	int  iPixelFormat; 
	HGLRC hgl;
	HDC hdc;

	hdc = GetDC(hWnd);

	// get the best available match of pixel format for the device context  
	iPixelFormat = ChoosePixelFormat(hdc, &pfd); 
		
	// make that the pixel format of the device context 
	SetPixelFormat(hdc, iPixelFormat, &pfd);

	hgl = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hgl);

	*phdc = hdc;

	return hgl;
}

static HGLRC winglstart(HDC hdc){
	PIXELFORMATDESCRIPTOR pfd = { 
		sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
		1,                     // version number 
		PFD_DRAW_TO_WINDOW |   // support window 
		PFD_SUPPORT_OPENGL |   // support OpenGL 
		PFD_DOUBLEBUFFER,      // double buffered 
		PFD_TYPE_RGBA,         // RGBA type 
		24,                    // 24-bit color depth 
		0, 0, 0, 0, 0, 0,      // color bits ignored 
		0,                     // no alpha buffer 
		0,                     // shift bit ignored 
		0,                     // no accumulation buffer 
		0, 0, 0, 0,            // accum bits ignored 
		32,                    // 32-bit z-buffer 
		0,                     // no stencil buffer 
		0,                     // no auxiliary buffer 
		PFD_MAIN_PLANE,        // main layer 
		0,                     // reserved 
		0, 0, 0                // layer masks ignored 
	}; 
	int  iPixelFormat; 
	HGLRC hgl;

	// get the best available match of pixel format for the device context  
	iPixelFormat = ChoosePixelFormat(hdc, &pfd); 
		
	// make that the pixel format of the device context 
	SetPixelFormat(hdc, iPixelFormat, &pfd);

	hgl = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hgl);

	return hgl;
}

static void winglend(HGLRC hgl){
	wglMakeCurrent (NULL, NULL) ;
	if(!wglDeleteContext(hgl))
		fprintf(stderr, "eeee!\n");
}

static LRESULT WINAPI CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	static HDC hdc;
	static HGLRC hgl = NULL;
	static int moc = 0;
	static POINT lastcursor;
	switch(message){
		case WM_CREATE:
			hgl = wingl(hWnd, &hdc);
			if(!SetTimer(hWnd, 2, 10, NULL))
				assert(0);
			hList = CreateWindow("ListBox", "boneList", WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
				100+512+12, 100, 250, 500, hWnd, NULL, GetModuleHandle(NULL), NULL);
			hListMotion = CreateWindow("ListBox", "motionList", WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
				100+512+12+250+10, 100, 150, 500, hWnd, NULL, GetModuleHandle(NULL), NULL);
			hListKeyframe = CreateWindow("ListBox", "keyframeList", WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
				100+512+12+250+10+150+10, 100, 500, 500, hWnd, NULL, GetModuleHandle(NULL), NULL);
			{
				RECT r = {100+512+12, 720, 100+512+12+250, 720+50};
				AdjustWindowRect(&r, WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE, FALSE);
				hEditMoveUnit = CreateWindow("Edit", "moveUnit", WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
					r.left, r.top, r.right - r.left, r.bottom - r.top, hWnd, NULL, GetModuleHandle(NULL), NULL);
				SetWindowText(hEditMoveUnit, cpplib::dstring(translate_unit));
			}
			break;

		case WM_COMMAND:
			if(lParam == LPARAM(hEditMoveUnit) && HIWORD(wParam) == EN_CHANGE){
				char buf[64];
				if(GetWindowText(hEditMoveUnit, buf, sizeof buf))
					translate_unit = atof(buf);
			}
			break;

		case WM_SIZE:
			if(hgl){
				reshape_func(LOWORD(lParam), HIWORD(lParam));
			}
			break;

		case WM_TIMER:
			if(hgl){
				HDC hdc;
//				HGLRC hgl;
				hdc = GetDC(hWnd);
//				hgl = winglstart(hdc);
				display_func();
				wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE);
//				winglend(hgl);
				ReleaseDC(hWnd, hdc);
			}
			break;
		case WM_RBUTTONDOWN:
			mouse_func(GLUT_RIGHT_BUTTON, GLUT_DOWN, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_LBUTTONDOWN:
			mouse_func(GLUT_LEFT_BUTTON, GLUT_DOWN, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_RBUTTONUP:
			mouse_func(GLUT_RIGHT_BUTTON, GLUT_UP, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_LBUTTONUP:
			mouse_func(GLUT_LEFT_BUTTON, GLUT_UP, LOWORD(lParam), HIWORD(lParam));
			break;
/*			moc = !moc;
			if(moc){
				RECT r;
				SetCapture(hWnd);
		//		ShowCursor(FALSE);
				GetWindowRect(hWnd, &r);
				ClipCursor(&r);
			}
			else{
				ReleaseCapture();
				ShowCursor(TRUE);
				ClipCursor(NULL);
			}
			break;*/
		case WM_MOUSEMOVE:
			if(wParam & MK_LBUTTON){
				long dx = LOWORD(lParam) - lastcursor.x;
				long dy = HIWORD(lParam) - lastcursor.y;

				aquat_t iviewrot;
				QUATCNJ(iviewrot, viewrot);
				Quat<double> qx;
				quatrot(qx, iviewrot, avec3_010);
/*				qx[0] = 0;
				qx[1] = sin(dx * M_PI / 2 / g_width);
				qx[2] = 0;*/
				VECSCALEIN(qx, sin(dx * M_PI / 2 / g_width));
				qx[3] = cos(dx * M_PI / 2 / g_width);
				aquat_t qr;
				QUATMUL(qr, viewrot, qx);
				QUATCPY(viewrot, qr);
				Quat<double> qy;
				quatrot(qy, iviewrot, avec3_100);
				((Vec3<double>&)qy).scalein(sin(dy * M_PI / 2 / g_height));
				qy[3] = cos(dy * M_PI / 2 / g_height);
				QUATMUL(qr, viewrot, qy);
				QUATCPY(viewrot, qr);

			}
			lastcursor.x = LOWORD(lParam);
			lastcursor.y = HIWORD(lParam);
/*			if(!mouse_captured){
				s_mousex = LOWORD(lParam);
				s_mousey = HIWORD(lParam);
				if(glwfocus && glwfocus->mouse){
					glwfocus->mouse(glwfocus, GLUT_LEFT_BUTTON, wParam & MK_LBUTTON ? GLUT_KEEP_DOWN : GLUT_KEEP_UP, s_mousex - glwfocus->x, s_mousey - glwfocus->y - 12);
				}
			}*/
#if 0
			else{
				POINT p;
				int rotmode;
				rotmode = pl.chase ? 3 : pl.height < 10. ? 1 : pl.cs == &spacestation || pl.cs == &spacecolony ? 2 : 0;
				p.x = LOWORD(lParam);
				p.y = HIWORD(lParam);
				if(pl.cs->flags & CS_PYR){
					pl.pyr[1] += (p.x - mouse_pos.x) * 0.001;
					pl.pyr[0] += (p.y - mouse_pos.y) * 0.001;
				}
				else{
					aquat_t q, qr, qc;
					double srad;
					if(rotmode == 2){
						srad = pl.pos[0] * pl.pos[0] + pl.pos[2] * pl.pos[2];
						if(pl.cs == &spacecolony && (srad < 2. * 2. || 3.25 * 3.25 < srad))
							rotmode = 0;
					}
					if(p.x != mouse_pos.x){
						if(rotmode == 3){
							VECCPY(q, avec3_010);
						}
						else if(rotmode == 2){
							double rad;
							rad = sqrt(srad);
							q[0] = -pl.pos[0] / rad;
							q[1] = 0.;
							q[2] = -pl.pos[2] / rad;
						}
						else if(rotmode){
							avec3_t epos;
							tocs(epos, pl.cs, earth.pos, earth.cs);
							VECSUB(epos, pl.pos, epos);
							VECNORM(q, epos);
/*								quatrot(q, pl.rot, epos);*/
						}
						else
							quatrot(q, pl.rot, avec3_010);
						VECSCALEIN(q, -(p.x - mouse_pos.x) * .001 / 2.);
						q[3] = 0.;
						quatrotquat(pl.rot, q, pl.rot);
					}
					if(p.y != mouse_pos.y){
						if(rotmode == 3){
							avec3_t epos, nh;
							quatrot(nh, pl.rot, avec3_001);
							VECVP(q, avec3_010, nh);
							VECNORMIN(q);
						}
						else if(rotmode == 2){
							double sp, rad;
							avec3_t xh, nh;
							quatrot(xh, pl.rot, avec3_100);
							rad = sqrt(srad);
							nh[0] = -pl.pos[0] / rad;
							nh[1] = 0.;
							nh[2] = -pl.pos[2] / rad;
							sp = VECSP(nh, xh);
							VECSADD(xh, nh, -sp);
							VECNORM(q, xh);
						}
						else if(rotmode){
							avec3_t epos, nh;
							tocs(epos, pl.cs, earth.pos, earth.cs);
							VECSUB(epos, pl.pos, epos);
							quatrot(nh, pl.rot, avec3_001);
							VECVP(q, epos, nh);
							VECNORMIN(q);
						}
						else
							quatrot(q, pl.rot, avec3_100);
						VECSCALEIN(q, -(p.y - mouse_pos.y) * .001 / 2.);
						q[3] = 0.;
						quatrotquat(pl.rot, q, pl.rot);
					}
				}
				SetCursorPos(mouse_pos.x, mouse_pos.y);
			}
#endif
/*			if(glwdrag){
				glwdrag->x = LOWORD(lParam) - glwdragpos[0];
				glwdrag->y = HIWORD(lParam) - glwdragpos[1];
			}*/
#if 0
			if(moc){
				RECT r, rc, rd;
				POINT p;
				int dx, dy;
				GetWindowRect(hWnd, &r);
				GetClientRect(hWnd, &rc);
				GetWindowRect(GetDesktopWindow(), &rd);
				pl.pyr[1] += (dx = LOWORD(lParam) - (rc.left + rc.right) / 2) * .001;
				pl.pyr[0] += (dy = HIWORD(lParam) - (rc.top + rc.bottom) / 2) * .001;

				/* prevent infinite loop of messages. mouse_event is better in that
				  it doesn't send unnecessary WM_MOUSEMOVEs but some old Windows' may
				  not support it. */
				if(dx || dy){
					p.x = (rc.left + rc.right) / 2;
					p.y = (rc.top + rc.bottom) / 2;
					ClientToScreen(hWnd, &p);
	/*				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
						65536 * p.x / rd.right,
						65536 * p.y / rd.bottom, 0, NULL);*/
					SetCursorPos(p.x, p.y);
					fprintf(stderr, "dx = %d, dy = %d\n", dx, dy);
				}
			}
#endif
			break;
		case WM_MOUSEWHEEL:
			mouse_func((short)HIWORD(wParam) < 0 ? GLUT_WHEEL_DOWN : GLUT_WHEEL_UP, GLUT_UP, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_CHAR:
			key_func((unsigned char)wParam, 0, 0);
			break;

		/* most inputs from keyboard is processed through WM_CHAR, but some special keys are
		  not sent as characters. */
		case WM_KEYDOWN:
#if 0
			switch(wParam){
			case VK_DELETE: key_func(DELETEKEY, 0, 0); break;
			case VK_ESCAPE: key_func(ESC, 0, 0); break;
			case VK_PRIOR: special_func(GLUT_KEY_PAGE_UP, 0, 0); break;
			case VK_NEXT: special_func(GLUT_KEY_PAGE_DOWN, 0, 0); break;
			case VK_HOME: special_func(GLUT_KEY_HOME, 0, 0); break;
			case VK_END: special_func(GLUT_KEY_END, 0, 0); break;
			case VK_INSERT: special_func(GLUT_KEY_INSERT, 0, 0); break;
			case VK_LEFT: special_func(GLUT_KEY_LEFT, 0, 0); break;
			case VK_RIGHT: special_func(GLUT_KEY_RIGHT, 0, 0); break;
			case VK_UP: special_func(GLUT_KEY_UP, 0, 0); break;
			case VK_DOWN: special_func(GLUT_KEY_DOWN, 0, 0); break;
/*			case VK_BACK: key_func(0x08, 0, 0);*/
			}
			if(VK_F1 <= wParam && wParam <= VK_F12)
				special_func(GLUT_KEY_F1 + wParam - VK_F1, 0, 0);
#endif
			break;

		/* technique to enable momentary key commands */
		case WM_KEYUP:
//			BindKeyUp(toupper(wParam));
			break;

		/* TODO: don't call every time the window defocus */
		case WM_KILLFOCUS:
//			BindKillFocus();
			break;

		case WM_DESTROY:
			KillTimer(hWnd, 2);

/*			if(moc){
				ReleaseCapture();
				ShowCursor(TRUE);
				ClipCursor(NULL);
			}*/

			PostQuitMessage(0);
			break;
		default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
#endif
#endif


/*
 *	main関数
 *		glutを使ってウインドウを作るなどの処理をする
 */
int main(int argc, char *argv[])
{
	int i;

	if(2 <= argc){
		dnm = LoadYSDNM(argv[1]);
	}
	else
		dnm = NULL;

#if !USEWIN
	/* glutの初期化 */
	glutInit(&argc, argv);

	/* 画面表示の設定 */
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE/* | GLUT_STENCIL*/);

	/* ウインドウの初期サイズを指定 */
	glutInitWindowSize(512,384);
/*	glutInitWindowSize(400,300);*/

	/* ウインドウを作る */
	glutCreateWindow("Sample 1");

	/* 画面更新用の関数を登録 */
	glutDisplayFunc(display_func);

	/* ウインドウのサイズ変更時の関数を登録 */
	glutReshapeFunc(reshape_func);

	/* input handlers */
	glutSpecialFunc(special_func);
	glutKeyboardFunc(key_func);
	glutMouseFunc(mouse_func);
	glutIdleFunc(idle);
#endif

#if USEWIN
	{
		MSG msg;
		ATOM atom;
		HINSTANCE hInst;
		RECT rc = {100,100,100+512,100+384};
/*		RECT rc = {0,0,0+512,0+384};*/
		hInst = GetModuleHandle(NULL);

		{
			WNDCLASSEX wcex;

			wcex.cbSize = sizeof(WNDCLASSEX); 

			wcex.style			= CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc	= (WNDPROC)WndProc;
			wcex.cbClsExtra		= 0;
			wcex.cbWndExtra		= 0;
			wcex.hInstance		= hInst;
			wcex.hIcon			= LoadIcon(hInst, IDI_APPLICATION);
			wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
			wcex.hbrBackground	= NULL;
			wcex.lpszMenuName	= NULL;
			wcex.lpszClassName	= "motioner";
			wcex.hIconSm		= LoadIcon(hInst, IDI_APPLICATION);

			atom = RegisterClassEx(&wcex);
		}


		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_CAPTION, FALSE);

		hWnd = CreateWindow(LPCTSTR(atom), "motioner", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);
/*		hWnd = CreateWindow(atom, "gltest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			100, 100, 400, 400, NULL, NULL, hInst, NULL);*/

		while (GetMessage(&msg, NULL, 0, 0)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
#else
/*	printf("%lg %lg %lg\n", fmod(-8., 5.), -8. / 5. - (int)(-8. / 5.), -8. / 5. - (floor)(-8. / 5.));*/
/*	display_func();*/
	glutMainLoop();
#endif

	return 0;
}

#ifdef _WIN32

HINSTANCE g_hInstance;

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, int nCmdShow) 
{
	g_hInstance = hinstance;
	return main(1, &lpCmdLine);
}
#endif

