#ifndef MQO_H
#define MQO_H
#include "export.h"

#ifdef __cplusplus
extern "C"{
#endif

#include <clib/suf/suf.h>
#include <clib/suf/sufdraw.h>

/* it's not that generic task to convert mqo to suf, so don't make
 library member. */

struct Bone{
	double joint[3];
	int depth;
	char *name;
	suf_t *suf;
	suftex_t *suftex;
	struct Bone *parent;
	struct Bone *children;
	struct Bone *nextSibling;
};

struct Model{
	suf_t **sufs;
	suftex_t **tex;
	struct Bone **bones;
	int n;
};

suf_t *LoadMQO_SUF(const char *fname);

EXPORT int LoadMQO_Scale(const char *fname, suf_t ***pret, char ***pname, sufcoord scale, struct Bone ***bones, suftex_t ***texes = NULL);
EXPORT int LoadMQO(const char *fname, suf_t ***ret, char ***pname, struct Bone ***bones);
EXPORT struct Model *LoadMQOModel(const char *fname, double scale);

typedef struct ysdnm_var ysdnmv_t;

EXPORT void DrawMQO_V(const struct Model*, const ysdnmv_t *);

#ifdef __cplusplus
}
#endif

#endif
