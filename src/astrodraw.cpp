/** \file
 * \brief Implementation of drawing methods of astronomical objects like Star and TexSphere.
 */
#define NOMINMAX
#include "astrodraw.h"
#include "draw/ring-draw.h"
#include "draw/DrawTextureSphere.h"
#include "draw/HDR.h"
#include "glw/GLWchart.h"
#include "Universe.h"
#include "judge.h"
#include "CoordSys.h"
#include "antiglut.h"
#include "galaxy_field.h"
#include "astro_star.h"
#include "stellar_file.h"
#include "TexSphere.h"
#include "cmd.h"
#include "glsl.h"
#include "glstack.h"
#include "StaticInitializer.h"
#include "CoordSys-find.h"
#include "Game.h"
#include "sqadapt.h"
#include "StarEnum.h"
#include "astrodef.h"
#include "draw/material.h"
#define exit something_meanless
#include <windows.h>
#undef exit
extern "C"{
#include "bitmap.h"
#include <clib/amat3.h>
#include <clib/rseq.h>
#include <clib/timemeas.h>
#include <clib/gl/cull.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/multitex.h>
#include <clib/suf/sufdraw.h>
#include <clib/colseq/cs2x.h>
#include <clib/cfloat.h>
#include <clib/c.h>
}
#include <clib/stats.h>
#include <cpplib/gl/cullplus.h>
#include <cpplib/mat2.h>
#include <gl/glu.h>
#include <gl/glext.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include <strstream>
#include <sstream>
#include <iomanip>
#include <algorithm>


#define COLIST(c) COLOR32R(c), COLOR32G(c), COLOR32B(c), COLOR32A(c)
#define EPSILON 1e-7 // not sure
#define DRAWTEXTURESPHERE_PROFILE 0
#define ICOSASPHERE_PROFILE 0
#define SQRT2P2 (1.4142135623730950488016887242097/2.)

int g_invert_hyperspace = 0;
static double r_star_back_brightness = 1. / 50.; ///< The scale of brightness of background stars.
static double r_galaxy_back_brightness = 5e-4;

static void initStarBack(){
	CvarAdd("r_star_back_brightness", &r_star_back_brightness, cvar_double);
	CvarAdd("r_galaxy_back_brightness", &r_galaxy_back_brightness, cvar_double);
}
StaticInitializer it(initStarBack);

void drawpsphere(Astrobj *ps, const Viewer *vw, COLOR32 col);
void drawAtmosphere(const Astrobj *a, const Viewer *vw, const Vec3d &sunpos, double thick, const GLfloat hor[4], const GLfloat dawn[4], GLfloat ret_horz[4], GLfloat ret_amb[4], int slices);

