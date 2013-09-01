/** \file
 * \brief Implementation of Aerial class's drawing methods.
 */

#include "../Aerial.h"
#include "Player.h"
#include "Game.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/GL/gldraw.h>
}
#include <cpplib/crc32.h>


bool Aerial::cull(WarDraw *wd)const{
	if(wd->vw->gc->cullFrustum(pos, .012))
		return 1;
	double pixels = .008 * fabs(wd->vw->gc->scale(pos));
//	if(ppixels)
//		*ppixels = pixels;
	if(pixels < 2)
		return 1;
	return 0;
}



void drawmuzzleflash(const Vec3d &pos, const Vec3d &org, double rad, const Mat4d &irot){
	double (*cuts)[2];
	int i;
	cuts = CircleCuts(10);
	RandomSequence rs(crc32(pos, sizeof pos));
	glPushMatrix();
	gldTranslate3dv(pos);
	glMultMatrixd(irot);
	gldScaled(rad);
	glBegin(GL_TRIANGLE_FAN);
	glColor4ub(255,255,31,255);
	glVertex3d(0., 0., 0.);
	glColor4ub(255,0,0,0);
	{
		double first;
		first = drseq(&rs) + .5;
		glVertex3d(first * cuts[0][1], first * cuts[0][0], 0.);
		for(i = 1; i < 10; i++){
			int k = i % 10;
			double r;
			r = drseq(&rs) + .5;
			glVertex3d(r * cuts[k][1], r * cuts[k][0], 0.);
		}
		glVertex3d(first * cuts[0][1], first * cuts[0][0], 0.);
	}
	glEnd();
	glPopMatrix();
}

/// \brief Debugging function to draw wing status over the Aerial vehicles' graphics.
void Aerial::drawDebugWings()const{
	// Show forces applied to each wings
	glBegin(GL_LINES);
	glColor4f(1,0,0,1);
	for(auto i = 0; i < getWings().size(); i++){
		if(i < force.size()){
			Vec3d org = this->pos + rot.trans(getWings()[i].pos);
			glVertex3dv(org);
			glVertex3dv(org + force[i] * 0.01);
		}
	}
	glEnd();

	// Show control surfaces directions
	glPushAttrib(GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor4f(0,0,1,1);
	for(auto it : getWings()){
		glPushMatrix();
		gldTranslate3dv(pos);

		gldMultQuat(rot);
		Quatd rot = quat_u;
		if(it.control == Wing::Control::Elevator)
			rot = Quatd::rotation(elevator * it.sensitivity, it.axis);
		else if(it.control == Wing::Control::Aileron)
			rot = Quatd::rotation(aileron * it.sensitivity, it.axis);
		else if(it.control == Wing::Control::Rudder)
			rot = Quatd::rotation(rudder * it.sensitivity, it.axis);

		// Obtain scales that makes cuboid with face areas proportional with
		// drag force perpendicular to the faces.
		//
		// Suppose $x,y,z$ are the each axis length.  We know that each side
		// face perpendicular to $x,y,z$ axes have area $X=yz, Y=zx, Z=xy$, respectively.
		// We can, for example, erase $y$ and $z$ from the equations to obtain
		// the length $x=\sqrt{YZ/X}$.
		//
		// Note that we cannot visualize sheer elements (other than diagonals) with
		// this technique.  I don't think doing so is needed.
		Vec3d sc;
		for(int i = 0; i < 3; i++) // Machine-code friendly loop logic
			sc[i] = ::sqrt((::fabs(it.aero[(i+1) % 3 * 4]) + ::fabs(it.aero[(i+2) % 3 * 4])) / ::fabs(it.aero[i * 4]));

		sc *= 0.0002; // Tweak visuals

		gldTranslate3dv(it.pos);
		gldMultQuat(rot);
		glScaled(sc[0], sc[1], sc[2]);

		// Draw cuboid-like wireframe to show each area along axes.
		glBegin(GL_QUADS);
		static const GLfloat arr[3][4] = {
			{-1, -1,  1, 1},
			{ 1, -1, -1, 1},
			{ 0,  0,  0, 0},
		};
		for(int i = 0; i < 3; i++){
			int i1 = (i + 1) % 3, i2 = (i + 2) % 3;
			for(int j = 0; j < 4; j++)
				glVertex3f(arr[i][j], arr[i1][j], arr[i2][j]);
		}
		glEnd();

		glPopMatrix();
	}
	glPopAttrib();
}



/*
static void fly_gib_draw(const Teline3CallbackData *pl, const struct Teline3CallbackData *dd, void *pv){
	int i = (int)pv;
	suf_t *suf;
	double scale;
	if(i < fly_s.sufbase->np){
		suf = fly_s.sufbase;
		scale = FLY_SCALE;
		gib_draw(pl, suf, scale, i);
	}
}
*/

void Flare::drawtra(WarDraw *wd){
	GLubyte white[4] = {255,255,255,255}, red[4] = {255,127,127,127};
	double len = /*((flare_t*)pt)->ttl < 1. ? ((flare_t*)pt)->ttl :*/ 1.;
	RandomSequence rs(crc32(&game->universe->global_time, sizeof game->universe->global_time, crc32(this, sizeof this)));
	red[3] = 127 + rseq(&rs) % 128;
	double sdist = (wd->vw->pos - this->pos).slen();
	gldSpriteGlow(this->pos, len * (sdist < .1 * .1 ? sdist / .1 * .05 : .05), red, wd->vw->irot);
	if(sdist < 1. * 1.)
		gldSpriteGlow(this->pos, .001, white, wd->vw->irot);
	if(this->pos[1] < .1){
		static const amat4_t mat = { /* pseudo rotation; it simply maps xy plane to xz */
			1,0,0,0,
			0,0,-1,0,
			0,1,0,0,
			0,0,0,1
		};
		Vec3d pos = this->pos;
		pos[1] = 0.;
		red[3] = red[3] * len * (.1 - this->pos[1]) / .1;
		gldSpriteGlow(pos, .10 * this->pos[1] * this->pos[1] / .1 / .1, red, mat);
	}
}


