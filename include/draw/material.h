/** \file
 * \brief Defines material handling, OpenGL texture and lighting model.
 */
#ifndef MATERIAL_H
#define MATERIAL_H
#include "export.h"
#ifdef __cplusplus
extern "C"{
#endif
#include <clib/suf/sufdraw.h>
#ifdef __cplusplus
}
#include "dstring.h"
#endif

#ifdef __cplusplus
struct EXPORT TexParam : public suftexparam_t{
	gltestp::dstring shaderProgram[2];

	TexParam(){ flags = 0; }
	TexParam(const suftexparam_t &o);
};
#endif

EXPORT void AddMaterial(const char *name, const char *texname1, TexParam *stp1, const char *texname2, TexParam *stp2);
EXPORT void CacheSUFMaterials(const suf_t *suf);
EXPORT extern unsigned CallCacheBitmap(const char *entry, const char *fname1, suftexparam_t *pstp, const char *fname2);
EXPORT unsigned CallCacheBitmap5(const char *entry, const char *fname1, suftexparam_t *pstp1, const char *fname2, suftexparam_t *pstp2);
EXPORT suf_t *CallLoadSUF(const char *fname);

#ifdef __cplusplus

namespace gltestp{

class TextureKey : public suftexparam_t{
//	gltestp::dstring name;
public:
//	TextureKey(gltestp::dstring a) : name(a){}
	TextureKey(suftexparam_t *p) : suftexparam_t(*p){}
//	~Texture(){if(tex) glDeleteTextures(1, &tex);}
//	void bind(const char *name){
//		this->name = name;
//	}
//	gltestp::dstring getName()const{return name;}
/*	bool operator<(const TextureKey &o)const{
		if(name < o.name)
			return true;
		else if(o.name < name)
			return false;
		if(flags < o.flags)
			return true;
		return false;
	}*/
};

typedef GLuint TextureBind;

class EXPORT TexCacheBind{
	TextureBind tex[2];
	GLuint list;
	gltestp::dstring name;
	bool additive;
public:
	TexCacheBind(const gltestp::dstring &a = gltestp::dstring(""), bool additive = false);
	~TexCacheBind();
	gltestp::dstring getName()const{return name;}
	void setList(GLuint a){list = a;}
	GLuint getList()const{return list;}
	void setTex(int i, TextureBind t){tex[i] = t;}
	TextureBind getTex(int i)const{return tex[i];}
	void setAdditive(bool a){additive = a;}
	bool getAdditive()const{return additive;}
};

EXPORT const TexCacheBind *FindTexture(const gltestp::dstring &name);
EXPORT unsigned long CacheSUFMTex(const char *name, const TextureKey *tex1, const TextureKey *tex2);
EXPORT unsigned long CacheSUFTex(const char *name, const BITMAPINFO *bmi, int mipmap);
EXPORT suftex_t *AllocSUFTex(const suf_t *suf);
EXPORT suftex_t *AllocSUFTexScales(const suf_t *suf, const double *scales, int nscales, const char **texes, int ntexes);

}

//}
#endif
#endif