#if 0
void directrot(const double pos[3], const double base[3], amat4_t irot){
#if 1
	avec3_t dr, v;
	aquat_t q;
	amat4_t mat;
	double p;
	VECSUB(dr, pos, base);
	VECNORMIN(dr);

	/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
	q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

	VECVP(v, avec3_001, dr);
	p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
	VECSCALE(q, v, p);
	quat2mat(mat, q);
	glMultMatrixd(mat);
	if(irot){
		MAT4CPY(irot, mat);
	}
#else
	double x = pos[0] - base[0], z = pos[2] - base[2], phi, theta;
	phi = atan2(x, z);
	theta = atan2((pos[1] - base[1]), sqrt(x * x + z * z));
	glRotated(phi * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(theta * 360 / 2. / M_PI, -1., 0., 0.);

	if(irot){
		amat4_t mat;
		MAT4ROTY(mat, mat4identity, -phi);
		MAT4ROTX(irot, mat, -theta);
	}
#endif
}


void rgb2hsv(double hsv[3], double r, double g, double b){
	double *ma, mi;
	ma = r < g ? &g : &r;
	ma = *ma < b ? &b : ma;
	mi = MIN(r, MIN(g, b));
	if(*ma == mi) hsv[0] = 0.; /* this value doesn't care */
	else if(ma == &r) hsv[0] = (g - b) / (*ma - mi) / 6.;
	else if(ma == &b) hsv[0] = (b - r) / (*ma - mi) / 6. + 1. / 3.;
	else hsv[0] = (r - g) / (*ma - mi) / 6. + 2. / 3.;
	hsv[0] -= floor(hsv[0]);
	hsv[1] = *ma ? (*ma - mi) / *ma : 1.;
	hsv[2] = *ma;
	assert(0. <= hsv[0] && hsv[0] <= 1.);
	assert(0. <= hsv[1] && hsv[1] <= 1.);
	assert(0. <= hsv[2] && hsv[2] <= 1.);
}

void hsv2rgb(double rgb[3], const double hsv[3]){
	int hi;
	double hue = hsv[0], sat = hsv[1], bri = hsv[2], f, p, q, t, r, g, b;
	hi = (int)floor(hue * 6.);
	f = hue * 6. - hi;
	p = bri * (1 - sat);
	q = bri * (1 - f * sat);
	t = bri * (1 - (1 - f) * sat);
	switch(hi){
		case 0: r = bri, g = t, b = q; break;
		case 1: r = q, g = bri, b = p; break;
		case 2: r = p, g = bri, b = t; break;
		case 3: r = p, g = q, b = bri; break;
		case 4: r = t, g = p, b = bri; break;
		case 5:
		case 6: r = bri, g = p, b = q; break;
		default: r = g = b = bri;
	}
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

void doppler(double rgb[3], double r, double g, double b, double velo){
	double hsv[3];
	rgb2hsv(hsv, r, g, b);
	hsv[0] = hsv[0] + 1. * velo / LIGHT_SPEED;
	hsv[1] = 1. - (1. - hsv[1]) / (1. + fabs(velo) / LIGHT_SPEED);
	hsv[2] *= (hsv[0] < 0. ? 1. / (1. - hsv[0]) : 1. < hsv[0] ? 1. / hsv[0] : 1.);
	hsv[0] = rangein(hsv[0], 0., 1.);
	hsv2rgb(rgb, hsv);
}

#endif








static int g_tscuts = 32;
static int g_cloud = 1;
static void g_tscuts_init(){CvarAdd("g_tscuts", &g_tscuts, cvar_int); CvarAdd("g_cloud", &g_cloud, cvar_int);}
static StaticInitializer s_tscuts(g_tscuts_init);

void TexSphere::draw(const Viewer *vw){

	// The noise position vector "approaches" the camera to simulate time fluctuating noise
	// without apparent spatial drift.
	// We will convert it to float vector in order to pass to the shader program, but accumulating
	// it over time with better precision is preferred, because this method is called thousands of
	// times in a session.
	// The floating point values tend to get far from 0, which is most precise region
	// in the supported range of a floating point value.
	Vec3d apparentPos = tocs(vw->pos, vw->cs);
	if(FLT_EPSILON < apparentPos.slen())
		noisePos += apparentPos.norm() * vw->dt;

	if(vw->zslice != 2)
		return;

	GLint maxLights = 1;
	glGetIntegerv(GL_MAX_LIGHTS, &maxLights);
	FindBrightestAstrobj param(vw->cs, apparentPos, maxLights, this);
	param.threshold = 1e-6;
	param.eclipseThreshold = 1e-5;

	find(param);
	Astrobj *sun = param.results.size() ? const_cast<Astrobj*>(param.results[0].cs) : NULL;
	Vec3d sunpos = sun ? vw->cs->tocs(sun->pos, sun->parent) : vec3_000;
	Quatd ringrot;
	int ringdrawn = 8;
	bool drawring = 0. < ringthick && !vw->gc->cullFrustum(calcPos(*vw), rad * ringmax * 1.1);

	GLfloat brightness = GLfloat(sqrt(param.brightness));

	// Sun distance
	double sundist = sun ? (parent->tocs(sun->pos, sun->parent) - pos).len() : 1e5;

	// Sun apparent radius
	double sunar = sun && sundist != 0. ? sun->rad / sundist : .01;

	Vec3d pos = vw->cs->tocs(this->pos, this->parent);
	if(drawring && 0. < ringmin){
		double dist = (vw->pos - pos).len();
		double ratio = this->rad / dist;
		// Avoid domain error
		double theta = ratio < 1. ? acos(ratio) : 0.;
		ringdrawn = dist / (this->rad * ringmin) < 1. ? 0 : int(theta * RING_CUTS / 2. / M_PI + 1);
		ringrot = Quatd::direction(omg);
		astroRing.ring_draw(*vw, this, sunpos, ringdrawn, RING_CUTS / 2, ringrot, ringthick, ringmin, ringmax, 0., oblateness, ringtexname, ringbacktexname, sunar, brightness);
	}

	drawAtmosphere(this, vw, sunpos, atmodensity, Vec4f(atmohor) * brightness, Vec4f(atmodawn) * brightness, NULL, NULL, 32);
	if(sunAtmosphere(*vw)){
		if(sun)
			Star::drawsuncorona(sun, vw);
	}
	if(!vw->gc->cullFrustum(calcPos(*vw), rad * 2.)){
		if(g_shader_enable){
			if(!shaderGiveup && !shader && vertexShaderName.size() && fragmentShaderName.size()){
				GLuint *shaders = NULL;
				try{
					std::vector<GLuint> shaders(vertexShaderName.size() + fragmentShaderName.size());
					int j = 0;
					for(unsigned i = 0; i < vertexShaderName.size(); i++)
						if(!glsl_load_shader(shaders[j++] = glCreateShader(GL_VERTEX_SHADER), vertexShaderName[i]))
							throw 1;
					for(unsigned i = 0; i < fragmentShaderName.size(); i++)
						if(!glsl_load_shader(shaders[j++] = glCreateShader(GL_FRAGMENT_SHADER), fragmentShaderName[i]))
							throw 2;
					shader = glsl_register_program(&shaders.front(), int(vertexShaderName.size() + fragmentShaderName.size()));
					for(unsigned i = 0; i < shaders.size(); i++)
						glDeleteShader(shaders[i]);
					if(!shader)
						throw 3;
				}
				catch(int){
					CmdPrint(cpplib::dstring() << "Shader compile error for " << getpath() << " globe.");
					shaderGiveup = true;
				}
			}

			if(!cloudShaderGiveup && !cloudShader && cloudVertexShaderName.size() && cloudFragmentShaderName.size()){
				try{
					std::vector<GLuint> shaders(cloudVertexShaderName.size() + cloudFragmentShaderName.size());
					int j = 0;
					for(unsigned i = 0; i < cloudVertexShaderName.size(); i++)
						if(!glsl_load_shader(shaders[j++] = glCreateShader(GL_VERTEX_SHADER), cloudVertexShaderName[i]))
							throw 1;
					for(unsigned i = 0; i < cloudFragmentShaderName.size(); i++)
						if(!glsl_load_shader(shaders[j++] = glCreateShader(GL_FRAGMENT_SHADER), cloudFragmentShaderName[i]))
							throw 2;
					cloudShader = glsl_register_program(&shaders.front(), int(shaders.size()));
					for(unsigned i = 0; i < shaders.size(); i++)
						glDeleteShader(shaders[i]);
					if(!shader)
						throw 3;
				}
				catch(int){
					CmdPrint(cpplib::dstring() << "Shader compile error for " << getpath() << " cloud.");
					cloudShaderGiveup = true;
				}
			}
		}
		double fcloudHeight = cloudHeight == 0. ? rad * 1.001 : cloudHeight + rad;
		bool underCloud = (pos - vw->pos).len() < fcloudHeight;
		if(oblateness != 0.){
			DrawTextureSpheroid cloudDraw = DrawTextureSpheroid(this, vw, sunpos);
			if(g_cloud && (cloudtexname.len() || cloudtexlist)){
				cloudDraw
				.oblateness(oblateness)
				.flags(DTS_LIGHTING)
				.texlist(&cloudtexlist)
				.texmat(cloudRotation().cnj().tomat4())
				.texname(cloudtexname)
				.textures(textures)
				.shader(cloudShader)
				.rad(fcloudHeight)
				.flags(DTS_ALPHA | DTS_NODETAIL | DTS_NOGLOBE)
				.drawint(true)
				.ncuts(g_tscuts);
				if(underCloud){
					bool ret = cloudDraw.draw();
					if(!ret && *cloudtexname){
						cloudtexname = "";
					}
				}
			}
			bool ret = DrawTextureSpheroid(this, vw, sunpos)
				.oblateness(oblateness)
				.flags(DTS_LIGHTING)
				.mat_diffuse(basecolor * brightness)
				.mat_ambient(basecolor * brightness / 10.)
				.texlist(&texlist)
				.texmat(mat4_u)
				.textures(textures)
				.shader(shader)
				.texname(texname)
				.rad(rad)
				.ring(&astroRing)
				.ringRange(ringmin, ringmax)
				.cloudRotation(cloudRotation())
				.noisePos(noisePos.cast<float>())
				.lightingStar(param.results)
				.draw();
			if(!ret && *texname){
				texname = "";
			}
			if(g_cloud && (cloudtexname.len() || cloudtexlist) && !underCloud){
				bool ret = cloudDraw.drawint(false).draw();
				if(!ret && *cloudtexname){
					cloudtexname = "";
				}
			}
		}
		else{
			DrawTextureSphere cloudDraw = DrawTextureSphere(this, vw, sunpos);
			if(g_cloud && (cloudtexname.len() || cloudtexlist)){
				cloudDraw
				.texlist(&cloudtexlist)
				.texmat(cloudRotation().cnj().tomat4())
				.texname(cloudtexname)
				.textures(textures)
				.shader(cloudShader)
				.rad(fcloudHeight)
				.flags(DTS_LIGHTING | DTS_ALPHA | DTS_NODETAIL | DTS_NOGLOBE)
				.lightingStar(param.results)
				.drawint(true)
				.ncuts(g_tscuts);
				if(underCloud){
					bool ret = cloudDraw.draw();
					if(!ret && *cloudtexname){
						cloudtexname = "";
					}
				}
			}
			auto proc = [&](DrawTextureSphere &ds){
				return ds
				.flags(DTS_LIGHTING)
				.mat_diffuse(basecolor)
				.mat_ambient(basecolor / 2.f)
				.texlist(&texlist).texmat(rot.cnj().tomat4()).texname(texname).shader(shader)
				.textures(textures)
				.ncuts(g_tscuts)
				.ring(&astroRing)
				.cloudRotation(cloudRotation())
				.noisePos(noisePos.cast<float>())
				.lightingStar(param.results)
				.draw();
			};
			bool ret = terrainNoiseEnable ?
				proc(DrawTextureCubeEx(this, vw, sunpos)
				.noiseHeight(terrainNoiseHeight)
				.noisePersistence(terrainNoisePersistence)
				.noiseOctaves(terrainNoiseOctaves))
				: proc(DrawTextureSphere(this, vw, sunpos));
			if(!ret && *texname){
				texname = "";
			}
			if(g_cloud && (cloudtexname.len() || cloudtexlist) && !underCloud){
				bool ret = cloudDraw.drawint(false).draw();
				if(!ret && *cloudtexname){
					cloudtexname = "";
				}
			}
		}
	}
	st::draw(vw);
	if(drawring && ringdrawn)
		astroRing.ring_draw(*vw, this, sunpos, 0, ringdrawn, ringrot, ringthick, ringmin, ringmax, 0., oblateness, NULL, NULL, sunar, brightness);
}

inline double atmo_sp2brightness(double sp);


double TexSphere::getAmbientBrightness(const Viewer &vw)const{
	// If there is no atmosphere, it's no use examining ambient brightness
	// induced by air scattering. Also it prevents zero division.
	// TODO: no need to think about diffuse scattering?
	if(atmodensity <= FLT_EPSILON)
		return 0.;

	FindBrightestAstrobj param(this, vec3_000);
	param.returnBrightness = true;
	param.threshold = 1e-20;

	const CoordSys *vwcs = vw.cs;
	if(!vwcs)
		return 0.;
	const Vec3d &vwpos = vw.pos;

	find(param);

	const Astrobj *sun = param.results.size() ? param.results[0].cs : NULL;
	Vec3d sunpos = sun ? vwcs->tocs(sun->pos, sun->parent) : vec3_000;
	Vec3d thispos = vwcs->tocs(this->pos, this->parent);

	Vec3d vwrpos = vwpos - thispos;
	Vec3d sunrpos = sunpos - thispos;
	double dist = vwrpos.len();
	double sp = FLT_EPSILON < sunrpos.slen() && FLT_EPSILON < vwrpos.slen() ? sunrpos.norm().sp(vwrpos.norm()) : 0;

	// Brightness for color components.
	double b = atmo_sp2brightness(sp);

	// Boost the brightness value to illuminate the day side with more intense ambient light.
	if(0.5 < b)
		b = (b - 0.5) * 2.0 + 0.5;

	double height = (thispos - vwpos).len() - this->rad;
	double thick = atmodensity;
	double pd = (thick * 16.) / (dist - this->rad);
	double d = std::min(pd, 1.);
	double air = height / thick / d;

	return sqrt(param.brightness) * b / (1. + air);
}

struct atmo_dye_vertex_param{
	double redness;
	double isotropy;
	double air;
	double d;
	double pd;
	double thickFactor;
	const Vec3d *sundir;
	const GLfloat *col, *dawn;
};

/// \brief Linear interpolation. Should be in some shared library.
static inline double lerp(double a, double b, double f){
	return a * (1. - f) + b * f;
}

/// \brief Convert scalar product of the sun lay direction and position vector to brightness factor.
/// Only used in atmo_dye_vertex().
inline double atmo_sp2brightness(double sp){
	double asp = 1. - fabs(sp);
	double asp2 = asp * asp;
	return (sp < 0 ? 0.99 * pow(1. + sp, 8.) + 0.01 : sp + 1.) / 2.;
}

/// \brief Blend the colors to represent sky sphere.
///
/// It became harder to implement since HDR auto exposure is introduced.
/// You would be able to take qualitative solution in non-HDR rendering algorithms, but in HDR, you'll need
/// knowledge of environmental lighting at some degree, such as Mie and Rayleigh scattering.
/// I don't have one, so here is the way invented by tuning parameters.
///
/// This function have a lot of room for improvements.
///
/// \param p The shared parameters given from callee.
/// \param x,y,z The vector in local (z up) coordinate system representing position of the vertex being drawn.
/// \param s The factor of distance from horizon. 0 is horizon, 1 is zenith.
/// \param base The position vector of intersecting point of the horizon and the vertical line projected from the point being drawn.
/// \param is The integral index of s variable.
static void atmo_dye_vertex(struct atmo_dye_vertex_param &p, double x, double y, double z, double s, const Vec3d &base, int is){
	const Vec3d &sundir = *p.sundir;
	Vec3d v(x, y, z);
	double sp = base.sp(sundir);
	double vsp = v.sp(sundir);

	// This value is 1 if the vertex is at zenith.
	// We interpolate horizon and zenith linearly to produce brightness.
	double zenith = (1. - z) / 2.;

	// Brightness for color components.
	double b = lerp(atmo_sp2brightness(sp), atmo_sp2brightness(-sundir[2]), zenith);

	// Boost the brightness value to illuminate the day side with more intense ambient light.
	if(0.5 < b)
		b = (b - 0.5) * 2.0 + 0.5;

	// Obtain the value indicating how the vertex is near the horizon.
	double horizon = 1. * (1. - (1. - v[2]) * (1. - v[2])) / (1. - sundir[2] * 4.);
	horizon = 1. / (1. + fabs(horizon));

	// This is the factor of how the vertex is near dawn or sunset sun.
	double f = p.redness * (vsp + 1. + p.isotropy) / (2. + p.isotropy) * horizon;

	// Brightness for alpha channel.
	double brightness = (/*1. +*/ b) /* 2.*/ / (1. + (1. - s * s) * p.air);
	brightness *= lerp(1., (is / 16.) * (is / 16.), p.thickFactor);

/*	g = f * horizon * horizon * horizon * horizon;*/
/*	f = redness * ((v[0] * sundir[0] + v[1] * sundir[1]) + v[2] * sundir[2] * isotropy + 1.) / (2. + isotropy);*/
	glColor4d(p.col[0] * b * (1. - f) + p.dawn[0] * f,
		p.col[1] * b * (1. - f) + p.dawn[1] * f,
		p.col[2] * b * (1. - f) + p.dawn[2] * f,
		brightness * lerp(p.col[3], p.dawn[3], f));
}

void drawAtmosphere(const Astrobj *a, const Viewer *vw, const Vec3d &sunpos, double thick, const GLfloat hor[4], const GLfloat dawn[4], GLfloat ret_horz[4], GLfloat ret_amb[4], int slices){
	// Don't call me from the first place!
	if(thick == 0.) return;
	int hdiv = slices == 0 ? 16 : slices;
	int s, t;
	Vec3d apos = vw->cs->tocs(a->pos, a->parent);
	Vec3d delta = apos - vw->pos;
	double sdist = delta.slen();

	/* too far */
	if(DBL_EPSILON < sdist && a->rad * a->rad / sdist < .01 * .01)
		return;

	static bool shader_compile = false;
	static GLuint shader = 0;

	if(g_shader_enable) do{
		static GLint exposureLoc = -1;
		static GLint tonemapLoc = -1;
		if(!shader_compile){
			shader_compile = true;
			GLuint shaders[2], &vtx = shaders[0], &frg = shaders[1];
			vtx = glCreateShader(GL_VERTEX_SHADER);
			if(!glsl_load_shader(vtx, "shaders/atmosphere.vs"))
				break;
			frg = glCreateShader(GL_FRAGMENT_SHADER);
			if(!glsl_load_shader(frg, "shaders/atmosphere.fs"))
				break;
			shader = glsl_register_program(shaders, 2);
			glDeleteShader(vtx);
			glDeleteShader(frg);
			if(!shader)
				break;
			exposureLoc = glGetUniformLocation(shader, "exposure");
			tonemapLoc = glGetUniformLocation(shader, "tonemap");
		}
		glUseProgram(shader);
		glUniform1f(exposureLoc, GLfloat(r_exposure));
		glUniform1i(tonemapLoc, r_tonemap);
	} while(0);


	double dist = sqrt(sdist);
	/* buried in */
	if(dist < a->rad)
		dist = a->rad + EPSILON;
	double sas = a->rad / dist;
	double as = asin(sas);
	double cas = cos(as);
	double height = dist - a->rad;
	double (*hcuts)[2] = CircleCuts(hdiv);
	double isotropy = thick / (thick / 5. + height);
	Vec3d spos = sunpos - apos;

	double b;
	double redness;
	{
		double sp, cen;
		double slen = spos.len();
		if(slen < EPSILON)
			return;
		sp = spos.sp(delta) / slen / dist;
		b = (1. - sp) / 2.;
		cen = (sp - .0);
		redness = (1. / (1. + 500 * cen * cen * cen * cen) / (1. + MAX(0., dist - a->rad) / thick));
	}

	glPushMatrix();
	Vec3d sundir;
	{
		Mat4d rot;
		gldLookatMatrix(~rot, ~(apos - vw->pos));
		glMultMatrixd(rot);
		sundir = rot.tdvp3(spos);
		sundir.normin();
	}
/*	glRotated(phi * 360 / 2. / M_PI, 0., 1., 0.);
	glRotated(theta * 360 / 2. / M_PI, -1., 0., 0.);*/
	struct atmo_dye_vertex_param param;
	double &pd = param.pd;
	double &d = param.d;
	double ss0 = sin(M_PI / 2. - as), sc0 = -cos(M_PI / 2. - as);
	pd = (thick * 16.) / (dist - a->rad);
	d = std::min(pd, 1.);
	param.redness = redness;
	param.isotropy = isotropy;
	param.air = height / thick / d;
	param.sundir = &sundir;
	param.col = hor;
	param.dawn = dawn;
	param.thickFactor = d / pd;
	for(s = 0; s < 16; s++){
		double h[2][2];
		double dss[2];
		double sss, sss1, ss, ss1, f, f1;
		sss = 16. * (1. - pow(1. - s / 16., 1. / (dist - a->rad)));
		sss1 = 16. * (1. - pow(1. - (s+1) / 16., 1. / (dist - a->rad)));
		ss = 1. - pow(1. - s / 16., 2. + (1. - d));
		ss1 = 1. - pow(1. - (s+1) / 16., 2. + (1. - d));
		f = (1. + (16 - s) / 16.), f1 = (1. + (16 - (s + 1)) / 16.);
/*		if(128. < (15 - s) / 16. * (dist - a->rad) / thick / d)
			continue;*/
		h[0][1] = sin((M_PI - as) * (dss[0] = d * ss + (1. - d)));
		h[0][0] = -cos((M_PI - as) * (dss[0]));
		h[1][1] = sin((M_PI - as) * (dss[1] = d * ss1 + (1. - d)));
		h[1][0] = -cos((M_PI - as) * (dss[1]));
		for(t = 0; t < hdiv; t++){
			int t1 = (t + 1) % hdiv;
			double dawness = redness * (1. - (1. - h[0][0]) * (1. - h[0][0])) / (1. + -sundir[2] * 4.);
			Vec3d base[2];
			base[0] = Vec3d(hcuts[t][0] * ss0, hcuts[t][1] * ss0, sc0);
			base[1] = Vec3d(hcuts[t1][0] * ss0, hcuts[t1][1] * ss0, sc0);

			glBegin(GL_QUADS);
			atmo_dye_vertex(param, hcuts[t][0] * h[0][1], hcuts[t][1] * h[0][1], h[0][0], dss[0], base[0], s);
			glVertex3d(hcuts[t][0] * h[0][1], hcuts[t][1] * h[0][1], h[0][0]);
			atmo_dye_vertex(param, hcuts[t1][0] * h[0][1], hcuts[t1][1] * h[0][1], h[0][0], dss[0], base[1], s);
			glVertex3d(hcuts[t1][0] * h[0][1], hcuts[t1][1] * h[0][1], h[0][0]);
			atmo_dye_vertex(param, hcuts[t1][0] * h[1][1], hcuts[t1][1] * h[1][1], h[1][0], dss[1], base[1], s+1);
			glVertex3d(hcuts[t1][0] * h[1][1], hcuts[t1][1] * h[1][1], h[1][0]);
			atmo_dye_vertex(param, hcuts[t][0] * h[1][1], hcuts[t][1] * h[1][1], h[1][0], dss[1], base[0], s+1);
			glVertex3d(hcuts[t][0] * h[1][1], hcuts[t][1] * h[1][1], h[1][0]);
			glEnd();

			if(s == 0 && ret_amb){
				glGetFloatv(GL_CURRENT_COLOR, ret_amb);
			}
			if(s == 15 && ret_horz){
				glGetFloatv(GL_CURRENT_COLOR, ret_horz);
			}


#if 0 && !defined NDEBUG
			if(t % 2 == 0){
				glBegin(GL_LINES);
				glColor4ub(255,255,255,255);
				glVertex3d(hcuts[t][0] * h[0][1], hcuts[t][1] * h[0][1], h[0][0]);
				glVertex3d(hcuts[t1][0] * h[0][1], hcuts[t1][1] * h[0][1], h[0][0]);
	/*			glVertex3d(hcuts[t][0] * h[1][1], hcuts[t][1] * h[1][1], h[1][0]);*/
				glEnd();
			}
#endif
		}
	}
	glPopMatrix();

	if(g_shader_enable){
		glUseProgram(0);
	}
}

#if 0
#include <clib/colseq/cs2x.h>

extern GLubyte g_diffuse[3];

void colorcsxv(struct cs2x_vertex *csxv, const struct cs2x_vertex *g_csxv, int num, double x, double sp, double cen, double clearness, double dimfactor, double dimness, double height, int lightning, GLubyte *g_diffuse){
	int i;
			for(i = 0; i < num; i++){
#if 1
				int air = /*i != 0 &&*/ i != 7 && i != 8 && i != 10 && i != 4;
				double whiteness = i == 0 ? (1. - x) / (1. + 20. * (height * height)) + x : x;
				COLOR32 col = g_csxv[i].c;
				unsigned char r = COLOR32R(col), g = COLOR32G(col), b = COLOR32B(col), a = COLOR32A(col);
				long rr, rg, rb, ra;
				rr = /*clearness * (sp < 0 ? -sp * r / 2 : 0)*/ + dimfactor * (!air ? r : lightning ? g_diffuse[0] : 256 * (1. - whiteness) + r * whiteness) * (96 * cen * cen / 4 + 32) / 256;
				rr = MIN(255, MAX(0, rr));
				rg = clearness * (sp < 1. / 2. ? ((1. / 2. - sp + .1) / (3. / 2. + .1)) * g : g * .1 / (3. / 2. + .1)) + dimfactor * (!air ? g : lightning ? g_diffuse[0] : 256 * (1. - whiteness) + g * whiteness) * (96 * cen * cen / 4 + 32) / 256;
				rg = MIN(255, MAX(0, rg));
				rb = clearness * (sp < 2. / 3. ? ((2. / 3. - sp + .2) / (5. / 3. + .2)) * b : b * .2 / (5. / 3. + .2)) + dimfactor * (!air ? b : lightning ? g_diffuse[0] : 256 * (1. - whiteness) + b * whiteness) * (96 * cen * cen / 4 + 32) / 256;
				rb = MIN(255, MAX(0, rb));
				ra = clearness * (a != 255 ? (sp < 0 ? 1 : (1. - sp)) * a : 255) + dimfactor * (!air ? a : 255 * (dimness) + a * (1. - dimness));
				ra = MIN(255, MAX(0, ra));
				csxv[i].c = COLOR32RGBA(
					rr/*clearness * (sp < 0 ? -sp * COLOR32R(g_csxv[i].c) / 2 : 0) + dimfactor * (!air ? COLOR32R(g_csxv[i].c) : lightning ? g_diffuse[0] : 256 * (1. - whiteness) + COLOR32R(g_csxv[i].c) * whiteness) * (96 * cen * cen / 4 + 32) / 256*/,
					rg/*clearness * (sp < 1. / 2. ? ((1. / 2. - sp + .1) / (3. / 2. + .1)) * COLOR32G(g_csxv[i].c) : COLOR32G(g_csxv[i].c) * .1 / (3. / 2. + .1)) + dimfactor * (!air ? COLOR32R(g_csxv[i].c) : lightning ? g_diffuse[0] : 256 * (1. - whiteness) + COLOR32G(g_csxv[i].c) * whiteness) * (96 * cen * cen / 4 + 32) / 256*/,
					rb,
					ra);
/*				dummyfunc(&g_csxv[i].c);
				dummyfunc(&csxv[i].c);*/
#if 1
/*				printf("earth %d: %u->%ld %u->%ld %u->%ld %u->%ld\n", i, r, rr, g, rg, b, rb, a, ra);*/
#elif 1
				printf("earth %d: %lg %d %d %d %lg %d\n", i, sp, 1/*COLOR32R(g_csxv[i].c)*/, 1/*COLOR32A(g_csxv[i].c)*/, COLOR32A(csxv[i].c), clearness * (COLOR32A(csxv[i].c) != 255 ? (sp < 0 ? 1 : (1. - sp)) * COLOR32A(csxv[i].c) : 255), a);
#endif
#else
				csxv[i].c = COLOR32RGBA(
					(1. - csxv[i].x) * redness/*(1. / (1.2 + 150 * cen * cen * cen * cen))*/ * (i > 1 && i != 7 && i != 8 && i != 10 && i != 4 ? 256 : 128) / 256 + (sp < 0 ? -sp * COLOR32R(csxv[i].c) / 2 : 0),
					sp < 1. / 2. ? ((1. / 2. - sp + .1) / (3. / 2. + .1)) * COLOR32G(csxv[i].c) : COLOR32G(csxv[i].c) * .1 / (3. / 2. + .1),
					sp < 2. / 3. ? ((2. / 3. - sp + .2) / (5. / 3. + .2)) * COLOR32B(csxv[i].c) : COLOR32B(csxv[i].c) * .2 / (5. / 3. + .2),
					COLOR32A(csxv[i].c) != 255 ? (sp < 0 ? 1 : (1. - sp)) * COLOR32A(csxv[i].c) : 255);
#endif
			}
}
#endif

static int setstarcolor(GLubyte *pr, GLubyte *pg, GLubyte *pb, struct random_sequence *prs, double rvelo, double avelo){
	GLubyte r, g, b;
	int bri, hi;
	{
		double hue, sat;
		hue = drseq(prs);
		hue = (hue + drseq(prs)) * .5 + 1. * rvelo / LIGHT_SPEED;
		sat = 1. - (1. - (.25 + drseq(prs) * .5) /** (hue < 0. ? 1. / (1. - hue) : 1. < hue ? 1. / hue : 1.)*/) / (1. + avelo / LIGHT_SPEED);
		bri = int((127 + rseq(prs) % 128) * (hue < 0. ? 1. / (1. - hue) : 1. < hue ? 1. / hue : 1.));
		hue = rangein(hue, 0., 1.);
		hi = (int)floor(hue * 6.);
		GLubyte f = GLubyte(hue * 6. - hi);
		GLubyte p = GLubyte(bri * (1 - sat));
		GLubyte q = GLubyte(bri * (1 - f * sat));
		GLubyte t = GLubyte(bri * (1 - (1 - f) * sat));
		switch(hi){
			case 0: r = bri, g = t, b = q; break;
			case 1: r = q, g = bri, b = p; break;
			case 2: r = p, g = bri, b = t; break;
			case 3: r = p, g = q, b = bri; break;
			case 4: r = t, g = p, b = bri; break;
			case 5:
			case 6: r = bri, g = p, b = q; break;
			default: r = g = b = bri;
		}
	}
	*pr = r;
	*pg = g;
	*pb = b;
	return bri;
}
static avec3_t bpos;
static amat4_t brot, birot;
static double boffset;
static int btoofar;
static int binside;

#define STARNORMALIZE 1 /* stars are way too far to reach by light speed */

#if STARNORMALIZE
#if 1
static void STN(avec3_t r){
	if(0/*!blackhole_gen*/){
		VECNORMIN(r);
		return;
	}
	else{
	double rad, y;
	int back;
	back = !(0. < VECSP(r, bpos));
	if(!btoofar && !binside && !back){
		avec3_t r1;
		double f;
		MAT4DVP3(r1, brot, r);
		VECNORMIN(r1);
		rad = sqrt(r1[0] * r1[0] + r1[1] * r1[1]);
		if(back)
			rad = 2. - rad;
		y = (binside ? 0. : boffset * boffset / (boffset + rad)) + rad;
		f = back ? rad / y : y / rad;
		r1[0] *= f, r1[1] *= f;
/*		r1[2] = sqrt(1. - r1[0] * r1[0] - r1[1] * r1[1]);*/
		MAT4DVP3(r, birot, r1);
	}
	VECNORMIN(r);
	}
}
#else
#define STN(r) VECNORMIN(r)
#endif
#else
#define STN(r)
#endif

#define STARLIST 0

#ifdef NDEBUG
int g_star_cells = 4;
double g_star_visible_thresh = .3;
double g_star_glow_thresh = .2;
#else
int g_star_cells = 3;
double g_star_visible_thresh = .6;
double g_star_glow_thresh = .5;
#endif

int g_gs_slices = 256;
int g_gs_stacks = 128;
int g_multithread = 1;
int g_gs_always_fine = 0;

#if 0 // Ray-tracing galaxy rendering is temporarily disabled.
GLubyte g_galaxy_field[FIELD][FIELD][FIELDZ][4];

static const GLubyte *negate_hyperspace(const GLubyte src[4], const Viewer *vw, GLubyte buf[4]){
	{
		int k;
		if(g_invert_hyperspace) for(k = 0; k < 3; k++){
			buf[k] = (LIGHT_SPEED < vw->velolen ? LIGHT_SPEED / vw->velolen : 1.) * src[k];
		}
		else
			memcpy(buf, src, 3);
		buf[3] = MIN(src[3], src[3] * GALAXY_DR / vw->dynamic_range);
		return buf;
	}
}

static void gs_vertex_proc(double x, double y, double z){
	avec3_t r;
	double rad;
	int back;
	r[0] = x;
	r[1] = y;
	r[2] = z;
	back = !(0. < VECSP(r, bpos));
	if(!btoofar && !binside && !back){
		avec3_t r1;
		double f;
		MAT4DVP3(r1, brot, r);
		VECNORMIN(r1);
		rad = sqrt(r1[0] * r1[0] + r1[1] * r1[1]);
		if(back)
			rad = 2. - rad;
		y = (binside ? 0. : boffset * boffset / (boffset + rad)) + rad;
		f = back ? rad / y : y / rad;
		r1[0] *= f, r1[1] *= f;
/*		r1[2] = sqrt(1. - r1[0] * r1[0] - r1[1] * r1[1]);*/
		MAT4DVP3(r, birot, r1);
	}
	VECNORMIN(r);
	glVertex3dv(r);
}

static int draw_gs_proc(
	GLubyte (*colbuf)[HDIV][4],
	const GLubyte (*field)[FIELD][FIELDZ][4],
	const Vec3d &plpos,
	double rayinterval,
	int raysamples,
	int slices,
	int hdiv,
	double (*cuts)[2],
	double (*cutss)[2],
	volatile LONG *quitsignal
){
	int k, n;
	int xi, yi;
		for(xi = 0; xi < slices; xi++) for(yi = 0; yi <= hdiv; yi++){
			double sum[4] = {0};
			double divider0 = 0.;
			if(quitsignal && *quitsignal)
				return 1;
			Vec3d dir(cuts[xi][0] * cutss[yi][0], cuts[xi][1] * cutss[yi][0], cutss[yi][1]);
			for(n = 0; n < raysamples; n++){
				int v1[3];
				double f = (n + 1) * rayinterval;
				Vec3d v0 = dir * f + plpos * FIELD / GALAXY_EXTENT;
/*				VECSADD(v0, solarsystempos, 1. * FIELD);*/
				for(k = 0; k < 3; k++)
					v1[k] = floor(v0[k]);
				if(v1[0] <= -HFIELD && dir[0] < 0 || HFIELD-1 <= v1[0] && 0 < dir[0] || v1[1] <= -HFIELD && dir[1] < 0 || HFIELD-1 <= v1[1] && 0 < dir[1] || v1[2] <= -HFIELD && dir[2] < 0 || HFIELD-1 <= v1[2] && 0 < dir[2])
					break;
				if(-HFIELD < v1[0] && v1[0] < HFIELD-1 && -HFIELD < v1[1] && v1[1] < HFIELD-1 && -HFIELDZ < v1[2] && v1[2] < HFIELDZ-1){
					int xj, yj, zj;

					/* weigh closer samples to enable dark nebulae to obscure background starlights. */
					double divider1 = 1. / (1. + rayinterval * sum[3] / 512.);
					divider0 += divider1;
					for(k = 0; k < 4; k++){
						for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
							sum[k] += (k == 3 ? 1. : divider1) * field[v1[0] + HFIELD + xj][v1[1] + HFIELD + yj][v1[2] + HFIELDZ + zj][k]
								* (xj ? v0[0] - v1[0] : 1. - (v0[0] - v1[0]))
								* (yj ? v0[1] - v1[1] : 1. - (v0[1] - v1[1]))
								* (zj ? v0[2] - v1[2] : 1. - (v0[2] - v1[2]));
						}
					}
	/*						colbuf[xi][yi][k] = field[v1[0] + 64][v1[1] + 64][v1[2] + 16][k] * (1. - (v0[0] - v1[0])) * (1. - (v0[1] - v1[1]))
						+ field[v1[0] + 1 + 64][v1[1] + 64][v1[2] + 16][k] * ((v0[0] - v1[0])) * (1. - (v0[1] - v1[1]))
						+ field[v1[0] + 1 + 64][v1[1] + 1 + 64][v1[2] + 16][k] * ((v0[0] - v1[0])) * ((v0[1] - v1[1]))
						+ field[v1[0] + 64][v1[1] + 64][v1[2] + 1 + 16][k] * (1. - (v0[0] - v1[0])) * ((v0[1] - v1[1]));*/
				}
	/*						else
					VEC4NULL(colbuf[xi][yi], cblack);*/
			}
			for(k = 0; k < 4; k++){
				double divider = (divider0 / 4. + 128.) / rayinterval;
				colbuf[xi][yi][k] = (sum[k] / divider < 256 ? sqrt(sum[k] / divider / 256.) * 255 : 255);
			}
		}
		return 0;
}

struct draw_gs_fine_thread_data{
	GLubyte (*field)[FIELD][FIELDZ][4];
	GLubyte (*colbuf)[HDIV][4];
	Viewer *vw;
	double (*cuts)[2], (*cutss)[2];
	Vec3d plpos;
	const Vec3d *solarsystempos;
/*	int lod;*/
	volatile LONG *threaddone;
	volatile LONG threadstop;
	HANDLE drawEvent;
};

static DWORD WINAPI draw_gs_fine_thread(struct draw_gs_fine_thread_data *dat){
	while(WAIT_OBJECT_0 == WaitForSingleObject(dat->drawEvent, INFINITE)){
		GLubyte (*field)[FIELD][FIELDZ][4] = dat->field;
		GLubyte (*colbuf)[HDIV][4] = dat->colbuf;
		Viewer *vw = dat->vw;
		int k, n;
/*		volatile lod = dat->lod;*/
		const Vec3d &solarsystempos = *dat->solarsystempos;
		int slices = SLICES, hdiv = HDIV;
		int xi, yi;
		double (*cuts)[2], (*cutss)[2];
		cuts = dat->cuts;
		cutss = dat->cutss;
/*		if(lod)
			slices = SLICES * 2, hdiv = HDIV * 2;*/
recalc:
		if(draw_gs_proc(colbuf, field, dat->plpos, .5, 1024, slices, hdiv, cuts, cutss, &dat->threadstop)){
			InterlockedExchange(&dat->threadstop, 0);
			goto recalc;
		}
		InterlockedExchange(dat->threaddone, 1);
	}
	return 0;
}

static void perlin_noise_3d(GLubyte (*field)[FIELD][FIELDZ][4], long seed){
	int octave;
	int octaves = 6;
	struct random_sequence rs;
	double (*buf)[FIELD][FIELD][FIELDZ][4];
	int xi, yi, zi;
	int k;
	init_rseq(&rs, seed);
	buf = (double (*)[FIELD][FIELD][FIELDZ][4])malloc(octaves * sizeof *buf);
	for(octave = 0; octave < octaves; octave ++){
		int cell = 1 << octave;
		int res = FIELD / cell;
		int zres = FIELDZ / cell;
		printf("octave %d\n", octave);
		for(xi = 0; xi < res; xi++) for(yi = 0; yi < res; yi++) for(zi = 0; zi < zres; zi++){
			int base;
			base = rseq(&rs) % 128;
			for(k = 0; k < 4; k++)
				buf[octave][xi][yi][zi][k] = (rseq(&rs) % 64 + base) / (double)octaves / (double)(octaves - octave);
		}
	}
	for(xi = 0; xi < FIELD; printf("xi %d\n", xi), xi++) for(yi = 0; yi < FIELD; yi++) for(zi = 0; zi < FIELD; zi++){
		int xj, yj, zj;
		double sum[4] = {0};
		for(k = 0; k < 4; k++) for(octave = 0; octave < octaves; octave ++){
			int cell = 1 << octave;
			int res = FIELD / cell;
			int zres = FIELDZ / cell;
			for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
				sum[k] += (double)buf[octave][(xi / cell + xj) % res][(yi / cell + yj) % res][(zi / cell + zj) % zres][k]
				* (xj ? xi % cell : (cell - xi % cell - 1)) / (double)cell
				* (yj ? yi % cell : (cell - yi % cell - 1)) / (double)cell
				* (zj ? zi % cell : (cell - zi % cell - 1)) / (double)cell;
			}
			field[xi][yi][zi][k] = MIN(255, field[xi][yi][zi][k] + sum[k] / 2);
		}
	}
	free(buf);
}
#endif

