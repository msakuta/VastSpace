// wtltest.cpp : main source file for wtltest.exe
//

#include "stdafx.h"

#include "motion.h"
#include "ysdnmmot.h"
#include "mqo.h"
#include "antiglut.h"
extern "C"{
#include <clib/c.h>
#include <clib/timemeas.h>
#include <clib/avec3.h>
#include <clib/aquat.h>
#include <clib/aquatrot.h>
#include <clib/amat4.h>
#include <clib/gl/gldraw.h>
}
//#include <cpplib/dstring.h>
#include <gl/gl.h>
#include <gl/glu.h>

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>

#include "resource.h"

#include "aboutdlg.h"
#include "PreviewDlg.h"
#include "MainDlg.h"

CAppModule _Module;

int Run(LPTSTR lpstrCmdLine = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainDlg dlgMain;

	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	if(dlgMain.openPreview() == NULL)
	{
		ATLTRACE(_T("Preview Window creation failed!\n"));
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);

	if(lpstrCmdLine){
		char buf[256];
		wcstombs(buf, lpstrCmdLine, sizeof buf);
		dlgMain.OpenModel(buf);
	}

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}

static void UpdateMotionList();

void CPreviewDlg::bone_ysdnm_node(ysdnm_t *dnm, struct ysdnm_srf *srf, ysdnmv_t *v0, const Mat4d &pmat){
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
		ysdnm_bone_var *bonerot = v->bonevar;
		int bones = v->bones;
		for(i = 0; i < bones; i++) if(!strcmp(bonerot[i].name, srf->name)){
			Mat4d rotmat = bonerot[i].rot.tomat4();
	/*		MAT4TRANSLATE(srf->pos[0], srf->pos[1], srf->pos[2]);*/
			Mat4d amat = mat;
			amat.translatein(srf->pos);
			amat.translatein(bonerot[i].pos);
			Mat4d rmat = amat * rotmat;
			rmat.translate(-srf->pos);
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
	for(i = 0; i < dnm->ns; i++) if(dnm->s[main.selectbone] == srf)
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

void CPreviewDlg::BoneYSDNM_V(ysdnm_t *dnm, ysdnmv_t *dnmv){
	if(!dnm->ns)
		return;
	bone_ysdnm_node(dnm, dnm->s[0], dnmv, mat4_u);
}

const MotionNode *CPreviewDlg::FindBone(int boneid){
	ysdnm_t *dnm = main.dnm;
	int selectbone = main.selectbone;
	MotionPose *dnmv = main.dnmv;

	const char *name = 0 <= selectbone && selectbone < dnm->ns && dnm->s[selectbone] ? dnm->s[boneid]->name : NULL;
	if(!name || !dnmv)
		return NULL;
	MotionPose::iterator it = dnmv->nodes.find(name);
	if(it != dnmv->nodes.end()){
		return &it->second;
	}
	return NULL;
}

void CPreviewDlg::display_func(){
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

	MotionPose *dnmv = main.dnmv;
/*	if(main.dnmv){
		if(fmod(realtime, .2) < .1){
			dnmv->target = GetSrfName(main.selectbone);
		}
		else
			dnmv->target = NULL;
	}*/

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

	if(main.dnm){
		glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
		if(wireframe)
			glPolygonMode(GL_FRONT, GL_LINE);
		else
			glPolygonMode(GL_FRONT, GL_FILL);
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

//		DrawYSDNM_V(main.dnm, main.dnmv);
		glPopAttrib();

/*		if(boneview){
			if(boneview == 2)
				glDisable(GL_DEPTH_TEST);
			glPushMatrix();
			BoneYSDNM_V(main.dnm, main.dnmv);
			glPopMatrix();
		}*/
	}
	else if(main.model){
		glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
		if(wireframe)
			glPolygonMode(GL_FRONT, GL_LINE);
		else
			glPolygonMode(GL_FRONT, GL_FILL);
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
//		DrawMQO_V(main.model, dnmv);
		DrawMQOPose(main.model, dnmv);
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

	if(main.dnm){

		glDisable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);

		glLoadIdentity();
		glTranslated(0,0,-.5);

		ysdnm_t *dnm = main.dnm;
		int selectbone = main.selectbone;
		Motion *&motion = main.motion;
		Motion *(&motions)[16] = main.motions;
		int nmotions = main.nmotions, curmotion = main.curmotion;
		double (&motion_time)[16] = main.motion_time;

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
/*		if(motion && motion->kfl.size()){
			glRasterPos2d(-g_width / 2, -g_height * 2 / 3);
			gldprintf("mot: (%d)", &motion->getKeyframe(motion_time[curmotion]) - motion->kfl);
		}*/

		for(int n = 0; n < nmotions; n++) if(motions[n] && motions[n]->kfl.size()){
			Motion *mot = motions[n];
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
			for(int i = 0; i < mot->kfl.size(); i++){
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

const char *CPreviewDlg::GetSrfName(int boneid){
	ysdnm_t *dnm = main.dnm;
	int selectbone = main.selectbone;
	Motion *&motion = main.motion;
	Model *model = main.model;

	if(main.dnm)
		return 0 <= selectbone && selectbone < dnm->ns && dnm->s[selectbone] ? dnm->s[boneid]->name : NULL;
	else if(model)
		return 0 <= selectbone && selectbone < model->n && model->bones[selectbone]->name ? model->bones[selectbone]->name : NULL;
	else
		return NULL;
}
