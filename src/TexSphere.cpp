/** \file
 * \brief Implementation of TexSphere class.
 *
 * Actually, majority of TexSphere is implemented in astrodraw.cpp.
 */
#define _CRT_SECURE_NO_WARNINGS
#include "TexSphere.h"
#include "serial_util.h"
#include "sqadapt.h"
#include "cmd.h"
#include "Game.h"
#include "CoordSys-property.h"
#include "CoordSys-sq.h"
#include "CoordSys-find.h"

#include <stdlib.h>

TexSphere::TexSphere(Game *game) :
	st(game),
#ifndef DEDICATED
	texlist(0),
	cloudtexlist(0),
	shader(0),
	shaderGiveup(false),
	cloudShader(0),
	cloudShaderGiveup(false),
#endif
	cloudPhase(0.),
	noisePos(0,0,0)
{
}

TexSphere::TexSphere(const char *name, CoordSys *cs) : st(name, cs),
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
	terrainNoiseHeight(1.),
	terrainNoisePersistence(0.65),
	terrainNoiseOctaves(7),
	terrainNoiseEnable(false)
{
	texlist = cloudtexlist = 0;
	ringmin = ringmax = ringthick = 0;
	atmodensity = 0.;
}

const ClassRegister<TexSphere> TexSphere::classRegister("TextureSphere", sq_define);

TexSphere::~TexSphere(){
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

SerializeStream &operator<<(SerializeStream &o, const TexSphere::Texture &a){
	o << a.uniformname;
	o << a.filename;
	o << a.cloudSync;
	o << a.flags;
	return o;
}

UnserializeStream &operator>>(UnserializeStream &i, TexSphere::Texture &a){
	i >> a.uniformname;
	i >> a.filename;
	i >> a.cloudSync;
	i >> a.flags;
#ifndef DEDICATED
	a.list = 0;
#endif
	return i;
}

inline bool operator==(const TexSphere::Texture &a, const TexSphere::Texture &b){
	return a.uniformname == b.uniformname && b.filename == b.filename && a.cloudSync == b.cloudSync && a.flags == b.flags;
}

inline bool operator!=(const TexSphere::Texture &a, const TexSphere::Texture &b){
	return !operator==(a,b);
}

void TexSphere::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << texname;
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
	sc.o << cloudtexname;
	sc.o << cloudVertexShaderName;
	sc.o << cloudFragmentShaderName;
	sc.o << cloudHeight;
	if(ring){
		sc.o << ringtexname;
		sc.o << ringbacktexname;
	}
	sc.o << terrainNoiseEnable;
	sc.o << terrainNoiseHeight;
	sc.o << terrainNoiseOctaves;
	sc.o << terrainNoisePersistence;
}

void TexSphere::unserialize(UnserializeContext &sc){
	std::vector<Texture> textures; // Temporary vector to merge to this->textures in postprocessing

	st::unserialize(sc);
	sc.i >> texname;
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
	sc.i >> cloudtexname;
	sc.i >> cloudVertexShaderName;
	sc.i >> cloudFragmentShaderName;
	sc.i >> cloudHeight;
	if(ring){
		sc.i >> ringtexname;
		sc.i >> ringbacktexname;
	}
	sc.i >> terrainNoiseEnable;
	sc.i >> terrainNoiseHeight;
	sc.i >> terrainNoiseOctaves;
	sc.i >> terrainNoisePersistence;

	// Postprocessing
	if(textures.size() != this->textures.size())
		this->textures = textures;
	else for(int i = 0; i < this->textures.size(); i++) if(textures[i] != this->textures[i])
		this->textures[i] = textures[i];
}

