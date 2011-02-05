#ifndef WARDRAW_H
#define WARDRAW_H
#include "war.h"
#include <gl/gl.h>

struct WarDraw{
//	unsigned listLight; /* OpenGL list name to switch on a light to draw solid faces */
//	void (*light_on)(void); /* processes to turn */
//	void (*light_off)(void); /* the light on and off. */
//	COLOR32 ambient; /* environmental color; non light-emitting objects dyed by this */
//	unsigned char hudcolor[4];
//	int irot_init; /* inverse rotation */
//	double (*rot)[16]; /* rotation matrix */
//	double irot[16];
//	double view[3]; /* view position */
//	double viewdir[3]; /* unit vector pointing view direction */
//	double fov; /* field of view */
//	GLcull *pgc;
	Viewer *vw;
	Vec3d light; ///< light direction
//	double gametime;
//	double maprange;
	int lightdraws;
	bool shadowmapping; ///< Wheter this pass draws shadow map.
	GLubyte texShadow; ///< The texture name for the shadow map.
	WarField *w;
	WarDraw() : shadowmapping(false), texShadow(0){}
};

typedef WarDraw war_draw_data;



#endif
