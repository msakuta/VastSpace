#ifndef ASTRODRAW_H
#define ASTRODRAW_H
#include "astro.h"
#include "Viewer.h"
#include <clib/colseq/color.h>
#include <cpplib/RandomSequence.h>
#include <cpplib/quat.h>
#ifdef _WIN32
#define exit something_meanless
#include <windows.h>
#undef exit
#endif
#include <GL/gl.h>
/*#include <GL/glut.h>*/

#define LIGHT_SPEED 299792.458 /* km/s */

void drawIcosaSphere(const Vec3d &org, double radius, const Viewer &vw, const Vec3d &scales = Vec3d(1,1,1), const Quatd &qrot = quat_u);

/*
extern struct glcull g_glcull;

void directrot(const double pos[3], const double base[3], amat4_t irot);
void rgb2hsv(double hsv[3], double r, double g, double b);
void hsv2rgb(double rgb[3], const double hsv[3]);
void doppler(double rgb[3], double r, double g, double b, double velo);
void drawPoint(const struct astrobj *a, const Viewer *p, const avec3_t pspos, const avec3_t sunp, const GLubyte color[4]);
void drawpsphere(Astrobj *ps, const Viewer *, COLOR32 col);
void drawShadeSphere(const struct astrobj *ps, const Viewer *p, const avec3_t sunpos, const GLubyte color[4], const GLubyte dark[4]);
void drawTextureSphere(const struct astrobj *a, const Viewer *vw, const avec3_t sunpos, GLfloat mat_diffuse[4], GLfloat mat_ambient[4], GLuint *texlist, const amat4_t texmat, const char *texname);
void drawSphere(const struct astrobj *a, const Viewer *vw, const avec3_t sunpos, GLfloat mat_diffuse[4], GLfloat mat_ambient[4]);

GLuint ProjectSphereMap(const char *name, const BITMAPINFO*);
GLuint ProjectSphereJpg(const char *fname);

void LightOn(void);
void LightOff(void);
extern double g_star_num;
extern int g_star_cells;
extern double g_star_visible_thresh;
extern double g_star_glow_thresh;
extern int g_invert_hyperspace;*/

EXPORT void drawstarback(const Viewer *vw, const CoordSys *csys, const Astrobj *pe, const Astrobj *sun);


/// \brief A class to enumerate randomly generated stars.
///
/// This routine had been embedded in drawstarback(), but it may be used by some other routines.
class StarEnum{
public:
	/// \brief The constructor.
	/// \param originPos The center of star generation.  This is usually the player's viewpoint.
	/// \param numSectors Count of sectors to generate around the center.
	///        Higher value generates more stars at once, but it may cost more computing time.
	StarEnum(const Vec3d &originPos, int numSectors, bool genNames = false);

	/// \brief Retrieves next star in this star generation sequence.
	/// \param pos Position of generated star.
	/// \param name Name of generated star.
	bool next(Vec3d &pos, gltestp::dstring *name = NULL);

	/// \brief Returns the random sequence for current star.
	RandomSequence *getRseq();

protected:
	Vec3d plpos;
	Vec3d cen;
	RandomSequence rs; ///< Per-sector random number sequence
	RandomSequence rsStar; ///< Per-star random number sequence
	int numSectors; ///< Size of cube measured in sectors to enumerate
	int gx, gy, gz;
	int numstars;
	bool genNames;
	bool newCell();
};


#endif
