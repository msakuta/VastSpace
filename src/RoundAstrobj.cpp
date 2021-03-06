/** \file
 * \brief Implementation of RoundAstrobj class.
 *
 * Actually, majority of RoundAstrobj is implemented in astrodraw.cpp.
 */
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include "RoundAstrobj.h"
#include "astrodef.h"
#include "serial_util.h"
#include "sqadapt.h"
#include "cmd.h"
#include "Game.h"
#include "CoordSys-property.h"
#include "CoordSys-sq.h"
#include "CoordSys-find.h"
#include "noises/simplexnoise1234d.h"
extern "C"{
#include <clib/mathdef.h>
}

#include <stdlib.h>

#include <algorithm>

using namespace DTS;

RoundAstrobj::RoundAstrobj(Game *game) :
	st(game),
#ifndef DEDICATED
	shader(0),
	shaderGiveup(false),
	cloudShader(0),
	cloudShaderGiveup(false),
#endif
	cloudPhase(0.),
	noisePos(0,0,0),
	tmods(terrainModMap[id])
{
#ifndef DEDICATED
	for(int i = 0; i < 6; i++)
		heightmap[i] = NULL;
#endif
}

RoundAstrobj::RoundAstrobj(const char *name, CoordSys *cs) : st(name, cs),
	oblateness(0.),
	ring(0),
	albedo(1),
#ifndef DEDICATED
	shader(0),
	shaderGiveup(false),
	cloudShader(0),
	cloudShaderGiveup(false),
	cloudHeight(0.),
#endif
	cloudPhase(0.),
	noisePos(0,0,0),
	tmods(terrainModMap[id])
{
	ringmin = ringmax = ringthick = 0;
	atmodensity = 0.;
#ifndef DEDICATED
	for(int i = 0; i < 6; i++)
		heightmap[i] = NULL;
#endif
}

const ClassRegister<RoundAstrobj> RoundAstrobj::classRegister("RoundAstrobj", sq_define);
TerrainModMap RoundAstrobj::terrainModMap;

RoundAstrobj::~RoundAstrobj(){
	// Should I delete here?
/*	if(texlist)
		glDeleteLists(texlist, 1);*/
}

template<typename T> SerializeStream &operator<<(SerializeStream &o, const std::vector<T> &v){
	o << unsigned(v.size());
	for(typename std::vector<T>::const_iterator it = v.begin(); it != v.end(); it++)
		o << *it;
	return o;
}

template<typename T> UnserializeStream &operator>>(UnserializeStream &i, std::vector<T> &v){
	int n;
	i >> n;
	v.clear();
	for(int it = 0; it < n; it++){
		T a;
		i >> a;
		v.push_back(a);
	}
	return i;
}

SerializeStream &operator<<(SerializeStream &o, const RoundAstrobj::Texture &a){
	o << a.uniformname;
	o << a.filename;
	o << a.cloudSync;
	o << a.flags;
	return o;
}

UnserializeStream &operator>>(UnserializeStream &i, RoundAstrobj::Texture &a){
	i >> a.uniformname;
	i >> a.filename;
	i >> a.cloudSync;
	i >> a.flags;
#ifndef DEDICATED
	a.list = 0;
#endif
	return i;
}

SerializeStream &operator<<(SerializeStream &o, const TerrainNoise &a){
	o << a.enable;
	o << a.height;
	o << a.mapHeight;
	o << a.persistence;
	o << a.lodRange;
	o << a.lods;
	o << a.octaves;
	o << a.baseLevel;
	o << a.zBufLODs;
	return o;
}

UnserializeStream &operator>>(UnserializeStream &i, TerrainNoise &a){
	i >> a.enable;
	i >> a.height;
	i >> a.mapHeight;
	i >> a.persistence;
	i >> a.lodRange;
	i >> a.lods;
	i >> a.octaves;
	i >> a.baseLevel;
	i >> a.zBufLODs;
	return i;
}

inline bool operator==(const RoundAstrobj::Texture &a, const RoundAstrobj::Texture &b){
	return a.uniformname == b.uniformname && b.filename == b.filename && a.cloudSync == b.cloudSync && a.flags == b.flags;
}