static double r_galaxy_brightness = GALAXY_DR;
static bool r_galaxy_blobInit = false;
static double r_galaxy_blob_density = 0.02;
static double r_galaxy_blob_size = 3.;
static double r_galaxy_blob_alpha = 0.075;
static double r_galaxy_blob_repaint_tolerance = 1.;
static double r_galaxy_blob_repaint_wait = 2.;
static int r_galaxy_blob_cube_texture_size = 512;

StaticInitializer draw_gs_init([](){
	CvarAdd("r_galaxy_brightness", &r_galaxy_brightness, cvar_double);
	CmdAdd("r_galaxy_blob_density", [](int argc, char *argv[]){
		if(argc <= 1){
			CmdPrint(gltestp::dstring() << "r_galaxy_blob_density is " << r_galaxy_blob_density);
			return 0;
		}
		double v = atof(argv[1]);
		if(v < 0 || 1 < v){
			CmdPrint(gltestp::dstring() << "Invalid value for r_galaxy_blob_density, must be in (0,1)");
			return 1;
		}
		if(v != r_galaxy_blob_density){
			r_galaxy_blobInit = false;
			r_galaxy_blob_density = v;
		}
		return 0;
	});
	CvarAdd("r_galaxy_blob_size", &r_galaxy_blob_size, cvar_double);
	CvarAdd("r_galaxy_blob_alpha", &r_galaxy_blob_alpha, cvar_double);
	CvarAdd("r_galaxy_blob_repaint_tolerance", &r_galaxy_blob_repaint_tolerance, cvar_double);
	CvarAdd("r_galaxy_blob_repaint_wait", &r_galaxy_blob_repaint_wait, cvar_double);
	CvarAdd("r_galaxy_blob_cube_texture_size", &r_galaxy_blob_cube_texture_size, cvar_int);
});

