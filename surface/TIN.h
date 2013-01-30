/** \file
 * \brief Definition of Triangular Irregular Network surface mapping class.
 */
#ifndef TIN_H
#define TIN_H
#include <cpplib/vec3.h>
#include <vector>
#include <map>
#include <set>

class TIN{
public:
	TIN(const char *fname);
//	virtual int getat(WarMapTile *return_info, int x, int y) = 0;
//	virtual void size(int *retx, int *rety) = 0;
	virtual double width();
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
protected:
	struct Vertex;

	struct Triangle{
		Vec3d normal;
		Vec3i vertices[3];
		Vertex *vrefs[3];
	};
	typedef std::vector<Triangle> Triangles;
	Triangles triangles;

	typedef std::set<int> VertexRefSet;
	struct Vertex{
		Vec3d normal;
		VertexRefSet vset;
	};
	typedef std::map<Vec3i, Vertex, bool (*)(const Vec3i &, const Vec3i &)> Vertices;
	Vertices vertices;

	static bool vertexPredicate(const Vec3i &a, const Vec3i &b);
};


#endif
