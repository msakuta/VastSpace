/** \file
 * \brief Implementation of TexSphere class.
 *
 * Actually, majority of TexSphere is implemented in astrodraw.cpp.
 */
#define _CRT_SECURE_NO_WARNINGS
#include "TexSphere.h"
#include "serial_util.h"
#include "sqadapt.h"
extern "C"{
#include "calc/calc.h"
}

TexSphere::TexSphere() :
	texlist(0),
	cloudtexlist(0),
	shader(0),
	shaderGiveup(false),
	cloudShader(0),
	cloudShaderGiveup(false),
	cloudPhase(0.)
{
}

TexSphere::TexSphere(const char *name, CoordSys *cs) : st(name, cs),
	oblateness(0.),
	ring(0),
	shader(0),
	shaderGiveup(false),
	cloudShader(0),
	cloudShaderGiveup(false),
	cloudHeight(0.),
	cloudPhase(0.)
{
	texlist = cloudtexlist = 0;
	ringmin = ringmax = 0;
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
	for(std::vector<T>::const_iterator it = v.begin(); it != v.end(); it++)
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
	o << a.normalmap;
	return o;
}

UnserializeStream &operator>>(UnserializeStream &i, TexSphere::Texture &a){
	i >> a.uniformname;
	i >> a.filename;
	i >> a.cloudSync;
	i >> a.normalmap;
	a.list = 0;
	return i;
}

inline bool operator==(const TexSphere::Texture &a, const TexSphere::Texture &b){
	return a.uniformname == b.uniformname && b.filename == b.filename && a.cloudSync == b.cloudSync && a.normalmap == b.normalmap;
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
	sc.o << ring;
	sc.o << textures;
	sc.o << vertexShaderName;
	sc.o << fragmentShaderName;
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
	sc.i >> ring;
	sc.i >> textures;
	sc.i >> vertexShaderName;
	sc.i >> fragmentShaderName;

	// Postprocessing
	if(textures.size() != this->textures.size())
		this->textures = textures;
	else for(int i = 0; i < this->textures.size(); i++) if(textures[i] != this->textures[i])
		this->textures[i] = textures[i];
}

bool TexSphere::readFile(StellarContext &sc, int argc, const char *argv[]){
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
			tex.list = 0;
			tex.cloudSync = 3 < argc && !!calc3(&argv[3], sc.vl, NULL);
			tex.normalmap = 4 < argc && !strcmp(argv[4], "normal");
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
			this->cloudHeight = calc3(&argv[1], sc.vl, NULL);
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
	else if(!strcmp(s, "atmosphere_height")){
		if(1 < argc){
			atmodensity = calc3(&ps, sc.vl, NULL);
			flags |= AO_ATMOSPHERE;
		}
		return true;
	}
	else if(!strcmp(s, "atmosphere_color")){
		if(1 < argc)
			atmohor[0] = float(calc3(&argv[1], sc.vl, NULL));
		if(2 < argc)
			atmohor[1] = float(calc3(&argv[2], sc.vl, NULL));
		if(3 < argc)
			atmohor[2] = float(calc3(&argv[3], sc.vl, NULL));
		if(4 < argc)
			atmohor[3] = float(calc3(&argv[4], sc.vl, NULL));
		else
			atmohor[3] = 1.f;
		return true;
	}
	else if(!strcmp(s, "atmosphere_dawn")){
		if(1 < argc)
			atmodawn[0] = float(calc3(&argv[1], sc.vl, NULL));
		if(2 < argc)
			atmodawn[1] = float(calc3(&argv[2], sc.vl, NULL));
		if(3 < argc)
			atmodawn[2] = float(calc3(&argv[3], sc.vl, NULL));
		if(4 < argc)
			atmodawn[3] = float(calc3(&argv[4], sc.vl, NULL));
		else
			atmodawn[3] = 1.f;
		return true;
	}
	else if((!strcmp(s, "ringmin") || !strcmp(s, "ringmax") || !strcmp(s, "ringthick"))){
		TexSphere *const p = this;
		p->ring = 1;
		*(!strcmp(s, "ringmin") ? &p->ringmin : !strcmp(s, "ringmax") ? &p->ringmax : &p->ringthick) = calc3(&ps, sc.vl, NULL);
		return true;
	}
	else
		return st::readFile(sc, argc, argv);
}

double TexSphere::atmoScatter(const Viewer &vw)const{
	if(!(flags & AO_ATMOSPHERE))
		return st::atmoScatter(vw);
	double dist = const_cast<TexSphere*>(this)->calcDist(vw);
	double thick = atmodensity;
	double d = (dist - rad) < thick * 128. ? 1. : thick * 128. / (dist - rad);
	return d;
}

bool TexSphere::sunAtmosphere(const Viewer &vw)const{
	return const_cast<TexSphere*>(this)->calcDist(vw) - rad < atmodensity * 10.;
}

void TexSphere::anim(double dt){
	cloudPhase += 1e-4 * dt * astro_timescale;
	st::anim(dt);
}


SQInteger TexSphere::sqf_get(HSQUIRRELVM v){
	TexSphere *p;
	const SQChar *wcs;
	sq_getstring(v, -1, &wcs);
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
	if(!strcmp(wcs, _SC("oblateness"))){
		sq_pushfloat(v, SQFloat(p->oblateness));
		return 1;
	}
	else
		return st::sqf_get(v);
}

SQInteger TexSphere::sqf_set(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return SQ_ERROR;
	TexSphere *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
	if(!strcmp(wcs, _SC("oblateness"))){
		SQFloat f;
		sq_getfloat(v, 3, &f);
		p->oblateness = double(f);
		return 0;
	}
	else
		return sqa::sqf_get<Astrobj>(v);
}

bool TexSphere::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	register_closure(v, _SC("_get"), sqf_get);
	register_closure(v, _SC("_set"), sqf_set);
	sq_createslot(v, -3);
	return true;
}



const ClassRegister<Satellite> Satellite::classRegister("Satellite");