inline bool operator!=(const RoundAstrobj::Texture &a, const RoundAstrobj::Texture &b){
	return !operator==(a,b);
}

void RoundAstrobj::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << texture;
	sc.o << oblateness;
	sc.o << ringmin;
	sc.o << ringmax;
	sc.o << ringthick;
	sc.o << atmodensity;
	sc.o << *(Vec4<float>*)(atmohor);
	sc.o << *(Vec4<float>*)(atmodawn);
	sc.o << albedo;
	sc.o << ring;
	sc.o << textures;
	sc.o << vertexShaderName;
	sc.o << fragmentShaderName;
	sc.o << cloudTexture;
	sc.o << cloudVertexShaderName;
	sc.o << cloudFragmentShaderName;
	sc.o << cloudHeight;
	if(ring){
		sc.o << ringtexname;
		sc.o << ringbacktexname;
	}
	sc.o << terrainNoise;
}

void RoundAstrobj::unserialize(UnserializeContext &sc){
	std::vector<Texture> textures; // Temporary vector to merge to this->textures in postprocessing

	st::unserialize(sc);
	sc.i >> texture;
	sc.i >> oblateness;
	sc.i >> ringmin;
	sc.i >> ringmax;
	sc.i >> ringthick;
	sc.i >> atmodensity;
	sc.i >> *(Vec4<float>*)(atmohor);
	sc.i >> *(Vec4<float>*)(atmodawn);
	sc.i >> albedo;
	sc.i >> ring;
	sc.i >> textures;
	sc.i >> vertexShaderName;
	sc.i >> fragmentShaderName;
	sc.i >> cloudTexture;
	sc.i >> cloudVertexShaderName;
	sc.i >> cloudFragmentShaderName;
	sc.i >> cloudHeight;
	if(ring){
		sc.i >> ringtexname;
		sc.i >> ringbacktexname;
	}
	sc.i >> terrainNoise;

	// Postprocessing
	if(textures.size() != this->textures.size())
		this->textures = textures;
	else for(int i = 0; i < this->textures.size(); i++) if(textures[i] != this->textures[i])
		this->textures[i] = textures[i];
}

