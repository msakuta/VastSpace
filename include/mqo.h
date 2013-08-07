#ifndef MQO_H
#define MQO_H
/** \file
 * \brief Definition of Metasequoia model files (.mqo) loading functions and Model class.
 */
#include "Model-forward.h"
#include "export.h"
#include "libmotion.h"

#ifdef __cplusplus
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/mat4.h>
#include <istream>
extern "C"{
#endif

#include <clib/suf/suf.h>
#include <clib/suf/sufdraw.h>


/* it's not that generic task to convert mqo to suf, so don't make
 library member. */

typedef struct ysdnm_var ysdnmv_t;

/// \brief A node of skeleton.
///
/// A Metasequoia object corresponds to a Bone.
struct Bone{
	Vec3d joint;
	int depth;
	gltestp::dstring name;
	suf_t *suf;
	suftex_t *suftex;
	struct Bone *parent;
	struct Bone *children;
	struct Bone *nextSibling;
};

/// \brief An object that can be created with a mqo file. Can contain Bones.
///
/// Metasequoia file's object hierarhcy structure is directly converted to skeleton tree.
///
/// Each object is converted to suf_t internally.
struct EXPORT Model{
	suf_t **sufs;
	suftex_t **tex;
	struct Bone **bones;
	int n;
	bool getBonePos(const char *boneName, const ysdnmv_t &var, Vec3d *pos, Quatd *rot = NULL)const;
	bool getBonePos(const char *boneName, const MotionPose &var, Vec3d *pos, Quatd *rot = NULL)const;
protected:
	bool getBonePosInt(const char *boneName, const ysdnmv_t &var, const Bone *, const Vec3d &spos, const Quatd &srot, Vec3d *pos, Quatd *rot)const;
	bool getBonePosInt(const char *boneName, const MotionPose &var, const Bone *, const Vec3d &spos, const Quatd &srot, Vec3d *pos, Quatd *rot)const;
};

suf_t *LoadMQO_SUF(const char *fname);

/// \brief A function object that is invoked once per suf.
struct EXPORT MQOTextureCallback{
	virtual void operator()(suf_t *, suftex_t **) = 0;
};

/// \brief An adapter for a plain function to MQOTextureCallback.
struct EXPORT MQOTextureFunction : public MQOTextureCallback{
	void (*func)(suf_t *, suftex_t **);
	MQOTextureFunction(void func(suf_t *, suftex_t **)) : func(func){}
	void operator()(suf_t *suf, suftex_t **suft){func(suf, suft);}
};

/// \brief Loads a MQO and returns each object in pret, with scaling factor.
EXPORT int LoadMQO_Scale(std::istream &is, suf_t ***pret, char ***pname, sufcoord scale, struct Bone ***bones, MQOTextureCallback *tex_callback = NULL, suftex_t ***callback_data = NULL);

/// \brief Loads a MQO and returns each object in ret.
EXPORT int LoadMQO(std::istream &is, suf_t ***ret, char ***pname, struct Bone ***bones);

/// \brief Loads a MQO and convert it into a Model object.
///
/// \param tex_callback Can be specified to handle occasions when a texture allocation is desired.
EXPORT struct Model *LoadMQOModelSource(std::istream &is, double scale, MQOTextureCallback *tex_callback);

/// \brief Easy version of LoadMQOModelSource, loads from a file specified by fname.
EXPORT struct Model *LoadMQOModel(const char *fname, double scale, MQOTextureCallback *tex_callback);

EXPORT void DrawMQO_V(const struct Model*, const ysdnmv_t *);
EXPORT void DrawMQOPose(const struct Model*, const MotionPose *);

#ifdef __cplusplus
}
#endif

#endif