static void draw_gs_blob(const CoordSys *galaxy, const Viewer *vw){
	const GalaxyField *field = initGalaxyField();
	if(!field)
		return;
	RandomSequence rs(1233441);
	static GLuint texname = 0;

	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT | GL_POINT_BIT);

	glPushMatrix();
	glLoadIdentity();

	Vec3d thispos = galaxy->tocs(vw->pos, vw->cs) - getGalaxyOffset();
	int thisx = int(thispos[0] / (GALAXY_EXTENT / FIELD) + FIELD / 2);
	int thisy = int(thispos[1] / (GALAXY_EXTENT / FIELD) + FIELD / 2);
	int thisz = int(thispos[2] / (GALAXY_EXTENT / FIELD) + FIELDZ / 2);
	Vec3i thisvec(thisx, thisy, thisz);
	static Vec3i lastpos;

	// Closure to draw actual blobs
	auto drawer = [&](const Mat4d &mat, double brightness, bool alwaysConsistentRotation){
		if(!texname){
			suftexparam_t stp;
			stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL | STP_ALPHA;
			stp.env = GL_MODULATE;
			stp.magfil = GL_LINEAR;
			stp.minfil = GL_LINEAR;
			texname = CallCacheBitmap5("textures/smoke2.png", "textures/smoke2.png", &stp, NULL, NULL);
		}
		glCallList(texname);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Alpha blend
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		Mat4d trans = mat4_u.translate(vw->cs->tocs(getGalaxyOffset(), galaxy) - vw->pos);
		trans = trans * vw->cs->tocsm(galaxy);
		struct BlobCache{
			Vec4d color;
			Vec3i pos;
			Vec2d verts[4];
		};
		trans.scalein(GALAXY_EXTENT / FIELD, GALAXY_EXTENT / FIELD, GALAXY_EXTENT / FIELD);
		trans.translatein(-FIELD / 2, -FIELD / 2, -FIELDZ / 2); // Offset center

		static std::vector<BlobCache> blobCache;
		if(!r_galaxy_blobInit){
			r_galaxy_blobInit = true;
			blobCache.clear();
			for(int ix = 0; ix < FIELD; ix++) for(int iy = 0; iy < FIELD; iy++) for(int iz = 0; iz < FIELDZ; iz++){
				const GLubyte *cell = (*field)[ix][iy][iz];
				unsigned intensity = cell[0] + cell[1] + cell[2] + cell[3];
				if(rs.next() % int(256 / r_galaxy_blob_density) < intensity){
					BlobCache bc;
					bc.color = Vec4d(cell[0] / 256., cell[1] / 256., cell[2] / 256., 1);
					double angle = rs.nextd() * M_PI * 2.;
					bc.pos = Vec3i(ix, iy, iz);
					Mat2d rot = Mat2d(cos(angle), sin(angle), -sin(angle), cos(-angle));
					bc.verts[0] = rot.vp(Vec2d(-1, -1));
					bc.verts[1] = rot.vp(Vec2d(-1, 1));
					bc.verts[2] = rot.vp(Vec2d(1, 1));
					bc.verts[3] = rot.vp(Vec2d(1, -1));
					blobCache.push_back(bc);
				}
			}
		}

		if(lastpos != thisvec){
			std::sort(blobCache.begin(), blobCache.end(),
				[&](const BlobCache &a, const BlobCache &b){
					return (b.pos - thisvec).slen() < (a.pos - thisvec).slen();
			});
			lastpos = thisvec;
		}

		int count = (int)blobCache.size();
		int c = count;
		glBegin(GL_QUADS);
		for(auto bc : blobCache){
			c--; // Countdown for near blob determination
			Vec4d color = bc.color;
			color *= brightness;
			color[3] = r_galaxy_blob_alpha;
			glColor4dv(color);
			Vec3d lpos = trans.vp3(bc.pos.cast<double>()); // Local (non-viewpoint aware) position
			Vec3d pos = mat.vp3(lpos);
			if(0 <= pos[2])
				continue;
			double psize = r_galaxy_blob_size;

			// Divide the position by galaxy field unit to make it reasonable range.
			pos /= GALAXY_EXTENT / FIELD;
			Vec3d p3d[4];

			// Consistent rotation calculation is costly, so only performed to blobs that are near
			// the viewpoint.  If alwaysConsistentRotation flag is set, this shortcut is disabled.
			if(alwaysConsistentRotation || c < count / 4 && 0.05 < psize / -pos[2]){
				Mat4d irot = mat * Quatd::direction(lpos).tomat4();
				for(int i = 0; i < 4; i++)
					p3d[i] = irot.vp3(bc.verts[i]) * psize;
			}
			else{
				for(int i = 0; i < 4; i++)
					p3d[i] = bc.verts[i] * psize;
			}

			glTexCoord2i(0, 0); glVertex3dv(pos + p3d[0]);
			glTexCoord2i(0, 1); glVertex3dv(pos + p3d[1]);
			glTexCoord2i(1, 1); glVertex3dv(pos + p3d[2]);
			glTexCoord2i(1, 0); glVertex3dv(pos + p3d[3]);
		}
		glEnd();
	};

	// Distance to assume that the cube background needs be repainted
	const double cellSize = GALAXY_EXTENT / FIELD * r_galaxy_blob_repaint_tolerance;

	// We use clock_t since we do not need too much high precision for just waiting viewpoint settles.
	static clock_t stableTime = 0;
	static Vec3d stablePos = Vec3d(0,0,0);
	if(cellSize * cellSize < (stablePos - thispos).slen()){
		stableTime = clock();
		stablePos = thispos;
	}
	if(clock() < stableTime + r_galaxy_blob_repaint_wait * CLOCKS_PER_SEC) // Wait 2 seconds
		drawer(vw->rot, r_galaxy_brightness * vw->dynamic_range, false);
	else do{
		static GLuint cube[6] = {0};
		static const int cubeWidth = r_galaxy_blob_cube_texture_size; // Cannot be altered while running

		if(!cube[0]){
			// We could use single cube map texture instead of 6 distinct textures, but
			// it's good to start from simpler way.
			glGenTextures(6, cube);
			for(int i = 0; i < 6; i++){
				glBindTexture(GL_TEXTURE_2D, cube[i]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cubeWidth, cubeWidth, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			}
		}

		static Vec3d cubeMapPos = Vec3d(0,0,0);
		// Repaint the cube textures only if the viewpoint move significant distance.
		if(cellSize * cellSize < (cubeMapPos - thispos).slen()){
			// Those extensions should be loaded by GLEW or something
			static PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
			static PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
			static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
			static PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
			if(!glGenFramebuffersEXT || !glBindFramebufferEXT || !glFramebufferTexture2DEXT || !glCheckFramebufferStatusEXT)
				break;

			// We keep the framebuffer object while the program is running.
			static GLuint framebuf = 0;
			if(!framebuf)
				glGenFramebuffersEXT(1, &framebuf);

			// I'm not sure which glPushAttrib() bits reserves the viewport, so we remember manually.
			GLint vp[4];
			glGetIntegerv(GL_VIEWPORT, vp);
			// Viewport must match the texture's dimensions.
			glViewport(0, 0, cubeWidth, cubeWidth);

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuf); // Attach framebuffer

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			double nearp = 1e-10; // We do not use z buffering, so setting extreme value here is valid.
			// Reset projection matrix since the default one is distorted to match screen dimensions
			glFrustum(-nearp, nearp, -nearp, nearp, nearp, 1e10);
			glMatrixMode(GL_MODELVIEW);
			for(int i = 0; i < numof(DrawTextureSphere::cubedirs); i++){
				// Attach each texture to the framebuffer
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, cube[i], 0);
				GLenum ret = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
				if(ret != GL_FRAMEBUFFER_COMPLETE_EXT)
					break;
				glClear(GL_COLOR_BUFFER_BIT);
				drawer(DrawTextureSphere::cubedirs[i].cnj().tomat4(), 1, true);
			}
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); // Detach framebuffer

			// Restore the original viewport
			glViewport(vp[0], vp[1], vp[2], vp[3]);

			cubeMapPos = thispos;
		}

		// Actually draw the background cube.
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		Vec4d color = Vec4d(1,1,1,1) * r_galaxy_brightness * vw->dynamic_range;
		color[3] = 1;
		glColor4dv(color);
		for(int i = 0; i < numof(DrawTextureSphere::cubedirs); i++){
			glBindTexture( GL_TEXTURE_2D, cube[i] );
			glPushMatrix();
			glLoadMatrixd(vw->rot);
			gldMultQuat(DrawTextureSphere::cubedirs[i]);
			glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex3i(-1, -1, -1);
			glTexCoord2i(1, 0); glVertex3i( 1, -1, -1);
			glTexCoord2i(1, 1); glVertex3i( 1,  1, -1);
			glTexCoord2i(0, 1); glVertex3i(-1,  1, -1);
			glEnd();
			glPopMatrix();
		}
	} while(0);

	glPopMatrix();
	glPopAttrib();
}

