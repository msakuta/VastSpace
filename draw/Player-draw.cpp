#include "../Player.h"
#include "../viewer.h"
#include "../CoordSys.h"
#include "../Entity.h"
#include "../motion.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufvbo.h>
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquat.h>
#include <clib/GL/gldraw.h>
#include <clib/wavsound.h>
#include <clib/zip/UnZip.h>
}
#include <gl/glext.h>


#define projection(e) glMatrixMode(GL_PROJECTION); e; glMatrixMode(GL_MODELVIEW);

static double diprint(const char *s, double x, double y){
	glPushMatrix();
	glTranslated(x, y, 0.);
	glScaled(8. * 1, -8. * 1, 1.);
	gldPutTextureString(s);
	glPopMatrix();
	return x + strlen(s) * 8;
}

void Player::drawindics(Viewer *vw){
	if(moveorder) do{
		bool move_lockz = this->move_lockz || MotionGet() & PL_SHIFT;
		glPushMatrix();
		glLoadMatrixd(vw->rot);
		gldTranslate3dv(-vw->pos);
//		char *strtargetcs;
		const CoordSys *cs;
		Mat4d rot, mat;
//		strtargetcs = CvarGetString("r_coordsys");
//		if(!strtargetcs)
//			break;
/*		if(!strcmp(strtargetcs, "local")){
			static const aquat_t qrot = {-M_SQRT2 / 2., 0, 0, M_SQRT2 / 2.};
			aquat_t q;
			if(!pl.chase)
				break;
			QUATMUL(q, pl.chase->rot, qrot);
			quat2mat(rot, q);
		}
		else if(!strcmp(strtargetcs, "current")){
			MAT4IDENTITY(rot);
		}
		else*/{
//			cs = findcs(galaxysystem, strtargetcs);
			cs = this->cs;
			if(!cs)
				break;
			rot = vw->cs->tocsim(cs).rotx(M_PI / 2.);
		}

		Mat4d imat;
		Vec3d ray, lray, mpos, vwpos, mlpos;
		double t;
		double (*cuts)[2];
		mat = vw->rot * rot;
		{
			move_rot = rot;
/*				VECCPY(&s_move_rot[12], vw->pos);
			if(pl.selected)
				VECSUBIN(&s_move_rot[12], pl.selected->pos);*/
			glGetDoublev(GL_MODELVIEW_MATRIX, move_model);
			glGetDoublev(GL_PROJECTION_MATRIX, move_proj);
			move_trans = move_proj * move_model;
		}
		Vec3d tpos = vw->pos;
		if(selected){
			tpos -= selected->pos;
			move_src = selected->pos;
		}
		else
			move_src = vec3_000;
		vwpos = rot.tvp3(tpos);
		vwpos[2] += move_z;
		imat = mat.transpose();
/*			VECSADD(velo, xh, (double)(pl.mousex - gvp.w / 2) / gvp.m * 2);
		VECSADD(velo, yh, -(double)(pl.mousey - gvp.h / 2) / gvp.m * 2);*/
		if(move_lockz){
			lray = move_org;
		}
		else{
			lray[0] = 2. * (vw->mousex - vw->vp.w / 2) * fov / vw->vp.m;
			lray[1] = -2. * (vw->mousey - vw->vp.h / 2) * fov / vw->vp.m;
			lray[2] = -1.;
			move_org = lray;
		}
		ray = imat.dvp3(lray);
		t = -vwpos[2] / ray[2];
/*			glBegin(GL_LINES);
		glVertex3dv(avec3_000);
		glVertex3dv(ray);
		glEnd();*/

		glPushMatrix();
		if(selected)
			gldTranslate3dv(selected->pos);
		glMultMatrixd(rot);
		glBegin(GL_LINES);
		for(int i = 0; i <= 10; i++){
			glVertex2d(i * 1. - 5., -5.);
			glVertex2d(i * 1. - 5.,  5.);
			glVertex2d(-5., i * 1. - 5.);
			glVertex2d( 5., i * 1. - 5.);
		}
		glEnd();

		if(move_lockz){
			mpos = move_hitpos;
		}
		else{
			mpos = ray * t;
			mpos += vwpos;
			move_hitpos = mpos;
		}
		glBegin(GL_LINES);
		glVertex3d(0, 0, 0.);
		glVertex3d(mpos[0], mpos[1], mpos[2] - move_z);
		glEnd();
		gldTranslate3dv(mpos);
		glBegin(GL_LINES);
		glVertex3d(0, 0, 0.);
		glVertex3d(0, 0, -move_z);
		glEnd();
		gldScaled(.1);
		cuts = CircleCuts(32);
		glBegin(GL_LINE_LOOP);
		for(int i = 0; i < 32; i++){
			glVertex2d(cuts[i][0], cuts[i][1]);
		}
		glEnd();
		glPopMatrix();

/*		mlpos = rot.dvp3(mpos);
		glPushMatrix();
		gldTranslate3dv(mlpos);
		gldScaled(.05);
		cuts = CircleCuts(32);
		glBegin(GL_LINE_LOOP);
		for(int i = 0; i < 32; i++){
			glVertex2d(cuts[i][0], cuts[i][1]);
		}
		glEnd();
		glPopMatrix();*/

		glLoadIdentity();
		Mat4d irot = mat4_u.rotx(M_PI / 2.);
		Vec4d mpos4 = irot.dvp3(mpos) + move_z * vec3_010 + move_src;
		mpos4[3] = 1.;
		Vec4d spos = move_trans.vp(mpos4);
		projection((glPushMatrix(), glLoadIdentity(), glOrtho(0, vw->vp.w, vw->vp.h, 0, -1, 1)));
		cuts = CircleCuts(32);
		glPushMatrix();
		glTranslated((spos[0] / spos[2] + 1.) * vw->vp.w / 2., (1. - spos[1] / spos[2]) * vw->vp.h / 2., 0.);
		gldScaled(20.);
		glBegin(GL_LINE_LOOP);
		for(int i = 0; i < 32; i++){
			glVertex2d(cuts[i][0], cuts[i][1]);
		}
		glEnd();
		glPopMatrix();
//		projection((glPushMatrix(), glLoadIdentity(), glOrtho(-vw->vp.w / 2., vw->vp.w / 2., vw->vp.h / 2., -vw->vp.h / 2., -1, 1)));
//		int x = vw->vp.w * spos[0] / spos[3] / 2, y = vw->vp.h * spos[1] / spos[3] / 2;
		int x = vw->mousex, y = vw->mousey;
		double destdist = (mpos + move_z * vec3_001).len();
		if(destdist < 1)
			diprint(cpplib::dstring() << destdist * 1000. << "m ", x, y);
		else
			diprint(cpplib::dstring() << destdist << "km ", x, y);
//		diprint(cpplib::dstring() << spos[0] << "," << spos[1] << "," << spos[3], x, y + 12);
//		diprint(cpplib::dstring() << mpos.len() << "km", 0, 0);
		projection((glPopMatrix()));
		glPopMatrix();

		move_viewpos = vw->pos;
	} while(0);
}


