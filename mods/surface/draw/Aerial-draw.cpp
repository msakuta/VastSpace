/** \file
 * \brief Implementation of Aerial class's drawing methods.
 */

#include "../Aerial.h"
#include "Player.h"
#include "Game.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"
#include "glw/glwindow.h"
#include "../Airport.h"
extern "C"{
#include <clib/GL/gldraw.h>
#include <clib/cfloat.h>
}
#include <cpplib/crc32.h>
#include <cpplib/vec2.h>


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

/// \brief Draw HUD on cockpit graphics for aircrafts.
void Aerial::drawCockpitHUD(const Vec3d &hudPos, double hudSize, const Vec3d &seat,
							const Vec3d &gunDirection)const
{
	if(0. < health){
		static const GLfloat hudcolor[4] = {0, 1, 0, 1};
//		GLfloat mat_diffuse[] = { .5, .5, .5, .2 };
//		GLfloat mat_ambient_color[] = { 0.5, 0.5, 0.5, .2 };
		Mat4d m;

		double air = w->atmosphericPressure(this->pos)/*exp(-pt->pos[1] / 10.)*/;

		glPushMatrix();
	/*	glRotated(-gunangle, 1., 0., 0.);*/
		glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_LINE_SMOOTH);
		glDepthMask(GL_FALSE);
		gldTranslate3dv(hudPos - seat);
		gldScaled(hudSize);
/*		gldScaled(.0002);
		glTranslated(0., 0., -1.);*/
		glGetDoublev(GL_MODELVIEW_MATRIX, m);

		glDisable(GL_LIGHTING);
/*		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);*/
		glColor4ub(0,127,255,255);
		glBegin(GL_LINE_LOOP);
		glVertex2d(-1., -1.);
		glVertex2d( 1., -1.);
		glVertex2d( 1.,  1.);
		glVertex2d(-1.,  1.);
		glEnd();

		/* crosshair */
		glColor4fv(hudcolor);
		glPushMatrix();
		// Calculate where the crosshair should be drawn to correctly indicate heading direction of shot rounds.
		const double factor = -(hudPos[2] - seat[2]) / hudSize;
		glTranslated(-gunDirection[0] * factor, gunDirection[1] * factor, 0);
		glBegin(GL_LINES);
		glVertex2d(-.15, 0.);
		glVertex2d(-.05, 0.);
		glVertex2d( .15, 0.);
		glVertex2d( .05, 0.);
		glVertex2d(0., -.15);
		glVertex2d(0., -.05);
		glVertex2d(0., .15);
		glVertex2d(0., .05);
		glEnd();
		glPopMatrix();

		// Watermark (nose direction)
		glBegin(GL_LINE_STRIP);
		glVertex2d(-0.1, 0.0);
		glVertex2d(-0.05, 0.0);
		glVertex2d(-0.025, -0.025);
		glVertex2d( 0.0, 0.0);
		glVertex2d( 0.025, -0.025);
		glVertex2d( 0.05, 0.0);
		glVertex2d(00.1, 0.0);
		glEnd();

		// Calculating heading vector for obtaining pitch and yaw works and intuitive, but it requires extra arithmetic cost
		// compared to the theory's direct solution.
//		const Vec3d heading = rot.trans(Vec3d(0, 0, -1));
//		double yaw = atan2(-heading[0], -heading[2]);

		const Quatd &q = rot;

		// According to the theory, yaw should be -atan2(2 * (q.i() * q.k() - q.re() * q.j()), 1 - 2 * (q.i() * q.i() + q.j() * q.j())).
		// I don't know why this works this way.
		const double yaw = atan2(2 * (q.i() * q.k() + q.re() * q.j()), 1 - 2 * (q.i() * q.i() + q.j() * q.j()));

		// Direction indicator (compass)
		for(int i = 0; i < 72; i++){
			double d = fmod(yaw + i * 2. * M_PI / 72. + M_PI / 2., M_PI * 2.) - M_PI / 2.;
			if(-.5 < d && d < .5){
				glBegin(GL_LINES);
				glVertex2d(d, 0.75);
				glVertex2d(d, i % 2 == 0 ? 0.77 : 0.76);
				glEnd();
				if(i % 2 == 0){
					int j, len;
					char buf[16];
					glPushMatrix();
					len = sprintf(buf, "%d", i * 5);
					glTranslated(d - (len - 1) * 0.015, 0.800, 0.);
					glScaled(0.015, 0.025, .1);
					for(j = 0; buf[j]; j++){
						gldPolyChar(buf[j]);
						glTranslated(2., 0., 0.);
					}
					glPopMatrix();
				}
			}
		}
		glBegin(GL_LINES);
		glVertex2d(-0.5, 0.75);
		glVertex2d( 0.5, 0.75);
		glEnd();

		// Compass center (direction indicator)
		glBegin(GL_LINE_STRIP);
		glVertex2d(-0.04, 0.75 - 0.05);
		glVertex2d( 0   , 0.75);
		glVertex2d( 0.04, 0.75 - 0.05);
		glEnd();

		/* pressure/height gauge */
		glBegin(GL_LINE_LOOP);
		glVertex2d(.92, -0.25);
		glVertex2d(.92, 0.25);
		glVertex2d(.9, 0.25);
		glVertex2d(.9, -.25);
		glEnd();
		glBegin(GL_LINES);
		glVertex2d(.92, -0.25 + air * .5);
		glVertex2d(.9, -0.25 + air * .5);
		glEnd();

		/* height gauge */
		{
			glBegin(GL_LINE_LOOP);
			glVertex2d(.8, .0);
			glVertex2d(.9, 0.025);
			glVertex2d(.9, -0.025);
			glEnd();

			char buf[16];
			// Avoid domain error in vacuum
			if(FLT_EPSILON < air){
				double height = -10. * log(air);
				glBegin(GL_LINES);
				glVertex2d(.8, .3);
				glVertex2d(.8, height < .05 ? -height * .3 / .05 : -.3);
				for(double d = MAX(.01 - fmod(height, .01), .05 - height) - .05; d < .05; d += .01){
					glVertex2d(.85, d * .3 / .05);
					glVertex2d(.8, d * .3 / .05);
				}
				glEnd();
				sprintf(buf, "%6.lf", height / 30.48e-5);
			}
			else
				strcpy(buf, "------");

			glPushMatrix();
			glTranslated(.7, .6, 0.);
			glScaled(.015, .025, .1);
			glBegin(GL_LINE_LOOP);
			glVertex2d(-1., -1.);
			glVertex2d(-1.,  1.);
			glVertex2d(11.,  1.);
			glVertex2d(11., -1.);
			glEnd();
			gldPolyString(buf);
			glPopMatrix();
		}
/*		glRasterPos3d(.45, .5 + 20. / mi, -1.);
		gldprintf("%lg meters", pt->pos[1] * 1e3);
		glRasterPos3d(.45, .5 + 0. / mi, -1.);
		gldprintf("%lg feet", pt->pos[1] / 30.48e-5);*/

		/* velocity */
		glPushMatrix();
		glTranslated(-.2, 0., 0.);
		glBegin(GL_LINES);
		double velo = this->velo.len();
		glVertex2d(-.5, .3);
		glVertex2d(-.5, velo < 50. ? -velo * .3 / 50. : -.3);
		for(double d = MAX(10. - fmod(velo, 10.), 50. - velo) - 50.; d < 50.; d += 10.){
			glVertex2d(-.55, d * .3 / 50.);
			glVertex2d(-.5, d * .3 / 50.);
		}
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex2d(-.5, .0);
		glVertex2d(-.6, 0.025);
		glVertex2d(-.6, -0.025);
		glEnd();
		glPopMatrix();
		glPushMatrix();
		glTranslated(-.7, .7, 0.);
		glScaled(.015, .025, .1);
		gldPolyPrintf("M %lg", velo / 340.);
		glPopMatrix();
		glPushMatrix();
		glTranslated(-.7, .6, 0.);
		glScaled(.015, .025, .1);
/*		gldPolyPrintf("%lg m/s", velo * 1e3);*/
		glBegin(GL_LINE_LOOP);
		glVertex2d(-1., -1.);
		glVertex2d(-1.,  1.);
		glVertex2d(9.,  1.);
		glVertex2d(9., -1.);
		glEnd();

		// Speed in knots
		gldPolyPrintf("%5.lf", 1.94386 * velo);
		glPopMatrix();

		if(0 < health && !controller){
			const double fontSize = 0.005;
			glPushMatrix();
			glTranslated(-fontSize * glwGetSizeTextureString("AUTO PILOT", 12) / 2., 0, 0);
			glScaled(fontSize, -fontSize, .1);
			glwPutTextureString("AUTO PILOT", 12);
			glPopMatrix();
		}

//		const double pitch = -asin(heading[1]);
		// According to the theory, pitch should be asin(2*(q.re() * q.i() + q.j() * q.k())).
		// I don't know why this works this way.
		const double pitch = -asin(2*(q.re() * q.i() - q.j() * q.k()));
		const double roll = atan2(-2 * (q.re() * q.k() + q.i() * q.j()), 1 - 2 * (q.k() * q.k() + q.i() * q.i()));
#if 0 // Quaternion to Euler angles (Roll) conversion experiment code.
		try{
			HSQUIRRELVM v = game->sqvm;
			StackReserver sr(v);
			sq_pushroottable(v);
			sq_pushstring(v, _SC("getRoll"), -1);
			if(SQ_FAILED(sq_get(v, -2)))
				throw SQFError("No getRoll");
			sq_pushroottable(v);
			SQQuatd sq = rot;
			sq.push(v);
			if(SQ_FAILED(sq_call(v, 2, SQTrue, SQTrue)))
				throw SQFError("Call fail");
			SQFloat f;
			if(SQ_FAILED(sq_getfloat(v, -1, &f)))
				throw SQFError("Not convertible to float");
			roll = f;
		}
		catch(SQFError &e){
			CmdPrintf(e.what());
		}
#endif

		// Sort of local function
		auto readOut = [](const char *s, double x, double y, double scale){
			glPushMatrix();
			glTranslated(x, y, 0.);
			glScaled(scale, -scale, .1);
			glwPutTextureString(s, 12);
			glPopMatrix();
		};

#if 0 // Debug draw
		// Print readouts for yaw, pitch and roll.
		readOut(gltestp::dstring("Y") << yaw, -.7, -.8, 0.005);
		readOut(gltestp::dstring("P") << pitch, -.7, -.85, 0.005);
		readOut(gltestp::dstring("R") << roll, -.7, -.90, 0.005);
#endif

		readOut(gltestp::dstring("ARMED: ") << this->getWeaponName(), 0.2, -0.75, 0.0035);
		readOut(gltestp::dstring("AMMO: ") << this->getAmmo(), 0.2, -0.8, 0.0035);

		// Indicates landing gear status
		if(gear)
			readOut("GEAR", -0.5, -0.60, 0.0050);

		// Indicates brake for landing gear
		if(brake)
			readOut("BRAKE", -0.5, -0.65, 0.0050);

		// Indicates afterburner in use
		if(afterburner)
			readOut("A/B", -0.5, -0.75, 0.0050);

		// Indicates spoiler
		if(spoiler)
			readOut("SPOILER", -0.5, -0.85, 0.0050);

		// Velocity Vector or Flight Path Vector.
		if(FLT_EPSILON < this->velo.slen()){
			Vec3d lheading = rot.itrans(this->velo.norm());
			double hudFov = hudSize / (seat - hudPos).len();
			if(-hudFov < lheading[0] && lheading[0] < hudFov && -hudFov < lheading[1] && lheading[1] < hudFov){
				glPushMatrix();
				glTranslated(lheading[0] / hudFov, lheading[1] / hudFov, 0);
				glScaled(0.02, 0.02, 1.);

				// Draw the circle
				glBegin(GL_LINE_LOOP);
				const double (*cuts)[2] = CircleCuts(16);
				for(int i = 0; i < 16; i++){
					double phase = i * 2 * M_PI / 16;
					glVertex2dv(cuts[i]);
				}
				glEnd();

				// Draw the stabilizers
				glBegin(GL_LINES);
				glVertex2d(0, 1);
				glVertex2d(0, 2);
				glVertex2d(1, 0);
				glVertex2d(3, 0);
				glVertex2d(-1, 0);
				glVertex2d(-3, 0);
				glEnd();

				glPopMatrix();
			}
		}

		// ILS indicator
		if(showILS){
			GetILSCommand gisBest;
			double sdistBest = 1e3 * 1e3;
			for(auto it : w->entlist()){
				GetILSCommand gis;
				if(it->command(&gis)){
					double sdist = (gis.pos - this->pos).slen();
					if(sdist < sdistBest){
						sdistBest = sdist;
						gisBest = gis;
					}
				}
			}

			if(sdistBest < 1e6){
				Vec3d relPos = gisBest.pos - this->pos;
				Vec2d lateralDir = Vec2d(relPos[0], relPos[2]).norm();
				Vec3d dir = relPos.norm();
				Vec3d landDir = Quatd::rotation(gisBest.heading, Vec3d(0, 1, 0)).trans(Vec3d(0, 0, 1));
				double loc = lateralDir.vp(Vec2d(landDir[0], landDir[2])); // Localizer
				double gs = rangein(relPos.norm()[1] + asin(3. / deg_per_rad), -1, 1); // Glide slope is 3 degrees
				glBegin(GL_LINES);
				glVertex2d(0.6 * loc, -0.6);
				glVertex2d(0.6 * loc, 0.6);
				glVertex2d(0.6, 0.6 * gs);
				glVertex2d(-0.6, 0.6 * gs);
				glEnd();
				readOut("LOC", 0.6 * loc, -0.55, 0.0050);
				readOut("GS", 0.6, 0.6 * gs, 0.0050);
			}
		}

		// Roll and Pitch indicator
		glRotated(deg_per_rad * (roll), 0., 0., 1.);
		for(int i = -12; i <= 12; i++){
			double d = 2. * (fmod(pitch + i * 2. * M_PI / 48. + M_PI / 2., M_PI * 2.) - M_PI / 2.);
			if(-.8 < d && d < .8){
				const double xstart = 0.2;
				const double xend = i % 6 == 0 ? .5 : 0.36;
				glBegin(GL_LINES);
				if(i < 0){
					// Negative pitch lines are dashed
					for(int k = -1; k <= 1; k += 2){
						for(double h = xstart; h < xend; h += 0.02){
							glVertex2d(k * h, d);
							glVertex2d(k * (h + 0.01), d);
							glVertex2d(k * xend, d);
							glVertex2d(k * xend, d + 0.03);
						}
					}
				}
				else{
					for(int k = -1; k <= 1; k += 2){
						glVertex2d(k * xstart, d);
						glVertex2d(k * xend, d);
						glVertex2d(k * xend, d);
						glVertex2d(k * xend, d - 0.03);
					}
				}
				glEnd();
				if(i % 2 == 0){
					char buf[16];
					int len = sprintf(buf, "%d", std::abs(i * 45 / 6));
					for(int k = -1; k <= 1; k += 2){
						glPushMatrix();
						glTranslated(k * (xend + (k < 0 ? len * .04 : 0.04)), d, 0.);
						glScaled(.02, .03, .1);
						for(int j = 0; buf[j]; j++){
							gldPolyChar(buf[j]);
							glTranslated(2., 0., 0.);
						}
						glPopMatrix();
					}
				}
			}
		}

		glPopAttrib();
		glPopMatrix();
	}
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

