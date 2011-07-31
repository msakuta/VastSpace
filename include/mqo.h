#ifndef MQO_H
#define MQO_H
#include "export.h"
#include "libmotion.h"

#ifdef __cplusplus
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/mat4.h>
extern "C"{
#endif

#include <clib/suf/suf.h>
#include <clib/suf/sufdraw.h>

/* it's not that generic task to convert mqo to suf, so don't make
 library member. */

typedef struct ysdnm_var ysdnmv_t;


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

EXPORT int LoadMQO_Scale(const char *fname, suf_t ***pret, char ***pname, sufcoord scale, struct Bone ***bones, void tex_callback(suf_t *, suftex_t **) = NULL, suftex_t ***callback_data = NULL);
EXPORT int LoadMQO(const char *fname, suf_t ***ret, char ***pname, struct Bone ***bones);
EXPORT struct Model *LoadMQOModel(const char *fname, double scale, void tex_callback(suf_t *, suftex_t **));

EXPORT void DrawMQO_V(const struct Model*, const ysdnmv_t *);
EXPORT void DrawMQOPose(const struct Model*, const MotionPose *);

#ifdef __cplusplus
}
#endif

#endif