bool RoundAstrobj::readFile(StellarContext &sc, int argc, const char *argv[]){
	using namespace stellar_util;

	/// A local function to interpret and assign flags.  Shared by texture and extexture commands.
	auto textureFlags = [sc, argc, argv](Texture &tex, int startIndex){
#ifndef DEDICATED
		tex.list = 0;
#endif
		for(int i = startIndex; i < argc; i++){
			if(!strcmp(argv[i], "alpha"))
				tex.flags |= DTS_ALPHA;
			else if(!strcmp(argv[i], "height"))
				tex.flags |= DTS_HEIGHTMAP;
			else if(!strcmp(argv[i], "normal"))
				tex.flags |= DTS_NORMALMAP;
			else if (!strcmp(argv[i], "normalize"))
				tex.flags |= DTS_NORMALIZE;
			else if(!strcmp(argv[i], "cloudSync"))
				tex.cloudSync = true;
			else if(!strcmp(argv[i], "-cloudSync"))
				tex.cloudSync = false;
			else
				CmdPrint(gltestp::dstring() << "Warning: unknown extexture parameter ignored: " << argv[i]);
		}
	};

	const char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "oblateness")){
		if(1 < argc){
			oblateness = atof(argv[1]);
		}
		return true;
	}
	else if(!strcmp(s, "texture")){
		if(1 < argc){
			Texture &tex = texture;
			tex.uniformname = "texture";
			tex.filename = argv[1];
			textureFlags(tex, 2);
		}
		return true;
	}
	else if(!strcmp(s, "cloudtexture")){
		if(1 < argc){
			cloudTexture.uniformname = "texture";
			cloudTexture.filename = argv[1];
			cloudTexture.flags = DTS_ALPHA | DTS_NODETAIL;
		}
		return true;
	}
	else if(!strcmp(s, "extexture")){
		if(2 < argc){
			Texture tex;
			tex.uniformname = argv[1];
			tex.filename = argv[2];
			textureFlags(tex, 3);
			textures.push_back(tex);
		}
		return true;
	}
	else if(!strcmp(s, "vertexshader")){
		if(1 < argc){
			this->vertexShaderName.push_back(argv[1]);
		}
		return true;
	}
	else if(!strcmp(s, "fragmentshader")){
		if(1 < argc){
			this->fragmentShaderName.push_back(argv[1]);
		}
		return true;
	}
	else if(!strcmp(s, "cloudvertexshader")){
		if(1 < argc){
			this->cloudVertexShaderName.push_back(argv[1]);
		}
		return true;
	}
	else if(!strcmp(s, "cloudfragmentshader")){
		if(1 < argc){
			this->cloudFragmentShaderName.push_back(argv[1]);
		}
		return true;
	}
	else if(!strcmp(s, "cloudheight")){
		if(1 < argc){
			this->cloudHeight = sqcalc(sc, argv[1], s) * lengthUnit;
		}
		return true;
	}
	else if(!strcmp(s, "ringtexture")){
		if(1 < argc){
			this->ringtexname = argv[1];
		}
		return true;
	}
	else if(!strcmp(s, "ringbacktexture")){
		if(1 < argc){
			this->ringbacktexname = argv[1];
		}
		return true;
	}
	else if(!strcmp(s, "albedo")){
		if(1 < argc){
			this->albedo = float(sqcalc(sc, argv[1], "albedo"));
		}
		return true;
	}
	else if(!strcmp(s, "atmosphere_height")){
		if(1 < argc){
			atmodensity = sqcalc(sc, ps, s) * lengthUnit;
			flags |= AO_ATMOSPHERE;
		}
		return true;
	}
	else if(!strcmp(s, "atmosphere_color")){
		if(1 < argc)
			atmohor[0] = float(sqcalc(sc, argv[1], s));
		if(2 < argc)
			atmohor[1] = float(sqcalc(sc, argv[2], s));
		if(3 < argc)
			atmohor[2] = float(sqcalc(sc, argv[3], s));
		if(4 < argc)
			atmohor[3] = float(sqcalc(sc, argv[4], s));
		else
			atmohor[3] = 1.f;
		return true;
	}
	else if(!strcmp(s, "atmosphere_dawn")){
		if(1 < argc)
			atmodawn[0] = float(sqcalc(sc, argv[1], s));
		if(2 < argc)
			atmodawn[1] = float(sqcalc(sc, argv[2], s));
		if(3 < argc)
			atmodawn[2] = float(sqcalc(sc, argv[3], s));
		if(4 < argc)
			atmodawn[3] = float(sqcalc(sc, argv[4], s));
		else
			atmodawn[3] = 1.f;
		return true;
	}
	else if((!strcmp(s, "ringmin") || !strcmp(s, "ringmax") || !strcmp(s, "ringthick"))){
		RoundAstrobj *const p = this;
		p->ring = 1;
		*(!strcmp(s, "ringmin") ? &p->ringmin : !strcmp(s, "ringmax") ? &p->ringmax : &p->ringthick) = sqcalc(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoiseEnable")){
		terrainNoise.enable = sqcalcb(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoiseLODRange")){
		terrainNoise.lodRange = sqcalcd(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoiseLODs")){
		terrainNoise.lods = sqcalci(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoiseHeight")){
		terrainNoise.height = sqcalc(sc, ps, s) * lengthUnit;
		return true;
	}
	else if(!scstrcmp(s, "terrainTextureHeight")){
		terrainNoise.mapHeight = sqcalc(sc, ps, s) * lengthUnit;
		return true;
	}
	else if(!scstrcmp(s, "terrainNoisePersistence")){
		terrainNoise.persistence = sqcalc(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoiseOctaves")){
		terrainNoise.octaves = sqcalci(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoiseBaseLevel")){
		terrainNoise.baseLevel = sqcalci(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoiseZBufLODs")){
		terrainNoise.zBufLODs = sqcalci(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, _SC("terrainMod"))){
		// TerrainMod that is being added with default values
		TerrainMod tmod = {Vec3d(1,0,0), 0.001, 0.001};

		// Temporary command map that accept local commands for terrainMod
		StellarContext::CommandMap cmap = *sc.commands;
		cmap[_SC("pos")] = [&](StellarContext &sc, TokenList &argv){
			if(argv.size() < 4)
				throw StellarError("terrainMod.pos command requires 3 arguments");
			Vec3d value = Vec3d(sqcalcd(sc, argv[1], _SC("terrainMod.pos")),
				sqcalcd(sc, argv[2], _SC("terrainMod.pos")),
				sqcalcd(sc, argv[3], _SC("terrainMod.pos")));
			if(value.slen() == 0.)
				throw StellarError("terrainMod.pos cannot be zero vector");
			tmod.pos = value.norm();
		};
		cmap[_SC("radius")] = [&](StellarContext &sc, TokenList &argv){
			if(argv.size() < 2)
				throw StellarError("terrainMod.radius command requires an argument");
			tmod.radius = sqcalcd(sc, argv[1], _SC("terrainMod.radius"));
		};
		cmap[_SC("falloff")] = [&](StellarContext &sc, TokenList &argv){
			if(argv.size() < 2)
				throw StellarError("terrainMod.falloff command requires an argument");
			tmod.falloff = sqcalcd(sc, argv[1], _SC("terrainMod.falloff"));
		};

		// Parse subcommands
		sc.parseString(ps, nullptr, sc.lastLine, &cmap);

		// Add interpreted TerrainMod object to the list of TerrainMods for this RoundAstrobj.
		tmods.push_back(tmod);
		return true;
	}
	else
		return st::readFile(sc, argc, argv);
}

void RoundAstrobj::anim(double dt){
	st::anim(dt);
	updateAbsMag(dt);
}

#define SQRT2P2 (1.4142135623730950488016887242097/2.)
static const Quatd cubedirs[] = {
	Quatd(SQRT2P2,0,SQRT2P2,0), /* {0,-SQRT2P2,0,SQRT2P2} */
	Quatd(SQRT2P2,0,0,SQRT2P2),
	Quatd(0,0,1,0), /* {0,0,0,1} */
	Quatd(-SQRT2P2,0,SQRT2P2,0), /*{0,SQRT2P2,0,SQRT2P2},*/
	Quatd(-SQRT2P2,0,0,SQRT2P2), /* ??? {0,-SQRT2P2,SQRT2P2,0},*/
	Quatd(-1,0,0,0), /* {0,1,0,0}, */
};

double RoundAstrobj::getTerrainHeight(const Vec3d &basepos)const{
	double maxf = 0.;
	int direction = 0;
	for(int d = 0; d < 6; d++){
		double sp = cubedirs[d].trans(Vec3d(0, 0, 1)).sp(basepos);
		if(maxf < sp){
			direction = d;
			maxf = sp;
		}
	}

	double height = 0;
#ifndef DEDICATED
	if(heightmap[direction]){
		BITMAPINFO *bi = heightmap[direction];

		Vec3d refvec = Vec3d(0,0,1);
		Vec3d p = cubedirs[direction].itrans(basepos);
		double sc = 1. / refvec.sp(p);
		p *= sc;

		double dx = (p[0] + 1.) * bi->bmiHeader.biWidth * 0.5;
		long ix = long(dx);
		double fx = dx - ix;
		double dy = (p[1] + 1.) * bi->bmiHeader.biHeight * 0.5;
		long iy = long(dy);
		double fy = dy - iy;

		double accum = 0.;
		for(int jx = 0; jx < 2; jx++){
			long jjx = std::max(std::min(ix + jx, bi->bmiHeader.biWidth-1), 0l);
			for(int jy = 0; jy < 2; jy++){
				long jjy = std::max(std::min(bi->bmiHeader.biHeight - long(iy + jy) - 1, bi->bmiHeader.biHeight-1), 0l);
				uint8_t ui = ((RGBQUAD*)(((uint8_t*)&bi->bmiColors[bi->bmiHeader.biClrUsed]) + bi->bmiHeader.biBitCount * (jjx + jjy * bi->bmiHeader.biWidth) / 8))->rgbRed;
				accum += this->terrainNoise.mapHeight * (jx ? fx : 1. - fx) * (jy ? fy : 1. - fy) * (ui - 42) / 256.;
			}
		}
		height = accum;
	}
#endif

	return getTerrainHeightInt(basepos * (1 << terrainNoise.baseLevel), terrainNoise.octaves, terrainNoise.persistence, terrainNoise.height / rad, tmods) + height / rad;
}

/// Simplex Fractal Noise in 3D
static double sfnoise3(const Vec3d &basepos, int octaves, double persistence){
	assert(0 < octaves);
	double ret = 0.;
	double f = 1.;
	double fsum = 0.;
	for(int i = 0; i < octaves; i++){
		double s = (1 << i);
		f *= persistence;
		ret += f * snoise3d(basepos[0] * s, basepos[1] * s, basepos[2] * s);
		fsum += f;
	}
	return ret / fsum;
}

double RoundAstrobj::getTerrainHeightInt(const Vec3d &basepos, int octaves, double persistence, double aheight, const TerrainMods &tmods){
	// Modulate persistence itself with another simplex noise to obtain variation in terrain
	persistence *= (sfnoise3(basepos, octaves, persistence) + 1.) / 2.;

	// Apply TerrainMods
	for(auto it : tmods){
		double dist = (it.pos - basepos.norm()).len();
		if(it.radius < dist)
			aheight /= 1. + it.falloff / (dist - it.radius);
		else
			aheight = 0.;
	}

	return ((sfnoise3(basepos, octaves, persistence) + 0.1) * aheight + 1.);
}

bool RoundAstrobj::hasTerrainMap()const{
	for(auto it : textures){
		if(it.flags & DTS_HEIGHTMAP)
			return true;
	}
	return false;
}

void RoundAstrobj::updateAbsMag(double dt){
	FindBrightestAstrobj finder(this, vec3_000);
	finder.threshold = 1e-6;
	find(finder);
	if(0 < finder.results.size()){
		// We use the brightest light source only since reflected lights tend to be so dim.
		// Absolute magnitude induced by another light source is calculated by dividing all light
		// intensity emitted from the other light source by apparent radius of the object seen
		// from a distance of 1 parsec.

		// First, obtain apparent solid angle seen from a distance of 1 parsec.
		double appArea = rad * rad / PARSEC / PARSEC;

		// Second, calculate absolute brightness by multiplying the brightness of the other
		// light source by albedo and radius.
		double absBrightness = albedo * finder.brightness * appArea;

		// Last, convert the brightness to absolute magnitude (logarithmic scale).
		brightness = absBrightness;
	}
}

double RoundAstrobj::atmoScatter(const Viewer &vw)const{
	if(!(flags & AO_ATMOSPHERE))
		return st::atmoScatter(vw);
	double dist = const_cast<RoundAstrobj*>(this)->calcDist(vw);
	double thick = atmodensity;
	double d = exp(-(dist - rad) / thick);
	return d;
}

bool RoundAstrobj::sunAtmosphere(const Viewer &vw)const{
	return const_cast<RoundAstrobj*>(this)->calcDist(vw) - rad < atmodensity * 10.;
}

void RoundAstrobj::updateInt(double dt){
	st::updateInt(dt);
	cloudPhase += 1e-4 * dt * game->universe->astro_timescale;
}

/// StringList getter template function for propertyMap.
template<RoundAstrobj::StringList RoundAstrobj::*memb>
SQInteger RoundAstrobj::slgetter(HSQUIRRELVM v, const CoordSys *cs){
	const RoundAstrobj *a = static_cast<const RoundAstrobj*>(cs);
	sq_newarray(v, (a->*memb).size());
	for(int i = 0; i < (a->*memb).size(); i++){
		sq_pushinteger(v, i);
		sq_pushstring(v, (a->*memb)[i], -1);
		sq_set(v, -2);
	}
	return SQInteger(1);
}

/// StringList setter template function for propertyMap.
template<RoundAstrobj::StringList RoundAstrobj::*memb>
SQInteger RoundAstrobj::slsetter(HSQUIRRELVM v, CoordSys *cs){
	RoundAstrobj *a = static_cast<RoundAstrobj*>(cs);
	if(sq_gettype(v, 3) == OT_STRING){
		const SQChar *str;
		if(SQ_FAILED(sq_getstring(v, 3, &str)))
			return sq_throwerror(v, _SC("shader set fail"));
		(a->*memb).push_back(str);
	}
	else if(sq_gettype(v, 3) == OT_ARRAY){
		SQInteger size = sq_getsize(v, 3);
		for(auto i = 0; i < size; i++){
			sq_pushinteger(v, i);
			if(SQ_FAILED(sq_get(v, -2)))
				return sq_throwerror(v, _SC("shader set fail"));
			const SQChar *str;
			if(SQ_FAILED(sq_getstring(v, 3, &str)))
				return sq_throwerror(v, _SC("shader set fail"));
			(a->*memb).push_back(str);
		}
	}
	return SQInteger(0);
}


const CoordSys::PropertyMap &RoundAstrobj::propertyMap()const{
	static PropertyMap pmap = st::propertyMap();
	static bool init = false;
	if(!init){
		init = true;
		pmap["oblateness"] = PropertyEntry(
			[](HSQUIRRELVM v, const CoordSys *cs){
				const RoundAstrobj *a = static_cast<const RoundAstrobj*>(cs);
				sq_pushfloat(v, SQFloat(a->oblateness));
				return SQInteger(1);
			},
			[](HSQUIRRELVM v, CoordSys *cs){
				RoundAstrobj *a = static_cast<RoundAstrobj*>(cs);
				SQFloat f;
				if(SQ_FAILED(sq_getfloat(v, 3, &f)))
					return sq_throwerror(v, _SC("RoundAstrobj.oblateness could not convert to float"));
				a->oblateness = f;
				return SQInteger(0);
			}
		);
		pmap["texture"] = PropertyEntry(
			[](HSQUIRRELVM v, const CoordSys *cs){
				const RoundAstrobj *a = static_cast<const RoundAstrobj*>(cs);
				sq_pushstring(v, a->texture.filename, -1);
				return SQInteger(1);
			},
			[](HSQUIRRELVM v, CoordSys *cs){
				RoundAstrobj *a = static_cast<RoundAstrobj*>(cs);
				const SQChar *str;
				if(SQ_FAILED(sq_getstring(v, 3, &str)))
					return sq_throwerror(v, _SC("RoundAstrobj.texture could not convert to string"));
				a->texture.filename = str;
				return SQInteger(0);
			}
		);
		pmap["vertexshader"] = PropertyEntry(
			slgetter<&RoundAstrobj::vertexShaderName>, slsetter<&RoundAstrobj::vertexShaderName>);
		pmap["fragmentshader"] = PropertyEntry(
			slgetter<&RoundAstrobj::fragmentShaderName>, slsetter<&RoundAstrobj::fragmentShaderName>);
		pmap["cloudvertexshader"] = PropertyEntry(
			slgetter<&RoundAstrobj::cloudVertexShaderName>, slsetter<&RoundAstrobj::cloudVertexShaderName>);
		pmap["cloudfragmentshader"] = PropertyEntry(
			slgetter<&RoundAstrobj::cloudFragmentShaderName>, slsetter<&RoundAstrobj::cloudFragmentShaderName>);
	}
	return pmap;
}

bool RoundAstrobj::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	sq_setclassudsize(v, -1, sq_udsize); // classudsize is not inherited from CoordSys
	register_closure(v, _SC("constructor"), sq_CoordSysConstruct<RoundAstrobj>);
	register_closure(v, _SC("getTerrainHeight"), [](HSQUIRRELVM v){
		RoundAstrobj *a = static_cast<RoundAstrobj*>(sq_refobj(v));
		SQVec3d qv;
		qv.getValue(v, 2);
		// RoundAstrobj::getTerrainHeight requires the argument vector to be normalized
		// and usually the caller should normalize before calling, but we will do this
		// for scripts in case the scripter has forgotten.  After all, Squirrel VM
		// overhead would be far more costly than a vector's normalization.
		sq_pushfloat(v, a->getTerrainHeight(qv.value.norm()));
		return SQInteger(1);
	});
	sq_createslot(v, -3);
	return true;
}

#ifdef DEDICATED
void RoundAstrobj::draw(const Viewer *){}
double RoundAstrobj::getAmbientBrightness(const Viewer &)const{return 0.;}
#endif



const ClassRegister<Satellite> Satellite::classRegister("Satellite");