static void draw_gs(const CoordSys *csys, const Viewer *vw){
	draw_gs_blob(csys, vw);
	return;
#if 0 // Ray-tracing rendering is temporarily disabled.
	{
		static GLuint spheretex = 0;
		static GLuint spheretexs[4] = {0};
		int xi, yi;
		double (*cuts)[2], (*cutss)[2];
		GLubyte cblack[4] = {0};
		static GLubyte colbuf[SLICES][HDIV][4];
		static GLubyte finecolbuf[SLICES][HDIV][4];
		static Vec3d lastpos(1e30, 1e30, 1e30);
		static int fresh = 0, firstload = 0, firstwrite = 0;
		static HANDLE ht;
		static DWORD tid = 0;
		static HANDLE drawEvent;
		static int threadrun = 0;
		static volatile LONG threaddone = 0;
		static Viewer svw;
		int recalc;
		int slices, hdiv;
		int firstloaded = 0;
		FILE *fp;

		Vec3d plpos = csys->tocs(vw->pos, vw->cs) + solarsystempos;
		int refresh = (GALAXY_EXTENT * GALAXY_EPSILON / FIELD) * (GALAXY_EXTENT * GALAXY_EPSILON / FIELD) < (lastpos - plpos).slen();
		if(!firstload){
			firstload = 1;
			if(ftimecmp("cache/galaxy.bmp", "ourgalaxy3.raw") > 0 && (fp = fopen("cache/galaxy.bmp", "rb"))){
				BITMAPFILEHEADER fh;
				BITMAPINFOHEADER bi;
				fread(&fh, 1, sizeof(BITMAPFILEHEADER), fp);
				fread(&bi, 1, sizeof bi, fp);
				fread(finecolbuf, 1, bi.biSizeImage, fp);
				fclose(fp);
				refresh = 0;
				firstloaded = firstwrite = 1;
				lastpos = plpos;
			}
		}
		fresh = (fresh << 1) | refresh;
		bool detail = g_gs_always_fine || !(fresh & 0xfff);
		if(g_multithread){
			static struct draw_gs_fine_thread_data dat;
			if(!ht){
				dat.drawEvent = drawEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
				ht = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)draw_gs_fine_thread, &dat, 0, &tid);
			}
			if(!threadrun && (fresh & 0x1fff) == 0x1000){
				dat.field = field;
				dat.colbuf = finecolbuf;
				dat.vw = &svw; svw = *vw;
				dat.threaddone = &threaddone;
				dat.threadstop = 0;
				dat.plpos = plpos;
				dat.solarsystempos = &solarsystempos;
				dat.cuts = CircleCuts(SLICES);
				dat.cutss = CircleCuts(HDIV * 2);
				threaddone = 0;
				SetEvent(dat.drawEvent);
				threadrun = 1;
			}
			if(threadrun && (fresh & 0x1fff) == 0x1000){
				dat.plpos = plpos;
				InterlockedExchange(&dat.threadstop, 1);
			}
			recalc = refresh;
		}
		else if(g_gs_always_fine){
			recalc = refresh;
			detail = 1;
		}
		else{
			recalc = ((fresh & 0x1fff) == 0x1000) || refresh;
		}
		if(!detail)
			slices = SLICES1, hdiv = HDIV1;
		else
			slices = SLICES2, hdiv = HDIV2;
		if(firstloaded || threadrun && threaddone){
//			extern int g_reflesh_cubetex;
			memcpy(colbuf, finecolbuf, sizeof colbuf);
			glBindTexture(GL_TEXTURE_2D, spheretex);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SLICES, HDIV, GL_RGBA, GL_UNSIGNED_BYTE, finecolbuf);
#if 1
			if(!firstwrite && ftimecmp("cache/galaxy.bmp", "ourgalaxy3.raw") < 0 && (fp = fopen("cache/galaxy.bmp", "wb"))){
				BITMAPFILEHEADER fh;
				BITMAPINFOHEADER bi;
				((char*)&fh.bfType)[0] = 'B';
				((char*)&fh.bfType)[1] = 'M';
				fh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof bi + SLICES * HDIV * 4;
				fh.bfReserved1 = fh.bfReserved2 = 0;
				fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
				fwrite(&fh, 1, sizeof(BITMAPFILEHEADER), fp);
				bi.biSize = sizeof bi;
				bi.biWidth = SLICES;
				bi.biHeight = HDIV;
				bi.biPlanes = 1;
				bi.biBitCount = 32;
				bi.biCompression = BI_RGB;
				bi.biSizeImage = bi.biWidth * bi.biHeight * 4;
				bi.biXPelsPerMeter = bi.biYPelsPerMeter = 0;
				bi.biClrUsed = bi.biClrImportant = 0;
				fwrite(&bi, 1, sizeof bi, fp);
				fwrite(finecolbuf, 1, bi.biSizeImage, fp);
				fclose(fp);
				firstwrite = 1;
			}
#endif
			detail = 1;
			slices = SLICES2, hdiv = HDIV2;
//			g_reflesh_cubetex = 1;
			threadrun = 0;
			lastpos = plpos;
		}
		if(threadrun && !threaddone){
			detail = 0;
			slices = SLICES1, hdiv = HDIV1;
		}
		cuts = CircleCuts(slices);
		cutss = CircleCuts(hdiv * 2);
		if(!(threadrun && threaddone) && recalc){
#if 1
			if(detail){
				draw_gs_proc(colbuf, field, plpos, .5, 1024, SLICES, HDIV, CircleCuts(SLICES), CircleCuts(HDIV * 2), NULL);
				glBindTexture(GL_TEXTURE_2D, spheretex);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SLICES, HDIV, GL_RGBA, GL_UNSIGNED_BYTE, colbuf);
			}
			else
				draw_gs_proc(colbuf, field, plpos, 4., 128, slices, hdiv, cuts, cutss, NULL);
#else
			for(xi = 0; xi < slices; xi++) for(yi = 0; yi <= hdiv; yi++){
				avec3_t v0;
				int v1[3];
				int k, n;
				double sum[4] = {0};
				for(n = 0; n < FIELDZ; n++){
					double f = (n + 1) * 2;
					v0[0] = cuts[xi][0] * cutss[yi][0] * f, v0[1] = cuts[xi][1] * cutss[yi][0] * f, v0[2] = cutss[yi][1] * f;
					VECSADD(v0, plpos, FIELD / GALAXY_EXTENT);
					VECSADD(v0, solarsystempos, 1. * FIELD);
					for(k = 0; k < 3; k++)
						v1[k] = floor(v0[k]);
					if(-HFIELD < v1[0] && v1[0] < HFIELD-1 && -HFIELD < v1[1] && v1[1] < HFIELD-1 && -HFIELDZ < v1[2] && v1[2] < HFIELDZ-1){
						int xj, yj, zj;
						for(k = 0; k < 4; k++){
							for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++) for(zj = 0; zj <= 1; zj++){
								sum[k] += field[v1[0] + HFIELD + xj][v1[1] + HFIELD + yj][v1[2] + HFIELDZ + zj][k]
									* (xj ? v0[0] - v1[0] : 1. - (v0[0] - v1[0]))
									* (yj ? v0[1] - v1[1] : 1. - (v0[1] - v1[1]))
									* (zj ? v0[2] - v1[2] : 1. - (v0[2] - v1[2]));
							}
						}
	/*						colbuf[xi][yi][k] = field[v1[0] + 64][v1[1] + 64][v1[2] + 16][k] * (1. - (v0[0] - v1[0])) * (1. - (v0[1] - v1[1]))
							+ field[v1[0] + 1 + 64][v1[1] + 64][v1[2] + 16][k] * ((v0[0] - v1[0])) * (1. - (v0[1] - v1[1]))
							+ field[v1[0] + 1 + 64][v1[1] + 1 + 64][v1[2] + 16][k] * ((v0[0] - v1[0])) * ((v0[1] - v1[1]))
							+ field[v1[0] + 64][v1[1] + 64][v1[2] + 1 + 16][k] * (1. - (v0[0] - v1[0])) * ((v0[1] - v1[1]));*/
					}
	/*						else
						VEC4NULL(colbuf[xi][yi], cblack);*/
				}
				for(k = 0; k < 4; k++){
					static const double divider = 4 * 32.;
					colbuf[xi][yi][k] = (sum[k] / divider < 256 ? sqrt(sum[k] / divider / 256.) * 255 : 255);
				}
			}
