#ifndef CLIB_GL_CULL_H
#define CLIB_GL_CULL_H

struct glcull{
	double fov; /* field of view */
	double znear; /* near clipping plane is sometimes necesarry */
	double zfar; /* the distance of z far clipping plane */
	double viewpoint[3], viewdir[3];
	double invrot[16]; /* inverse of rotation of modelview transformation */
	double trans[16]; /* projection * modelview transformation; used for culling */
	int width, height, res; /* viewport characteristics */
	int ortho; /* orthogonal flag */
};

/* do not change projection or modelview matrix after glcullInit and before glcullFrustum */
extern void glcullInit(struct glcull *, double fov, const double (*viewpoint)[3], const double (*invrot)[16], double zfar);
extern int glcullNear(const double (*pos)[3], double rad, const struct glcull *gc);
extern int glcullFar(const double (*pos)[3], double rad, const struct glcull *gc);
extern int glcullFrustum(const double (*pos)[3], double radius, const struct glcull *);
extern double glcullScale(const double (*pos)[3], const struct glcull *);

#endif
