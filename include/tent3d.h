#ifndef TENT3D_H
#define TENT3D_H

#ifdef __cplusplus
extern "C"{
#endif
#include <clib/colseq/color.h>
#include <clib/colseq/cs.h>
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <cpplib/vec3.h>
#include <cpplib/mat4.h>
#include <cpplib/quat.h>
#include <cpplib/gl/cullplus.h>
#endif
#ifndef NPROFILE
#include <stddef.h>
#endif
#include "export.h"

typedef unsigned long tent3d_flags_t;

/* teline_flags_t */
#define TEL_NOREMOVE    (1<<0) /* if set, won't be deleted when it gets out of border. */
#define TEL3_HEADFORWARD (1<<1) /* the line will head towards its velocity. omg now means angle offset. */
#define TEL_VELOLEN     (1<<2) /* set length by its velocity. len now means factor. */
#define TEL_RETURN      (1<<3) /* lines gone beyond border will appear at opposide edge */
#define TEL_SHRINK      (1<<4) /* shrink length as its life decreases */
#define TEL3_REFLECT    (1<<5) /* whether reflect on border, ignored when RETURN is set */
#define TEL3_INVROTATE  (1<<6) /* perform inverse rotation by a given matrix for billboard effect */
#define TEL3_THICK      (1<<7) /* line draws thick */
#define TEL3_NOLINE		(1<<8) /* disable draw of lines */
#define TEL_TRAIL		(1<<9) /* append trailing tracer */
#define TEL_FOLLOW      (1<<10) /* follow some other tent, assume vx as the pointer to target */
#define TEL_CIRCLE		(1<<11) /* line form; uses 4 bits up to 15th bit */
#define TEL_FILLCIRCLE	(2<<11)
#define TEL_POINT		(3<<11)
#define TEL_RECTANGLE	(4<<11)
#define TEL3_CYLINDER   (5<<11) /* kind of shockwave */
#define TEL3_PSEUDOSPHERE	(6<<11)
#define TEL3_EXPANDISK	(7<<11) /* expanding disk */
#define TEL3_STATICDISK (8<<11)
#define TEL3_CALLBACK   (9<<11) /* call a given function pointer to draw; be careful on using this */
#define TEL3_GLOW       (10<<11) /* glowing effect sprite */
#define TEL3_EXPANDGLOW (11<<11) /* growing glow */
#define TEL3_SPRITE     (12<<11) /* glowing effect sprite */
#define TEL3_EXPANDTORUS (13<<11) /* glowing effect sprite */
#define TEL3_GRADCIRCLE  (14<<11) /* gradiated circle with transparency, assume col as pointer to CS */
#define TEL3_GRADCIRCLE2 (15<<11) /* time-dependent version, assume col as pointer to CS2 */
#define TEL3_RANDOMCOLOR (1<<16)
#define TEL3_NEAR       (1<<17) /* draw the line a little nearer than it actually is, to prevent race condition of decals. */
#define TEL3_FADEEND    (1<<18) /* fade the tail to make it look smoother. */
#define TEL3_HALF       (1<<19) /* center point means one of the edges of the line */
#define TEL3_QUAT       (1<<20) /* use quaternion to express rotation */
#define TEL3_HITFUNC    (1<<21) /* use w->vft->pointhit to determine intersection into ground */
#define TEL3_ACCELFUNC  (1<<22) /* use w->vft->accel to determine gravity */



#ifdef __cplusplus
struct tent3d_line_list;
struct tent3d_line_callback{
	Vec3d pos; /* center point, double because the velo may be so slow */
	Vec3d velo; /* velocity. */
	double len; /* length of the beam */
	Quatd rot; /* rotation in quaternion form */
	Vec3d omg; /* angle velocity */
	double life; /* remaining rendering time */
};
struct tent3d_line_drawdata{
	Vec3d viewpoint, viewdir;
	Quatd rot;
	Mat4d invrot;
	double fov; /* field of view */

	/* following are trivial members; caller need not to set them */
	GLcull *pgc;
};
struct tent3d_fpol_list;
struct glcull;

EXPORT struct tent3d_line_list *NewTeline3D(unsigned maxt, unsigned init, unsigned unit); /* constructor */
EXPORT struct tent3d_line_list *NewTeline3DFunc(unsigned maxt, unsigned init, unsigned unit, struct war_field*); /* constructor */
EXPORT void DelTeline3D(struct tent3d_line_list *); /* destructor */
EXPORT void AddTeline3D(struct tent3d_line_list *tell, const Vec3d &pos, const Vec3d &velo, double len, const Quatd &rot, const Vec3d &omg, const Vec3d &grv, COLOR32 col, tent3d_flags_t flags, float life);
EXPORT void AddTelineCallback3D(struct tent3d_line_list *tell, const Vec3d &pos, const Vec3d &velo, double len, const Quatd &rot, const Vec3d &omg, const Vec3d &grv, void (*draw_proc)(const struct tent3d_line_callback*, const struct tent3d_line_drawdata*, void *private_data), void *private_data, tent3d_flags_t flags, float life);
EXPORT void AnimTeline3D(struct tent3d_line_list *tell, double dt);
EXPORT void DrawTeline3D(struct tent3d_line_list *tell, struct tent3d_line_drawdata *);
#endif


#ifndef NPROFILE
struct tent3d_line_debug{
	double drawteline;
	size_t so_teline3_t;
	unsigned teline_c;
	unsigned teline_m;
	unsigned teline_s;
};
const struct tent3d_line_debug *Teline3DDebug(const struct tent3d_line_list *);
#endif


#endif
