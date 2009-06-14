#include "clib/gl/cull.h"
#include "clib/avec3.h"
#include "clib/amat4.h"
#if _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <stdlib.h>

#define SQRT2 1.4142135623730950488016887242097

void glcullInit(struct glcull *gc, double fov, const double (*viewpoint)[3], const double (*invrot)[16], double znear, double zfar){
	gc->fov = fov;
	VECCPY(gc->viewpoint, *viewpoint);
	memcpy(gc->invrot, *invrot, sizeof gc->invrot);
	gc->znear = znear;
	gc->zfar = zfar;

	/* initiate the matrix for culling */
	{
		amat4_t modelmat, persmat;
		glGetDoublev(GL_MODELVIEW_MATRIX, modelmat);
		VECNULL(&modelmat[12]);
		MAT4TRANSLATE(modelmat, -(*viewpoint)[0], -(*viewpoint)[1], -(*viewpoint)[2]);
		glGetDoublev(GL_PROJECTION_MATRIX, persmat);

		/* TODO: aspect ratio other than 1 causes troubles determining intersection against
		  sphere geometries unless we assign fake ratio here. */
		persmat[0] = 1. / fov;
		persmat[5] = 1. / fov;
		persmat[14] = 0.; /* assign 0 to this element to make negative coords to be culled */
		mat4mp(gc->trans, persmat, modelmat);
/*		MAT4CPY(gc->trans, modelmat);*/
	}
	{
		avec3_t nh0 = {0., 0., -1.};
/*		amat4_t rot;
		MAT4TRANSPOSE(rot, *invrot);*/
		MAT4VP3(gc->viewdir, *invrot, nh0);
	}
	{
		int viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		gc->width = viewport[2];
		gc->height = viewport[3];
		gc->res = gc->width < gc->height ? gc->height : gc->width;
	}
	gc->ortho = 0;
}

int glcullNear(const double (*pos)[3], double rad, const struct glcull *gc){
	avec3_t vec, delta;
	VECSUB(delta, *pos, gc->viewpoint);
	if(VECSP(gc->viewdir, delta) < gc->znear - rad)
		return 1;
	else
		return 0;
}

int glcullFar(const double (*pos)[3], double rad, const struct glcull *gc){
	avec3_t vec, delta;
	VECSUB(delta, *pos, gc->viewpoint);
	if(gc->zfar + rad < VECSP(gc->viewdir, delta))
		return 1;
	else
		return 0;
}

int glcullFrustum(const double (*pos)[3], double rad, const struct glcull *gc){
	avec3_t dst, vec, delta;
	double sp;
	if(gc->ortho)
		return 0;
	VECSUB(delta, *pos, gc->viewpoint);
	sp = VECSP(gc->viewdir, delta);
	if(gc->zfar + rad < sp)
		return 1;
	if(sp < gc->znear - rad)
		return 1;
	VECCPY(dst, *pos);
	VECSADD(dst, gc->viewdir, SQRT2 * rad / gc->fov);
	MAT4VP3(vec, gc->trans, dst);
	if(vec[2] < 0. || vec[0] < -vec[2] || vec[2] < vec[0] || vec[1] < -vec[2] || vec[2] < vec[1])
		return 1;
	return 0;
}

/* returned value can be negative! */
double glcullScale(const double (*pos)[3], const struct glcull *gc){
	avec3_t delta;
	if(gc->ortho)
		return gc->res / gc->zfar;
	VECSUB(delta, *pos, gc->viewpoint);
	return 1. / VECSP(delta, gc->viewdir) / gc->fov * gc->res;
}



