#define NOMINMAX
#include "RoundAstrobj.h"
#include "CoordSys-find.h"
#include "astro_star.h"
#include "glsl.h"
#include "cmd.h"
#include "StaticInitializer.h"
#include "DrawTextureSphere.h"
#include "draw/HDR.h"

extern "C"{
#include <clib/mathdef.h>
#include <clib/gl/gldraw.h>
}

#include <algorithm>

#define EPSILON 1e-7 // not sure

void drawAtmosphere(const Astrobj *a, const Viewer *vw, const Vec3d &sunpos, double thick, const GLfloat hor[4], const GLfloat dawn[4], GLfloat ret_horz[4], GLfloat ret_amb[4], int slices);


static int g_tscuts = 32;
static int g_cloud = 1;
static void g_tscuts_init(){CvarAdd("g_tscuts", &g_tscuts, cvar_int); CvarAdd("g_cloud", &g_cloud, cvar_int);}
static StaticInitializer s_tscuts(g_tscuts_init);

void RoundAstrobj::draw(const Viewer *vw){

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

	double brightness = sqrt(param.brightness);

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
				.noiseLODRange(terrainNoiseLODRange)
				.noiseLODs(terrainNoiseLODs)
				.noiseHeight(terrainNoiseHeight)
				.noisePersistence(terrainNoisePersistence)
				.noiseOctaves(terrainNoiseOctaves)
				.noiseBaseLevel(terrainNoiseBaseLevel))
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

void RoundAstrobj::drawSolid(const Viewer *vw){
	Vec3d apparentPos = tocs(vw->pos, vw->cs);

	GLint maxLights = 1;
	glGetIntegerv(GL_MAX_LIGHTS, &maxLights);
	FindBrightestAstrobj param(vw->cs, apparentPos, maxLights, this);
	param.threshold = 1e-6;
	param.eclipseThreshold = 1e-5;

	find(param);
	Astrobj *sun = param.results.size() ? const_cast<Astrobj*>(param.results[0].cs) : NULL;
	Vec3d sunpos = sun ? vw->cs->tocs(sun->pos, sun->parent) : vec3_000;
	if(oblateness != 0.){
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
	}
	else{
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
			.noiseLODRange(terrainNoiseLODRange)
			.noiseLODs(terrainNoiseLODs)
			.noiseHeight(terrainNoiseHeight)
			.noisePersistence(terrainNoisePersistence)
			.noiseOctaves(terrainNoiseOctaves)
			.noiseBaseLevel(terrainNoiseBaseLevel)
			.zbufmode(true))
			: proc(DrawTextureSphere(this, vw, sunpos));
	}
}

inline double atmo_sp2brightness(double sp);


double RoundAstrobj::getAmbientBrightness(const Viewer &vw)const{
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