#endif
			VECCPY(lastpos, plpos);
		}
		if(detail){
			int slices2 = slices, hdiv2 = hdiv;
			glPushAttrib(GL_TEXTURE_BIT);
			glBindTexture(GL_TEXTURE_2D, spheretex);
			glEnable(GL_TEXTURE_2D);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glBegin(GL_QUADS);
			glColor4ub(255,255,255, 255/*MIN(255, 255 * GALAXY_DR / vw->dynamic_range)*/);
			for(xi = 0; xi < slices2; xi++) for(yi = 0; yi < hdiv2; yi++){
				int xi1 = (xi + 1) % slices2, yi1 = yi + 1;
				glTexCoord2d((double)yi / slices2, (double)xi / slices2);
				gs_vertex_proc(cuts[xi][0] * cutss[yi][0], cuts[xi][1] * cutss[yi][0], cutss[yi][1]);
				glTexCoord2d((double)yi / slices2, (double)(xi+1) / slices2);
				gs_vertex_proc(cuts[xi1][0] * cutss[yi][0], cuts[xi1][1] * cutss[yi][0], cutss[yi][1]);
				glTexCoord2d((double)yi1 / slices2, (double)(xi+1) / slices2);
				gs_vertex_proc(cuts[xi1][0] * cutss[yi1][0], cuts[xi1][1] * cutss[yi1][0], cutss[yi1][1]);
				glTexCoord2d((double)yi1 / slices2, (double)xi / slices2);
				gs_vertex_proc(cuts[xi][0] * cutss[yi1][0], cuts[xi][1] * cutss[yi1][0], cutss[yi1][1]);
			}
			glEnd();
			glBindTexture(GL_TEXTURE_2D, 0);
			glPopAttrib();
		}
		else{
			glBegin(GL_QUADS);
			for(xi = 0; xi < slices; xi++) for(yi = 0; yi < hdiv; yi++){
				int xi1 = (xi + 1) % slices, yi1 = yi + 1;
				GLubyte buf[4];
	/*					avec3_t v0;
				int v1[3];
				v0[0] = cuts[xi][0] * cuts[yi][0] * 10, v0[1] = cuts[xi][1] * cuts[yi][0] * 10, v0[2] = cuts[yi][1] * 10;
				VECSADD(v0, vw->pos, 1e-4);
				VECCPY(v1, v0);
				if(-64 < v1[0] && v1[0] < 64 && -64 < v1[1] && v1[1] < 64 && -16 < v1[2] && v1[2] < 16)*/
				{
	/*						glColor4ubv(field[v1[0] + 64][v1[1] + 64][v1[2] + 16]);*/
					glColor4ubv(negate_hyperspace(colbuf[xi][yi], vw, buf));
					gs_vertex_proc(cuts[xi][0] * cutss[yi][0], cuts[xi][1] * cutss[yi][0], cutss[yi][1]);
					glColor4ubv(negate_hyperspace(colbuf[xi1][yi], vw, buf));
					gs_vertex_proc(cuts[xi1][0] * cutss[yi][0], cuts[xi1][1] * cutss[yi][0], cutss[yi][1]);
					glColor4ubv(negate_hyperspace(colbuf[xi1][yi1], vw, buf));
					gs_vertex_proc(cuts[xi1][0] * cutss[yi1][0], cuts[xi1][1] * cutss[yi1][0], cutss[yi1][1]);
					glColor4ubv(negate_hyperspace(colbuf[xi][yi1], vw, buf));
					gs_vertex_proc(cuts[xi][0] * cutss[yi1][0], cuts[xi][1] * cutss[yi1][0], cutss[yi1][1]);
				}
			}
			glEnd();
		}
	}
#endif
}

#define TEXSIZE 64
static GLuint drawStarTexture(){
	static GLubyte bits[TEXSIZE][TEXSIZE][2];
	int i, j;
	GLuint texname;
	glGenTextures(1, &texname);
	glBindTexture(GL_TEXTURE_2D, texname);
	for(i = 0; i < TEXSIZE; i++) for(j = 0; j < TEXSIZE; j++){
		int di = i - TEXSIZE / 2, dj = j - TEXSIZE / 2;
		int sdist = di * di + dj * dj;
		bits[i][j][0] = /*255;*/
		bits[i][j][1] = GLubyte(TEXSIZE * TEXSIZE / 2 / 2 <= sdist ? 0 : 255 - 255 * pow((double)(di * di + dj * dj) / (TEXSIZE * TEXSIZE / 2 / 2), 1. / 8.));
	}
	glTexImage2D(GL_TEXTURE_2D, 0, 2, TEXSIZE, TEXSIZE, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, bits);
	return texname;
}

void drawstarback(const Viewer *vw, const CoordSys *csys, const Astrobj *pe, const Astrobj *sun){
	static int init = 0;
	static GLuint listBright, listDark;
	double height;
	const Mat4d &irot = vw->irot;
	static int invokes = 0;
	int drawnstars = 0;
	timemeas_t tm;
	double velolen;
	Vec3d velo;
	Astrobj *blackhole = NULL;

	TimeMeasStart(&tm);

	velo = csys->tocsv(vw->velo, vw->pos, vw->cs);
	velolen = velo.len();

	/* calculate height from the planet's surface */
	if(pe){
		Vec3d epos;
		epos = vw->cs->tocs(pe->pos, pe->parent);
		height = (epos - vw->pos).len() - pe->rad;
	}
	else
		height = 1e10;

	invokes++;
	{
		Mat4d mat;
		glPushMatrix();
		mat = csys->tocsm(vw->cs);
		glMultMatrixd(mat);
	}

#if 0
	static GLuint backimg = 0;
	if(!backimg)
		backimg = DrawTextureSphere::ProjectSphereCubeImage("mwpan2_Merc_2000x1200.jpg", 0);
	{
		glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
		glCallList(backimg);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		double br = r_galaxy_back_brightness * r_exposure;
		glColor4f(br, br, br, 1.);
//		glColor4f(.5, .5, .5, 1.);
		for(int i = 0; i < numof(DrawTextureSphere::cubedirs); i++){
			glPushMatrix();
			gldMultQuat(DrawTextureSphere::cubedirs[i]);
			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			gldMultQuat(DrawTextureSphere::cubedirs[i]);
			glBegin(GL_QUADS);
			glTexCoord3i(-1,-1,-1); glVertex3i(-1, -1, -1);
			glTexCoord3i( 1,-1,-1); glVertex3i( 1, -1, -1);
			glTexCoord3i( 1, 1,-1); glVertex3i( 1,  1, -1);
			glTexCoord3i(-1, 1,-1); glVertex3i(-1,  1, -1);
			glEnd();
/*			int in[4];
			glGetIntegerv(GL_ATTRIB_STACK_DEPTH, &in[0]);
			glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &in[1]);
			glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &in[2]);
			glGetIntegerv(GL_TEXTURE_STACK_DEPTH, &in[3]);
			printf("amp %d,%d,%d,%d\n", in[0], in[1], in[2], in[3]);*/
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
		glPopAttrib();
	}
#endif

/*	{
		int i;
		extern struct astrobj **astrobjs;
		extern int nastrobjs;
		for(i = nastrobjs-1; 0 <= i; i--) if(astrobjs[i]->flags & AO_BLACKHOLE){
			blackhole = astrobjs[i];
			break;
		}
	}

	if(blackhole){
		avec3_t bpos1;
		tocs(bpos1, vw->cs, blackhole->pos, blackhole->cs);
		VECSUBIN(bpos1, vw->pos);
		VECCPY(bpos, bpos1);
	}


	bdist = VECLEN(bpos);
	btoofar = 0;
	if(!blackhole || 1e5 < bdist / blackhole->rad){
		tangent = 0.;
		boffset = 0.;
		btoofar = 1;
	}
	else if(blackhole->rad < bdist){
		tangent = asin(blackhole->rad / bdist);
		boffset = tan(tangent);
		binside = 0;
	}
	else
		binside = 1;

	{
		amat4_t mat, mat2;
		gldLookatMatrix(mat, bpos);
		tocsim(mat2, csys, vw->cs);
		mat4mp(birot, mat2, mat);
		MAT4TRANSPOSE(brot, birot);*/
/*		glPushMatrix();
		directrot(bpos, avec3_000, mat);
		glPopMatrix();
		MAT4CPY(birot, mat);*/
//	}
/*	MAT4TRANSPOSE(brot, birot);*/

/*		glBegin(GL_LINES);
		glColor4ub(0,255,0,255);
		for(i = 0; i < 128; i++){
			int j;
			for(j = 0; j < 128; j++){
				glVertex3d(i - 64 - vw->pos[0] * 1e-4, j - 64 - vw->pos[1] * 1e-4, -16 - vw->pos[2] * 1e-4);
				glVertex3d(i - 64 - vw->pos[0] * 1e-4, j - 64 - vw->pos[1] * 1e-4,  16 - vw->pos[2] * 1e-4);
			}
		}
		for(i = 0; i < 128; i++){
			int j;
			for(j = 0; j < 32; j++){
				glVertex3d(i - 64 - vw->pos[0] * 1e-4,  64 - vw->pos[1] * 1e-4, j - 16 - vw->pos[2] * 1e-4);
				glVertex3d(i - 64 - vw->pos[0] * 1e-4, -64 - vw->pos[1] * 1e-4, j - 16 - vw->pos[2] * 1e-4);
				glVertex3d( 64 - vw->pos[0] * 1e-4, i - 64 - vw->pos[1] * 1e-4, j - 16 - vw->pos[2] * 1e-4);
				glVertex3d(-64 - vw->pos[0] * 1e-4, i - 64 - vw->pos[1] * 1e-4, j - 16 - vw->pos[2] * 1e-4);
			}
		}
		glEnd();*/

/**		glBegin(GL_LINES);
		{
			int xi, yi, zi;
			for(xi = 0; xi < 128; xi++) for(yi = 0; yi < 128; yi++) for(zi = 0; zi < 32; zi++){
				glColor4ub(xi * 256 / 128, yi * 256 / 128, zi * 256 / 32, 255);
				glVertex3d(xi - 64 - vw->pos[0] * 1e-4, yi - 64 - vw->pos[1] * 1e-4, zi - 16 - vw->pos[2] * 1e-4 + .25);
				glVertex3d(xi - 64 - vw->pos[0] * 1e-4, yi - 64 - vw->pos[1] * 1e-4, zi - 16 - vw->pos[2] * 1e-4 - .25);
			}
		}
		glEnd();*/

		draw_gs(csys, vw);

#if STARLIST
	if(!init)
#endif
	{
		static struct cs2x *cs = NULL;
		static struct cs2x_vertex_ex{
			double pos[3];
			COLOR32 c;
		} *ver;
//		FILE *fp;
		double velo;
		velo = vw->relative ? vw->velolen : 0.;
#if STARLIST
		init = 1;
		listBright = glGenLists(2);
		listDark = listBright + 1;

		glNewList(listBright, GL_COMPILE);
#endif

#if 1
#define GLOWTHRESH g_star_glow_thresh
#define VISIBLETHRESH g_star_visible_thresh
#define NUMCELLS g_star_cells
#ifdef NDEBUG
/*
#define NUMGLOWSTARS 4500
#define NUMCELLS 6*/
/*#define GLOWTHRESH 3.
#define VISIBLETHRESH .2*/
#else
/*
#define NUMGLOWSTARS 2
#define NUMCELLS 5*/
/*#define GLOWTHRESH 6.
#define VISIBLETHRESH .5*/
#endif
		{
		Mat4d relmat;
		int pointstart = 0, glowstart = 0, nearest = 0, gx = 0, gy = 0, frameinvokes = 0;
		double radiusfactor = .03 * 1. / (1. + 10. / height) * 512. / vw->vp.m;
//		GLcull glc;
		Vec3d gpos;
		Vec3d npos;
		GLubyte nearest_color[4];
/*		int v1[3];*/

#if 0
		if(pe){
			double sp;
			avec3_t sunp, earthp, vpos;
			tocs(vpos, vw->cs, vw->pos, vw->cs);
			tocs(sunp, vw->cs, sun->pos, sun->cs);
			tocs(earthp, vw->cs, pe->pos, pe->cs);
			VECSUBIN(sunp, vpos);
			VECSUBIN(earthp, vpos);
			sp = (.5 + VECSP(sunp, earthp) / VECLEN(sunp) / VECLEN(earthp));
			if(sp < 0) sp = 0.;
			else if(1. < sp) sp = 1.;
			radiusfactor = .03 * ((1. - sp) / (1. + 10. / height) + sp) * 512. / gvp.m;
		}
		else{
			radiusfactor = .03 * (2.) * 512. / gvp.m;
		}
#endif

		Vec3d plpos = csys->tocs(vw->pos, vw->cs);

		glPushMatrix();
		glLoadIdentity();
//		glcullInit(&glc, g_glcull.fov, avec3_000, mat4identity, g_glcull.znear, g_glcull.zfar);
		glPopMatrix();

/*		tocs(v0, csys, vw->pos, vw->cs);
		VECSCALEIN(v0, FIELD / GALAXY_EXTENT);*/
/*		VECCPY(v1, v0);*/
/*		printf("stardensity(%lg,%lg,%lg)[%d,%d,%d]: %lg\n", v0[0], v0[1], v0[2], v1[0], v1[1], v1[2], (0 <= v1[0] && v1[0] < FIELD && 0 <= v1[1] && v1[1] < FIELD && 0 <= v1[2] && v1[2] < FIELDZ ? g_galaxy_field[v1[0]][v1[1]][v1[2]][3] / 256. : 0.));*/

		glPushAttrib(GL_TEXTURE_BIT | GL_POINT_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT);

		Vec3d pos;
		StarEnum se(plpos, NUMCELLS);
		StarCache *sc;
		while(se.next(pos, &sc)){
			// Skip drawing materialized stars or solar systems
			if(sc && sc->system)
				continue;
			double rvelo;
			double radius = radiusfactor;
			GLubyte r, g, b;
			int bri, current_nearest = 0;

			pos -= plpos;
			pos /= StarEnum::sectorSize;

			radius /= 1. + VECLEN(pos);

#if 1
/*			if(glcullFrustum(&pos, 0., &g_glcull)){
				int j;
				for(j = 0; j < 4; j++)
					rseq(&rs);
			}*/
#else
			{
				double hsv[3];
				r = 127 + rseq(&rs) % 128;
				g = 127 + rseq(&rs) % 128;
				b = 127 + rseq(&rs) % 128;
				rgb2hsv(hsv, r / 256., g / 256., b / 256.);
				bri = hsv[2] * 255;
			}
#endif

			if(VECSLEN(pos) < 1e-6 * 1e-6){
				VECSCALE(npos, pos, 1e14);
				current_nearest = nearest = 1;
				radius /= VECLEN(pos) * 1e6;
			}

			STN(pos);
			rvelo = vw->relative ? VECSP(pos, vw->velo) : 0.;
			frameinvokes++;
			if(GLOWTHRESH < radius * vw->vp.m / vw->fov/* && 1. < gvp.m * radiusfactor * .2*/){
				static GLuint texname = 0, list;
				if(pointstart){
					glEnd();
					pointstart = 0;
				}
				if(!glowstart){
					Mat4d csrot;
					glowstart = 1;
					if(!texname){
						texname = drawStarTexture();
					/*	glNewList(list = glGenLists(1), GL_COMPILE);
						glPushAttrib(GL_TEXTURE_BIT | GL_PIXEL_MODE_BIT);
						glBindTexture(GL_TEXTURE_2D, texname);
						glPixelTransferf(GL_RED_BIAS, color[0] / 256.F);
						glPixelTransferf(GL_GREEN_BIAS, color[1] / 256.F);
						glPixelTransferf(GL_BLUE_BIAS, color[2] / 256.F);
						glEndList();*/
					}
					else{
						glBindTexture(GL_TEXTURE_2D, texname);
					}
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
						GL_NEAREST);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
						GL_NEAREST);
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, texname);
					glPushMatrix();
					glGetDoublev(GL_MODELVIEW_MATRIX, relmat);
					csrot = vw->cs->tocsim(csys);
					relmat = vw->relrot * csrot;
				}
				{
					static const GLfloat col[4] = {1., 1., 1., 1.};
					Vec3d local = relmat.vp3(pos);
					if(local.slen() < FLT_EPSILON)
						break;
					local.normin();
/*					if(!current_nearest && glcullFrustum(local, radius, &glc)){
						int j;
						for(j = 0; j < 4; j++)
							rseq(&rs);
						continue;
					}*/
					bri = setstarcolor(&r, &g, &b, se.getRseq(), rvelo, velo);
					radius *= bri / 256.;
					drawnstars++;
/*					col[0] = r / 256.F;
					col[1] = g / 256.F;
					col[2] = b / 256.F;
					col[3] = 1.F;*/
/*					if(g_invert_hyperspace && LIGHT_SPEED < velolen){
						VECSCALEIN(col, GLfloat(LIGHT_SPEED / velolen));
						r *= LIGHT_SPEED / velolen;
						g *= LIGHT_SPEED / velolen;
						b *= LIGHT_SPEED / velolen;
					}*/
					if(current_nearest){
						nearest_color[0] = r;
						nearest_color[1] = g;
						nearest_color[2] = b;
						nearest_color[3] = 255;
					}
					glColor4ub(r, g, b, std::min(255, int(bri * r_exposure * r_star_back_brightness)));
					glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, col);
/*					glPushMatrix();*/
/*					glTranslated(pos[0], pos[1], pos[2]);
					glMultMatrixd(irot);
					glScaled(radius, radius, radius);*/
//					printf("%lg %lg %lg\n", local[0], local[1], local[2]);
					glLoadIdentity();
					gldTranslate3dv(local);
					glScaled(radius, radius, radius);
					glBegin(GL_QUADS);
					glTexCoord2i(0, 0); glVertex2i(-1, -1);
					glTexCoord2i(1, 0); glVertex2i( 1, -1);
					glTexCoord2i(1, 1); glVertex2i( 1,  1);
					glTexCoord2i(0, 1); glVertex2i(-1,  1);
					glEnd();
/*					glPopMatrix();*/
				}
/*				gldTextureGlow(pos, .03 * bri / 256, rgb, vw->irot);*/
			}
			else if(VISIBLETHRESH < radius * vw->vp.m / vw->fov){
				double f = radius * vw->vp.m / vw->fov  / GLOWTHRESH;
				if(glowstart){
					glPopMatrix();
					glDisable(GL_TEXTURE_2D);
					glowstart = 0;
				}
				if(!pointstart){
					glBegin(GL_POINTS);
					pointstart = 1;
				}
				bri = setstarcolor(&r, &g, &b, se.getRseq(), rvelo, velo);
				if(g_invert_hyperspace && LIGHT_SPEED < vw->velolen){
					r = GLubyte(r * LIGHT_SPEED / vw->velolen);
					g = GLubyte(g * LIGHT_SPEED / vw->velolen);
					b = GLubyte(b * LIGHT_SPEED / vw->velolen);
				}
				glColor4f(r / 255.f, g / 255.f, b / 255.f, GLfloat(255 * f * f));
				glVertex3dv(pos);
			}
		}
		if(glowstart){
			glPopMatrix();
		}
		if(pointstart)
			glEnd();
		glPopAttrib();