bool TexSphere::readFile(StellarContext &sc, int argc, const char *argv[]){
	using namespace stellar_util;
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
			this->texname = argv[1];
		}
		return true;
	}
	else if(!strcmp(s, "cloudtexture")){
		if(1 < argc){
			this->cloudtexname = argv[1];
		}
		return true;
	}
	else if(!strcmp(s, "extexture")){
		if(2 < argc){
			Texture tex;
			tex.uniformname = argv[1];
			tex.filename = argv[2];
#ifndef DEDICATED
			tex.list = 0;
#endif
			tex.cloudSync = 3 < argc && 0. != sqcalc(sc, argv[3], s);
			for(int i = 4; i < argc; i++){
				if(!strcmp(argv[i], "alpha"))
					tex.flags |= DTS_ALPHA;
				else if(!strcmp(argv[i], "normal"))
					tex.flags |= DTS_NORMALMAP;
				else
					CmdPrint(gltestp::dstring() << "Warning: unknown extexture parameter ignored: " << argv[i]);
			}
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
			this->cloudHeight = sqcalc(sc, argv[1], s);
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
			atmodensity = sqcalc(sc, ps, s);
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
		TexSphere *const p = this;
		p->ring = 1;
		*(!strcmp(s, "ringmin") ? &p->ringmin : !strcmp(s, "ringmax") ? &p->ringmax : &p->ringthick) = sqcalc(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoiseEnable")){
		terrainNoiseEnable = sqcalcb(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoiseHeight")){
		terrainNoiseHeight = sqcalc(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoisePersistence")){
		terrainNoisePersistence = sqcalc(sc, ps, s);
		return true;
	}
	else if(!scstrcmp(s, "terrainNoiseOctaves")){
		terrainNoiseOctaves = sqcalci(sc, ps, s);
		return true;
	}
	else
		return st::readFile(sc, argc, argv);
}

void TexSphere::anim(double dt){
	st::anim(dt);
	updateAbsMag(dt);
}

void TexSphere::updateAbsMag(double dt){
	FindBrightestAstrobj finder(this, vec3_000);
	finder.threshold = 1e-6;
	find(finder);
	if(0 < finder.results.size()){
		// We use the brightest light source only since reflected lights tend to be so dim.
		// Absolute magnitude induced by another light source is calculated by dividing all light
		// intensity emitted from the other light source by apparent radius of the object seen
		// from a distance of 1 parsec.

		// First, obtain apparent solid angle seen from a distance of 1 parsec.
		double appArea = rad * rad / 3.e14 / 3.e14;

		// Second, calculate absolute brightness by multiplying the brightness of the other
		// light source by albedo and radius.
		double absBrightness = albedo * finder.brightness * appArea;

		// Last, convert the brightness to absolute magnitude (logarithmic scale).
		absmag = float(-log(absBrightness) / log(2.512) - 26.7);
	}
}

double TexSphere::atmoScatter(const Viewer &vw)const{
	if(!(flags & AO_ATMOSPHERE))
		return st::atmoScatter(vw);
	double dist = const_cast<TexSphere*>(this)->calcDist(vw);
	double thick = atmodensity;
	double d = exp(-(dist - rad) / thick);
	return d;
}

bool TexSphere::sunAtmosphere(const Viewer &vw)const{
	return const_cast<TexSphere*>(this)->calcDist(vw) - rad < atmodensity * 10.;
}

void TexSphere::updateInt(double dt){
	st::updateInt(dt);
	cloudPhase += 1e-4 * dt * game->universe->astro_timescale;
}

/// StringList getter template function for propertyMap.
template<TexSphere::StringList TexSphere::*memb>
SQInteger TexSphere::slgetter(HSQUIRRELVM v, const CoordSys *cs){
	const TexSphere *a = static_cast<const TexSphere*>(cs);
	sq_newarray(v, (a->*memb).size());
	for(int i = 0; i < (a->*memb).size(); i++){
		sq_pushinteger(v, i);
		sq_pushstring(v, (a->*memb)[i], -1);
		sq_set(v, -2);
	}
	return SQInteger(1);
}

/// StringList setter template function for propertyMap.
template<TexSphere::StringList TexSphere::*memb>
SQInteger TexSphere::slsetter(HSQUIRRELVM v, CoordSys *cs){
	TexSphere *a = static_cast<TexSphere*>(cs);
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


const CoordSys::PropertyMap &TexSphere::propertyMap()const{
	static PropertyMap pmap = st::propertyMap();
	static bool init = false;
	if(!init){
		init = true;
		pmap["oblateness"] = PropertyEntry(
			[](HSQUIRRELVM v, const CoordSys *cs){
				const TexSphere *a = static_cast<const TexSphere*>(cs);
				sq_pushfloat(v, SQFloat(a->oblateness));
				return SQInteger(1);
			},
			[](HSQUIRRELVM v, CoordSys *cs){
				TexSphere *a = static_cast<TexSphere*>(cs);
				SQFloat f;
				if(SQ_FAILED(sq_getfloat(v, 3, &f)))
					return sq_throwerror(v, _SC("TexSphere.oblateness could not convert to float"));
				a->oblateness = f;
				return SQInteger(0);
			}
		);
		pmap["texture"] = PropertyEntry(
			[](HSQUIRRELVM v, const CoordSys *cs){
				const TexSphere *a = static_cast<const TexSphere*>(cs);
				sq_pushstring(v, a->texname, -1);
				return SQInteger(1);
			},
			[](HSQUIRRELVM v, CoordSys *cs){
				TexSphere *a = static_cast<TexSphere*>(cs);
				const SQChar *str;
				if(SQ_FAILED(sq_getstring(v, 3, &str)))
					return sq_throwerror(v, _SC("TexSphere.texture could not convert to string"));
				a->texname = str;
				return SQInteger(0);
			}
		);
		pmap["vertexshader"] = PropertyEntry(
			slgetter<&TexSphere::vertexShaderName>, slsetter<&TexSphere::vertexShaderName>);
		pmap["fragmentshader"] = PropertyEntry(
			slgetter<&TexSphere::fragmentShaderName>, slsetter<&TexSphere::fragmentShaderName>);
		pmap["cloudvertexshader"] = PropertyEntry(
			slgetter<&TexSphere::cloudVertexShaderName>, slsetter<&TexSphere::cloudVertexShaderName>);
		pmap["cloudfragmentshader"] = PropertyEntry(
			slgetter<&TexSphere::cloudFragmentShaderName>, slsetter<&TexSphere::cloudFragmentShaderName>);
	}
	return pmap;
}

bool TexSphere::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	sq_setclassudsize(v, -1, sq_udsize); // classudsize is not inherited from CoordSys
	register_closure(v, _SC("constructor"), sq_CoordSysConstruct<TexSphere>);
	sq_createslot(v, -3);
	return true;
}

#ifdef DEDICATED
void TexSphere::draw(const Viewer *){}
double TexSphere::getAmbientBrightness(const Viewer &)const{return 0.;}
#endif



const ClassRegister<Satellite> Satellite::classRegister("Satellite");
