#ifndef TENT3D_H
#define TENT3D_H

#include <clib/colseq/color.h>
#include <clib/colseq/cs.h>
#ifndef NPROFILE
#include <stddef.h>
#endif

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
#define TEL_CIRCLE		(1<<11) /* line form; uses 3 bits up to 12th bit */
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
#define TEL_GRADCIRCLE  (12<<11) /* gradiated circle with transparency, assume col as pointer to CS */
#define TEL_GRADCIRCLE2 (13<<11) /* time-dependent version, assume col as pointer to CS2 */
#define TEL3_RANDOMCOLOR (1<<16)
#define TEL3_NEAR       (1<<17) /* draw the line a little nearer than it actually is, to prevent race condition of decals. */
#define TEL3_FADEEND    (1<<18) /* fade the tail to make it look smoother. */
#define TEL3_HALF       (1<<19) /* center point means one of the edges of the line */
#define TEL3_QUAT       (1<<20) /* use quaternion to express rotation */
#define TEL3_HITFUNC    (1<<21) /* use w->vft->pointhit to determine intersection into ground */
#define TEL3_ACCELFUNC  (1<<22) /* use w->vft->accel to determine gravity */

/* tefpol_flags_t */
#define TEP_FINE        (1<<0) /* finer granularity for graduation */
#define TEP_SHAKE       (1<<1) /* randomly shift vertices per rendering */
#define TEP_WIND        (1<<2) /* wake of polyline will wind */
#define TEP_RETURN      (1<<3) /* lines gone beyond border will appear at opposide edge */
#define TEP_DRUNK       (1<<4) /* add random velocity to the head per step */
#define TEP3_REFLECT     (1<<5) /* whether reflect on border */
#define TEP3_ROUGH		(1<<6) /* use less vertices */
#define TEP3_THICK       (1<<7) /* enthicken polyline to 2 pixels wide */
#define TEP3_THICKER     (2<<7) /* enthicken polyline to 3 pixels wide */
#define TEP3_THICKEST    (3<<7) /* enthicken polyline to 4 pixels wide */
#define TEP3_FAINT       (4<<7) /* have volume but dim */
#define TEP3_HEAD        (1<<10) /* append head graphic */
#define TEP_ADDBLEND    (0<<14) /* additive; default */
#define TEP_SOLIDBLEND  (1<<14) /* transparency mode */
#define TEP_ALPHABLEND  (2<<14) /* alpha indexed */
#define TEP_SUBBLEND    (3<<14) /* subtractive */
/* teline's forms can be used as well. */


struct tent3d_line_list;
struct tent3d_line_callback{
	double pos[3]; /* center point, double because the velo may be so slow */
	double velo[3]; /* velocity. */
	double len; /* length of the beam */
/*	double pyr[3]; *//* pitch, yaw and roll angles of the beam */
	double rot[4]; /* rotation in quaternion form */
	double omg[3]; /* angle velocity */
	double life; /* remaining rendering time */
};
struct tent3d_line_drawdata{
	double viewpoint[3], viewdir[3], rot[4], invrot[16];
	double fov; /* field of view */

	/* following are trivial members; caller need not to set them */
	struct glcull *pgc;
};
struct tent3d_fpol_list;

struct tent3d_line_list *NewTeline3D(unsigned maxt, unsigned init, unsigned unit); /* constructor */
struct tent3d_line_list *NewTeline3DFunc(unsigned maxt, unsigned init, unsigned unit, struct war_field*); /* constructor */
void DelTeline3D(struct tent3d_line_list *); /* destructor */
void AddTeline3D(struct tent3d_line_list *tell, const double pos[3], const double velo[3], double len, const double pyr[3], const double omg[3], const double grv[3], COLOR32 col, tent3d_flags_t flags, double life);
void AddTelineCallback3D(struct tent3d_line_list *tell, const double pos[3], const double velo[3], double len, const double pyr[3], const double omg[3], const double grv[3], void (*draw_proc)(const struct tent3d_line_callback*, const struct tent3d_line_drawdata*, void *private_data), void *private_data, tent3d_flags_t flags, double life);
void AnimTeline3D(struct tent3d_line_list *tell, double dt);
void DrawTeline3D(struct tent3d_line_list *tell, struct tent3d_line_drawdata *);

struct tent3d_fpol_list *NewTefpol3D(unsigned maxt, unsigned init, unsigned unit);
void AddTefpol3D(struct tent3d_fpol_list *tepl, const double pos[3], const double velo[3], const double gravity[3], const struct color_sequence *col, tent3d_flags_t flags, double life);
struct tent3d_fpol *AddTefpolMovable3D(struct tent3d_fpol_list *tepl, const double pos[3], const double velo[3], const double gravity[3], const struct color_sequence *col, tent3d_flags_t flags, double life);
void MoveTefpol3D(struct tent3d_fpol *fpol, const double pos[3], const double velo[3], double life, int skip);
void ImmobilizeTefpol3D(struct tent3d_fpol*);
void AnimTefpol3D(struct tent3d_fpol_list *tell, double dt);
void DrawTefpol3D(struct tent3d_fpol_list *tepl, const double viewpoint[3], const struct glcull *);

#ifndef NPROFILE
const struct tent3d_line_debug{
	double drawteline;
	size_t so_teline3_t;
	unsigned teline_c;
	unsigned teline_m;
	unsigned teline_s;
} *Teline3DDebug(const struct tent3d_line_list *);
const struct tent3d_fpol_debug{
	double animtefpol, drawtefpol;
	size_t so_tefpol3_t, so_tevert3_t;
	unsigned tefpol_c;
	unsigned tefpol_m;
	unsigned tefpol_s;
	unsigned tevert_c;
	unsigned tevert_s;
} *Tefpol3DDebug(const struct tent3d_fpol_list *);
#endif


#endif
