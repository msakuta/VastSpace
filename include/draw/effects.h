/** \file
 * \brief Definition of various graphical effects.
 */

#ifndef DRAW_EFFECTS_H
#define DRAW_EFFECTS_H
#include "export.h"
#include "tent3d.h"

// Damn silly Windows needs windows.h to be included before gl/gl.h.
#ifdef _WIN32
typedef unsigned int GLuint;
#else
#include <GL/gl.h>
#endif

EXPORT void sparkdraw(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data);
EXPORT void sparkspritedraw(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data);

EXPORT void smokedraw(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data);

struct smokedraw_swirl_data{
	COLOR32 col;
	bool expand;
};

EXPORT void smokedraw_swirl(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data);

EXPORT void firesmokedraw(const Teline3CallbackData *p, const Teline3DrawData *dd, void *private_data);

EXPORT void debrigib(const Teline3CallbackData *pl, const Teline3DrawData *dd, void *pv);

EXPORT void debrigib_multi(const Teline3CallbackData *pl, const Teline3DrawData *dd, void *pv);

EXPORT GLuint muzzle_texture();

EXPORT void gldScrollTextureBeam(const Vec3d &view, const Vec3d &start, const Vec3d &end, double width, double offset);

EXPORT void drawmuzzleflash4(const Vec3d &pos, const Mat4d &rot, double rad, const Mat4d &irot, struct random_sequence *prs, const Vec3d &viewer);

EXPORT double perlin_noise_pixel(int x, int y, int bit);

EXPORT void drawmuzzleflasha(const Vec3d &pos, const Vec3d &org, double rad, const Mat4d &irot);


#endif