void Player::mousemove(HWND hWnd, int deltax, int deltay, WPARAM wParam, LPARAM lParam){
	Player &pl = *this;
	if(pl.moveorder && wParam & MK_SHIFT && !pl.move_lockz){
		Mat4d rotmat = mat4_u.rotx(M_PI / 2.);
		Vec3d lpos = rotmat.dvp3(pl.move_hitpos);
		int maxpix, minpix;
		RECT crect;
		GetClientRect(hWnd, &crect);
		maxpix = MAX(crect.right, crect.bottom);
		minpix = MIN(crect.right, crect.bottom);
#if 1 /* TODO: screen to space z axis coordinates transformation */
		Vec3d delta(deltax, deltay, 0);
		Vec3d normal = (pl.move_trans.vp(Vec4d(lpos) + vec4_0001()) - pl.move_trans.vp(Vec4d((lpos + vec3_010)) + vec4_0001())).norm();
//					normal[1] *= -1;
		double sp = normal.sp(delta);
		pl.move_z += sp * (pl.move_hitpos - pl.move_viewpos).len() / maxpix;
		Vec4d worlddest = Vec4d(rotmat.dvp3(pl.move_hitpos) + pl.move_z * vec3_010 + move_src) + vec4_0001();
		Vec4d redelta = pl.move_trans.vp(worlddest);
		POINT p, p0;
		p.x = (redelta[0] / redelta[3] + 1.) * crect.right / 2;
		p.y = (1. - redelta[1] / redelta[3]) * crect.bottom / 2;
		p0 = p;
		ClientToScreen(hWnd, &p);
		pl.move_lockz = true;
		crect.left += p.x - p0.x;
		crect.right += p.x - p0.x;
		crect.top += p.y - p0.y;
		crect.bottom += p.y - p0.y;
		ClipCursor(&crect);
		SetCursorPos(p.x, p.y);
#elif 0
		avec3_t lray, ray, znorm;
		avec4_t z1, z0, sz1, sz0;
		double sp;
		GLint vp[4] = {0,0,2,2};
		s_move_lockz = 1;
		lray[0] = 2. * (s_mousex - gvp.w / 2) * fov / gvp.m;
		lray[1] = -2. * (s_mousey - gvp.h / 2) * fov / gvp.m;
		lray[2] = -1.;
		VECCPY(z0, s_move_hitpos);
		z0[3] = 1.;
		VECCPY(z1, s_move_hitpos);
		z1[2] += 1.;
		z1[3] = 1.;
		mat4vp(sz1, s_move_trans, z1);
		VECSCALEIN(sz1, 1. / sz1[3]);
		mat4vp(sz0, s_move_trans, z0);
		VECSCALEIN(sz0, 1. / sz0[3]);
		VECSUB(znorm, sz1, sz0);
		VECSUBIN(lray, s_move_org);
		sp = VECSP(lray, znorm);
		VECSADD(sz0, znorm, sp);
		gluUnProject(sz0[0], sz0[1], sz0[2], s_move_model, s_move_proj, vp, &z1[0], &z1[1], &z1[2]);
		s_move_z = z1[2] - z0[2];
/*					mat4vp3(ray, s_move_rot, lray);*/
#else
		pl.move_z += 1e-3 * (s_mousedragy - s_mousey);
#endif
	}
	else{
		pl.move_lockz = false;
		ClipCursor(NULL);
	}
}
