#ifndef WARMAP_H
#define WARMAP_H
#include <cpplib/vec3.h>

#define SHADOWS 8 /* count of shadow textures */

typedef struct warmap_tile{
	double height; /* altitude */
	/*int grass;*/ /* grass density */
} wartile_t;

class WarMap{
public:
	WarMap();
	virtual int getat(wartile_t *return_info, int x, int y) = 0;
	virtual void size(int *retx, int *rety) = 0;
	virtual double width() = 0;
	virtual Vec3d normal(double x, double y);
	virtual int linehit(const Vec3d &src, const Vec3d &dir, double t, Vec3d &ret);
	virtual ~WarMap();
	virtual void levelterrain(int x0, int y0, int x1, int y1) = 0;
	double height(double x, double y, Vec3d *normal);
	void altterrain(int x0, int y0, int x1, int y1, void altfunc(WarMap *, int x, int y, void *hint), void *hint);
	/* Road check */
	double heightr(double x, double y, Vec3d *normal);
	int nalt;
	struct warmap_alt{
		void (*altfunc)(WarMap *, int, int, void *);
		void *hint;
		int x0, y0, x1, y1;
	} *alt;
};

WarMap *OpenMas(const char *fname);
WarMap *OpenSPMMas(const char *fname);
WarMap *CreateMemap(void);
WarMap *RepeatMap(WarMap *src, int xmin, int ymin, int xmax, int ymax);
WarMap *OpenHGTMap(const char *fname);




#endif
