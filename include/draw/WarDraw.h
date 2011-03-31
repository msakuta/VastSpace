#ifndef WARDRAW_H
#define WARDRAW_H
#include "war.h"
#include <gl/gl.h>

class ShadowMap;

/// Parameters when drawing a WarSpace.
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
	Viewer *vw; ///< The viewer object
	Vec3d light; ///< Light direction in world coordinates
//	double gametime;
//	double maprange;
	int lightdraws; ///< Counter of drawing of lit objects
	ShadowMap *shadowMap; ///< The shadow mapping object, non NULL if active.
	bool shadowmapping; ///< Wheter this pass draws shadow map.
	GLubyte texShadow; ///< The texture name for the shadow map.
	GLuint shader; ///< GLSL shader unit name or 0 if not available.
	GLint textureLoc; ///< GLSL location in shader for texture uniform value.
	GLint shadowmapLoc;  ///< GLSL location in shader for shadowmap uniform value.
	WarSpace *w; ///< Reference to the associated WarSpace.
	WarDraw() : shadowmapping(false), shadowMap(NULL), texShadow(0), shader(0), textureLoc(0), shadowmapLoc(0){}
};

typedef WarDraw war_draw_data;



#endif