/// \biref A shared logic for Aerial vehicles to show targetting marker on the HMD.
///
/// Unlike drawCockpitHUD(), this function is meant to be called from drawHUD(),
/// which is really drawing HMD now.
void Aerial::drawTargetMarker(WarDraw *wd){
	if(isCockpitView(game->player->getChaseCamera()) && this->enemy){
		glPushMatrix();
		glLoadMatrixd(wd->vw->rot);
		Vec3d pos = (this->enemy->pos - this->pos).norm();
		gldTranslate3dv(pos);
		glMultMatrixd(wd->vw->irot);
		{
			Vec3d dp = this->enemy->pos - this->pos;
			double dist = dp.len();
			glRasterPos3d(.01, .02, 0.);
			if(dist < 1000.)
				gldprintf("%.4g m", dist);
			else
				gldprintf("%.4g km", dist / 1000.);
			Vec3d dv = this->enemy->velo - this->velo;
			dist = dv.sp(dp) / dist;
			glRasterPos3d(.01, -.03, 0.);
			if(dist < 1000.)
				gldprintf("%.4g m/s", dist);
			else
				gldprintf("%.4g km/s", dist / 1000.);
		}
		glBegin(GL_LINE_LOOP);
		glVertex3d(.05, .05, 0.);
		glVertex3d(-.05, .05, 0.);
		glVertex3d(-.05, -.05, 0.);
		glVertex3d(.05, -.05, 0.);
		glEnd();
		glBegin(GL_LINES);
		glVertex3d(.04, .0, 0.);
		glVertex3d(.02, .0, 0.);
		glVertex3d(-.04, .0, 0.);
		glVertex3d(-.02, .0, 0.);
		glVertex3d(.0, .04, 0.);
		glVertex3d(.0, .02, 0.);
		glVertex3d(.0, -.04, 0.);
		glVertex3d(.0, -.02, 0.);
		glEnd();
		glPopMatrix();
	}
}
