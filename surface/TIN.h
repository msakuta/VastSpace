/** \file
 * \brief Definition of Triangular Irregular Network surface mapping class.
 */
#ifndef TIN_H
#define TIN_H
#include "WarMap.h"
#include <cpplib/vec3.h>
#include <vector>
#include <map>
#include <set>

class TIN : public WarMap{
public:
	TIN(const char *fname);
	int getat(WarMapTile *return_info, int x, int y)override;
	void size(int *retx, int *rety)override;
	double width()override;
//	virtual Vec3d normal(double x, double y);
//	virtual int linehit(const Vec3d &src, const Vec3d &dir, double t, Vec3d &ret);
	virtual ~TIN();
//	virtual void levelterrain(int x0, int y0, int x1, int y1){}
//	virtual const void *rawData()const{return NULL;} ///< Access to the raw data, NULL if not available
//	virtual PHY_ScalarType rawType()const{return PHY_SHORT;} ///< Type of raw data, if available
//	double height(double x, double y, Vec3d *normal);
//	void altterrain(int x0, int y0, int x1, int y1, void altfunc(WarMap *, int x, int y, void *hint), void *hint);
	/* Road check */
//	double heightr(double x, double y, Vec3d *normal);
//	int nalt;
//	struct warmap_alt{
//		void (*altfunc)(WarMap *, int, int, void *);
//		void *hint;
//		int x0, y0, x1, y1;
//	} *alt;
	void draw();

	double getHeight(double x, double y, const Vec3d *scales = NULL, Vec3d *normal = NULL)const;
	bool traceHit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal)const;
protected:
	struct Vertex;

	struct AABB{
		Vec3d mins;
		Vec3d maxs;
	};

	struct Triangle{
		Vec3d normal;
		Vec3i vertices[3];
		Vertex *vrefs[3];
		AABB getAABB()const;
	};
	typedef std::vector<Triangle> Triangles;
	Triangles triangles;

	/// List of Triangle pointers
	typedef std::vector<Triangle*> TriangleList;
	TriangleList sortx;

	static const int GridSize = 16;
	typedef std::vector<Triangle*> GridCell;
	typedef GridCell TriangleGrid[GridSize][GridSize];
	TriangleGrid tgrid;

	typedef std::set<int> VertexRefSet;
	struct Vertex{
		Vec3d normal;
		VertexRefSet vset;
	};
	typedef std::map<Vec3i, Vertex, bool (*)(const Vec3i &, const Vec3i &)> Vertices;
	Vertices vertices;

	static bool vertexPredicate(const Vec3i &a, const Vec3i &b);
	static double triangleMaxX(const Triangle &a);
	static bool trianglePredicate(const Triangle *a, const Triangle *b);
};



inline TIN::AABB TIN::Triangle::getAABB()const{
	AABB ret = {Vec3d(DBL_MAX, DBL_MAX, DBL_MAX), Vec3d(DBL_MIN, DBL_MIN, DBL_MIN)};
	for(int i = 0; i < 3; i++){
		for(int j = 0; j < 3; j++){
			if(vertices[j][i] < ret.mins[i])
				ret.mins[i] = vertices[j][i];
			if(ret.maxs[i] < vertices[j][i])
				ret.maxs[i] = vertices[j][i];
		}
	}
	return ret;
}

#endif