#endif

#if STARLIST
		glEndList();
#endif

		if(nearest){
			gldPseudoSphere(npos, 1e5, nearest_color);
		}
		}
	}

#if STARLIST
	/* don't ever think about constellations' collapsing if you are so close to the
	  earth. but aberration takes effect if your velocity is so high. */
#if 0 /* old aberration by vector arithmetics */
	if(1e5 < height || LIGHT_SPEED / 100. < pl.velolen){
		const double c = LIGHT_SPEED;
		int i;
		struct random_sequence rs;
		double velo;
		double velo, gamma;

		/* Lorentz transformation */
		velo = VECLEN(pl.velo);
		gamma = sqrt(1 - velo * velo / c / c);

		init_rseq(&rs, 1);

		glBegin(GL_POINTS);
		for(i = 0; i < 500; i++){
			{
				GLubyte r, g, b;
				r = 127 + rseq(&rs) % 128;
				g = 127 + rseq(&rs) % 128;
				b = 127 + rseq(&rs) % 128;
				glColor3ub(r, g, b);
			}
			{
				double r[3], v[3], dist, vlen, theta;
				r[0] = 10. * (drseq(&rs) * 2. - 1.);
				r[1] = drseq(&rs) * 2. - 1.;
				r[2] = drseq(&rs) * 2. - 1.;
				if(velo < LIGHT_SPEED){
					double sp, s1, s2;
					VECNORMIN(r);
					VECSCALE(v, r, c * gamma);
					sp = VECSP(pl.velo, r);
					s1 = 1 + sp / (c * (1. + gamma));
					VECSADD(v, pl.velo, s1);
					s2 = 1. / (c + VECSP(pl.velo, r));
					VECSCALE(v, v, s2);
					glVertex3dv(v);
				}
			}
		}
		glEnd();
	}
#else
	else if(1){
		glCallList(listBright);
		glCallList(listDark);
	}
#endif
	else
		glCallList(listBright);
#endif

	glPopMatrix();

	// There's better method to output draw time.
/*	printf("stars[%d]: %lg\n", drawnstars, TimeMeasLap(&tm));*/
	{
		// We plot the time it takes to draw the stars.
		double value = TimeMeasLap(&tm);
		GLwindow *w = glwlist;
		for(; w; w = w->getNext()){
			if(w->classname() && !strcmp(w->classname(), "GLWchart")){
				static_cast<GLWchart*>(w)->addSample("drawstartime", value);
				static_cast<GLWchart*>(w)->addSample("drawstarcount", drawnstars);
			}
		}
	}

#if 1
	if(!vw->relative/*(pl.mover == warp_move || pl.mover == player_tour || pl.chase && pl.chase->vft->is_warping && pl.chase->vft->is_warping(pl.chase, pl.cs->w))*/ && LIGHT_SPEED * LIGHT_SPEED < velo.slen()){
		int i, count = 1250;
		RandomSequence rs(2413531);
		static double raintime = 0.;
		double cellsize = 1e7;
		double speedfactor;
		const double width = .00002 * 1000. / vw->vp.m; /* line width changes by the viewport size */
		double basebright;
		const double levelfactor = 1.5e3;
		int level = 0;
		const Vec3d nh0(0., 0., -1.);
		Mat4d irot, rot;
		GLubyte col[4] = {255, 255, 255, 255};
		static GLuint texname = 0;
		static const GLfloat envcolor[4] = {0,0,0,0};

/*		count = raintime / dt < .1 ? 500 * (1. - raintime / dt / .1) * (1. - raintime / dt / .1) : 100;*/
/*		TimeMeasStart(&tm);*/
		rot = csys->tocsm(vw->cs);
		Vec3d plpos = csys->tocs(vw->pos, vw->cs);
#if 0
		tocsv(velo, csys, /*pl.chase ? pl.chase->velo :*/ vw->velo, vw->pos, vw->cs);
#elif 1
		Vec3d localvelo = velo;
#else
		mat4dvp3(localvelo, rot, velo);
#endif
		velolen = localvelo.len();
		Vec3d nh = vw->irot.dvp3(nh0);
		glPushMatrix();
		glLoadMatrixd(vw->rot);
		glMultMatrixd(rot);
		glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
		if(!texname){
			GLubyte texbits[64][64][2];
			int i, j;
			for(i = 0; i < 64; i++) for(j = 0; j < 64; j++){
				int r;
				r = (32 * 32 - (i - 32) * (i - 32) - (j - 32) * (j - 32)) * 255 / 32 / 32;
				texbits[i][j][0] = GLubyte(256. * sqrt(MIN(255, MAX(0, r)) / 256.)/*256. * sqrt((255 - MIN(255, MAX(0, r))) / 256.)*/);
				r = (32 * 32 - (i - 32) * (i - 32) - (j - 32) * (j - 32)) * 255 / 32 / 32;
				texbits[i][j][1] = GLubyte(256. * sqrt(MIN(255, MAX(0, r)) / 256.));
			}
			glGenTextures(1, &texname);
			glBindTexture(GL_TEXTURE_2D, texname);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 64, 64, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, texbits);
		}
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcolor);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBindTexture(GL_TEXTURE_2D, texname);
		glDepthMask(0);
		/*glTranslated*//*(plpos[0] = floor(pl.pos[0] / cellsize) * cellsize, plpos[1] = floor(pl.pos[1] / cellsize) * cellsize, plpos[2] = floor(pl.pos[2] / cellsize) * cellsize);*/

		if(velolen < LIGHT_SPEED * levelfactor);
		else if(velolen < LIGHT_SPEED * levelfactor * levelfactor){
			level = 1;
			cellsize *= levelfactor;
		}
		else{
			level = 2;
			cellsize *= levelfactor * levelfactor;
		}
		speedfactor = velolen / LIGHT_SPEED / (level == 0 ? 1. : level == 1 ? levelfactor : levelfactor * levelfactor);
		basebright = .5 / (.998 + .002 * speedfactor) * (1. - 2. / (speedfactor + 1.));

		localvelo *= .025 * (velolen + cellsize / 100. / .025) / velolen;

		for(i = count; i; i--){
			Vec3d pos, dst;
			GLfloat red;
			pos[0] = plpos[0] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[0] = floor(pos[0] / cellsize) * cellsize + cellsize / 2. - pos[0];
			pos[1] = plpos[1] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[1] = floor(pos[1] / cellsize) * cellsize + cellsize / 2. - pos[1];
			pos[2] = plpos[2] + (drseq(&rs) - .5) * 2 * cellsize;
			pos[2] = floor(pos[2] / cellsize) * cellsize + cellsize / 2. - pos[2];
			red = GLfloat(rs.nextd());
			dst = pos;
			dst += localvelo;
			pos -= localvelo;
			double bright = (255 - pos.len() * 255 / (cellsize / 2.)) * basebright;
			if(1. / 255. < bright){
				glColor4f(level == 0 ? red : 1.f, level == 1 ? red : 1.f, level == 2 ? red : 1.f, GLfloat(bright / 255.));
				gldTextureBeam(avec3_000, pos, dst, cellsize / 100. / (1. + speedfactor / levelfactor));
			}
		}
		glPopAttrib();
		glPopMatrix();
	}
