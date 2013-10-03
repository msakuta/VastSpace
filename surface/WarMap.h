#ifndef WARMAP_H
#define WARMAP_H
#include <cpplib/vec3.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#define SHADOWS 8 /* count of shadow textures */

struct WarMapTile{
	double height; /* altitude */
	/*int grass;*/ /* grass density */

	WarMapTile &operator+=(const WarMapTile &o){
		height += o.height;
		return *this;
	}
	WarMapTile operator+(const WarMapTile &o)const{
		return WarMapTile(*this) += o;
	}
	WarMapTile &operator*=(double d){
		height *= d;
		return *this;
	}
	WarMapTile operator*(int d){
		return WarMapTile(*this) *= d;
	}
	WarMapTile &operator/=(int d){
		height /= d;
		return *this;
	}
	WarMapTile operator/(int d){
		return WarMapTile(*this) /= d;
	}
};

class WarMap{
public:
	struct TraceParams{
		TraceParams(Vec3d &src, Vec3d &dir, double traceTime = 1., double radius = 0.)
			: src(src), dir(dir), traceTime(traceTime), radius(radius){}
		Vec3d src;
		Vec3d dir;
		double traceTime;
		double radius;
	};
	struct TraceResult{
		double hitTime;
		Vec3d position;
		Vec3d normal;
	};
	WarMap();
	virtual int getat(WarMapTile *return_info, int x, int y) = 0;
	virtual void size(int *retx, int *rety) = 0;
	virtual double width() = 0;
	virtual Vec3d normal(double x, double y);
	virtual bool traceHit(const TraceParams &params, TraceResult &result)const;
	virtual ~WarMap();
	virtual void levelterrain(int x0, int y0, int x1, int y1){}
	virtual const void *rawData()const{return NULL;} ///< Access to the raw data, NULL if not available
	virtual PHY_ScalarType rawType()const{return PHY_SHORT;} ///< Type of raw data, if available
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