#endif
}




























void Star::predraw(const Viewer *){
	flags &= ~AO_DRAWAIRCORONA;
}

void Star::draw(const Viewer *vw){
	if(vw->zslice != 2)
		return;
	COLOR32 col = COLOR32RGBA(255,255,255,255);
	Vec3d gvelo = parent->tocsv(vw->velo, vw->pos, vw->cs);
	if(LIGHT_SPEED * LIGHT_SPEED < gvelo.slen()){
		extern int g_invert_hyperspace;
		double velolen = gvelo.len();
		if(g_invert_hyperspace)
			col = COLOR32SCALE(col, LIGHT_SPEED / velolen * 256) & COLOR32RGBA(255,255,255,0) | COLOR32RGBA(0,0,0,COLOR32A(col));
	}

/*	Vec3d spos = vw->cs->tocs(pos, this) - vw->pos;
	double dist = spos.len();
	double apparentSize = rad / dist / vw->fov * vw->vp.m;
	if(apparentSize < 1){
		gldSpriteGlow(spos, dist * vw->fov / vw->vp.m * 10, (spectralRGB(this->spect) * 255).cast<GLubyte>(), vw->irot);
		return;
	}*/

	static GLuint texlist = 0;
	DrawTextureSphere(this, vw, vec3_000)
		.texlist(&texlist)
		.texname("textures/Star.jpg")
		.draw();
	if(/*!((struct sun*)a)->aircorona !(flags & AO_DRAWAIRCORONA)*/1){
		Astrobj *abest = NULL;
		Vec3d epos, spos;

		/* astrobjs are sorted and the nearest solid celestial body can be
		  easily found without distance examination. */
		CoordSys *eics = findeisystem();
		if(eics){
			bool drawhere = true;
			for(AOList::iterator i = eics->aorder.begin(); i != eics->aorder.end(); i++){
				abest = (*i)->toAstrobj();
				if(abest && abest->sunAtmosphere(*vw)){
					drawhere = false;
					break;
				}
			}
			if(drawhere){
				drawsuncorona(this, vw);
			}
		}
	}
	st::draw(vw);
}

/* show flat disk that has same apparent radius as if it were a sphere. */
void drawpsphere(Astrobj *ps, const Viewer *vw, COLOR32 col){
	int i;
	Vec3d plpos, pspos, delta;
	double sdist, scale;
	plpos = vw->cs->tocs(vw->pos, vw->cs);
	pspos = vw->cs->tocs(ps->pos, ps->parent);
	delta = pspos - plpos;
#if 0
	if(glcullFrustum(&pspos/*&delta*/, ps->rad, &g_glcull))
		return;
#endif
	sdist = (pspos - plpos).slen();
	scale = ps->rad * 1/*glcullScale(pspos, &g_glcull)*/;
	if(scale * scale < .1 * .1)
		return;
	else if(scale * scale < 2. * 2.){
/*		dist = sqrt(sdist);
		glGetIntegerv(GL_VIEWPORT, vp);*/
		glPushAttrib(GL_POINT_BIT);
/*		glEnable(GL_POINT_SMOOTH);*/
		glPointSize(float(scale * 2.));
		glColor4ub(COLIST(col));
		glBegin(GL_POINTS);
/*		glVertex3d((pspos[0] - plpos[0]) / dist, (pspos[1] - plpos[1]) / dist, (pspos[2] - plpos[2]) / dist);*/
		glVertex3dv(delta);
		glEnd();
		glPopAttrib();
	}
	else if(ps->rad * ps->rad < sdist){
		int n;
		double dist, as, cas, sas;
		double (*cuts)[2];
		double x = pspos[0] - plpos[0], z = pspos[2] - plpos[2], phi, theta;
		dist = sqrt(sdist);
		as = asin(sas = ps->rad / dist);
		cas = cos(as);
		{
			double x = sas - .5;
			n = (int)(32*(-x * x / .5 / .5+ 1.)) + 8;
		}
/*		n = (int)(16*(-(1. - sas) * (1. - sas) + 1.)) + 5;*/
/*		n = (int)(16*(-(as - M_PI / 4.) * (as - M_PI / 4.) / M_PI * 4. / M_PI * 4. + 1.)) + 5;*/
		phi = atan2(x, z);
		theta = atan2((pspos[1] - plpos[1]), sqrt(x * x + z * z));
		cuts = CircleCuts(n);
		glPushMatrix();
		glRotated(phi * 360 / 2. / M_PI, 0., 1., 0.);
		glRotated(theta * 360 / 2. / M_PI, -1., 0., 0.);
		glColor4ub(COLIST(col));
		glBegin(GL_POLYGON);
		for(i = 0; i < n; i++)
			glVertex3d(cuts[i][0] * sas, cuts[i][1] * sas, cas);
		glEnd();
		glPopMatrix();
	}
}

/// \brief Draws glow effect on the celestial sphere.
///
/// The effect can flood to opposite side of view angle towards glow source, if as is greater than pi / 2.
///
/// \param q The vector towards glow source seen from the view point.
/// \param as Angle of glow bloom effect radius in celestial sphere in radians.
/// \param color The color vector for painting the glow.
/// \param numcuts The number of subdivision along phi angle. Greater value results in finer rendering with additional load.
void gldGlow(const Vec3d &dir, double as, const Vec4f &color, int numcuts = 32){
	double sas = sin(as);
	double cas = cos(as);
	double (*cuts)[2] = CircleCuts(numcuts);

	glPushMatrix();
	gldMultQuat(Quatd::direction(dir));
	glBegin(GL_TRIANGLE_FAN);
	glColor4fv(color);
	glVertex3d(0., 0., 1.);
	Vec4f edgecolor = color;
	edgecolor[3] = 0;
	glColor4fv(edgecolor);
	for(int i = 0; i <= numcuts; i++){
		int k = i % numcuts;
		glVertex3d(cuts[k][0] * sas, cuts[k][1] * sas, cas);
	}
	glEnd();
	glPopMatrix();
}


/// \brief Draws Sun corona.
///
/// \param a The Star object pointer.
/// \param vw The viewer object for drawing.
void Star::drawsuncorona(Astrobj *a, const Viewer *vw){
	double height;
	double sdist;
	double dist, as;
	Vec3d vpos, epos, spos;
	double x, z;
	Astrobj *abest = NULL;

	/* astrobjs are sorted and the nearest celestial body with atmosphere can be
	  easily found without distance examination. */
	CoordSys *eics = a->findeisystem();
	for(Astrobj::AOList::iterator i = eics->aorder.begin(); i != eics->aorder.end(); i++){
		Astrobj *a = (*i)->toAstrobj();
		if(a && a->flags & AO_ATMOSPHERE){
			abest = a;
			break;
		}
	}
	spos = a->calcPos(*vw);
	vpos = vw->pos;
	if(abest){
		epos = abest->calcPos(*vw);
		height = std::max(.001, (epos - vpos).len() - abest->rad);
	}
	else{
		epos = Vec3d(1e5,1e5,1e5);
		height = 1e10;
	}

	x = spos[0] - vpos[0], z = spos[2] - vpos[2];
	sdist = VECSDIST(spos, vpos);
	dist = sqrt(sdist);

	Vec4f col;
	{
		Vec4f white(1, 1, 1, 1);
		double f = 1./* - calcredness(vw, abest->rad, de, ds) / 256.*//*1. - h + h / MAX(1., 1.0 + sp)*/;
		white[1] = float(white[1] * 1. - (1. - f) * .5);
		white[2] = float(white[2] * f);
		if(a->classname() == Star::classRegister.id){
			col = Vec4f(Star::spectralRGB(((Star*)a)->spect));
			col[3] = 1.;
		}
		else
			col = white;
	}

	static GLuint shader = 0;
	if(g_shader_enable && !shader){
		try{
			std::vector<GLuint> shaders(2);
			int j = 0;
			if(!glsl_load_shader(shaders[j++] = glCreateShader(GL_VERTEX_SHADER), "shaders/corona.vs"))
				throw 1;
			if(!glsl_load_shader(shaders[j++] = glCreateShader(GL_FRAGMENT_SHADER), "shaders/corona.fs"))
				throw 2;
			shader = glsl_register_program(&shaders.front(), 2);
			for(unsigned i = 0; i < shaders.size(); i++)
				glDeleteShader(shaders[i]);
			if(!shader)
				throw 3;
		}
		catch(int){
			CmdPrint("drawsuncorona fail");
		}
	}

	GLattrib gla(GL_COLOR_BUFFER_BIT);
	if(g_shader_enable && shader){
		glUseProgram(shader);
		static GLint exposureLoc = glGetUniformLocation(shader, "exposure");
		static GLint tonemapLoc = glGetUniformLocation(shader, "tonemap");
		if(0 <= exposureLoc)
			glUniform1f(exposureLoc, GLfloat(r_exposure));
		if(0 <= tonemapLoc)
			glUniform1i(tonemapLoc, r_tonemap);
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	double asfactor = 1. / (1. + dist / a->rad * 1e-3);
	if(asfactor < vw->fov / vw->vp.m * 20.){
		as = M_PI * vw->fov / vw->vp.m * 2.;
		Vec3d pos = (spos - vpos) / dist;
		// Disable the shader before drawing the sprite.
		if(g_shader_enable && shader)
			glUseProgram(0);
		gldTextureGlow(pos, as, (255. * col).cast<GLubyte>(), vw->irot);
		return;
	}
	/* if neither sun nor planet with atmosphere like earth is near you, the sun's corona shrinks. */
	else{
		static const double c = .8, d = .1, e = .08;
		static int normalized = 0;
		static double normalizer;
		if(!normalized)
			normalizer = 1. / c, normalized = 1;
		Vec3d dv = vw->pos - spos;
		Vec3d spos1 = vw->rot.dvp3(dv);
		double sp = -spos1[2];
		double brightness = pow(100, -a->absmag / 5.);
		double dvslen = dv.slen();
		if(dvslen < EPSILON)
			return;
		as = M_PI * normalizer * (c / (1. + dist / a->rad / 5.) + d / (1. + height / 3.) + e * (sp * sp / dvslen)) * asfactor / (1. + 0.1 / r_exposure);

		// Retrieve eclipse check result. If we're in a penumbra, returned value is between 0 and 1.
		double val = checkEclipse(a, vw->cs, vw->pos);

		// Adjust corona glow radius and brightness by eclipse check result.
		as *= (val + 1.) / 2.;
		col[3] *= GLfloat((val + 1.) / 2.);

		Mat4d mat = mat4_u;
		spos1.normin();
		mat.vec3(2) = spos1;
		mat.vec3(1) = mat.vec3(2).vp(vec3_001);
		if(mat.vec3(1).slen() == 0.)
			mat.vec3(1) = vec3_010;
		else
			mat.vec3(1).normin();
		mat.vec3(0) = mat.vec3(1).vp(mat.vec3(2));
		sp = 100. * (1. - mat[10]) * (1. - mat[10]) - 7.;
		if(sp < 1.)
			sp = 1.;
		mat.vec3(0) *= sp;
		sp /= 100.;
		if(sp < 1.)
			sp = 1.;
		mat.vec3(1) *= sp;
		glDisable(GL_CULL_FACE);
		glPushMatrix();
		glLoadIdentity();
		spos1 *= -1;
//		gldSpriteGlow(spos1, as / M_PI, col, mat);
		glPopMatrix();
	}

	/* the sun is well too far that corona doesn't show up in a pixel. */
/*	if(as < M_PI / 300.)
		return;*/

//	col[3] = 127 + (int)(127 / (1. + height));
	{
		Vec3d pos = (spos - vpos) / dist;
		if(vw->relative){
		}
		else if(g_invert_hyperspace && LIGHT_SPEED < vw->velolen)
			col *= GLubyte(LIGHT_SPEED / vw->velolen);
		gldGlow(spos - vpos, as, col);
	}

	a->flags |= AO_DRAWAIRCORONA;

	if(g_shader_enable && shader)
		glUseProgram(0);
}


void Universe::draw(const Viewer *vw){
	if(vw->zslice == 2)
		drawstarback(vw, this, NULL, NULL);
	st::draw(vw);
}

