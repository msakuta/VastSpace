/** \file
 * \brief Definition of Mesh and MeshTex classes.
 */
#ifndef MESH_H
#define MESH_H
#include "Mesh-forward.h"
#include "export.h"
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <vector>

#define EXTENDABLE 1

/// \brief A mesh of polygon surfaces.
///
/// This class is designed to be as much compatible as possible to clib's suf_t,
/// but defined in C++ for better integration to the application.
///
/// Keep it a POD (Plain Old Data) or the compatibility may be broken.
/// This also applies to all sub-structs.
///
/// Historically, suf_t structure had been derived from PolyEdit, a very old
/// 3D model designer.  We no longer support PolyEdit compatibility, so we are
/// free to alter the data structure.
struct EXPORT Mesh{
	typedef double Coord; ///< Internal coordinate value type
	typedef unsigned short Index; ///< Internal index value for attributes and vertex buffers.
	typedef Coord Vec[3]; ///< Internal vector class in 3-D.
	typedef unsigned Flags; ///< Flag fundamental type

	static const Index INDEX_MAX = USHRT_MAX; ///< Maximum value of an Index variable.

	// draw flags
	static const Flags Attribs  = 0xfff; ///< enable attribute-file specified material if one exists
	static const Flags Color    = 1<<0; ///< validity flags
	static const Flags Trans    = 1<<1;
	static const Flags Ambient  = 1<<2; ///< Ambient
	static const Flags Diffuse  = 1<<3; ///< Diffuse
	static const Flags Specular = 1<<4; ///< Specular
	static const Flags Texture  = 1<<5; ///< Texture presence
	static const Flags Emission = 1<<6; ///< Emission
	static const Flags MultiTex = 1<<7; ///< multi-texture presence
	static const Flags VtxArray = 1<<8; ///< use vertex array to draw polygons
	static const Flags Shine    = 1<<9; ///< Shininess

	int nv;
	Vec *v;

	/// \brief Attribute parameters compatible to sufatr_t.
	struct Attrib{
		char *name;
		Flags valid; /* validity flag of following values */
		float col[4]; /* clamped */
		float tra[4];
		float amb[4]; /* ambient */
		float emi[4]; /* emission */
		float dif[4]; /* diffuse */
		float spc[4]; /* specular lighting */
		float shininess; ///< Shininess parameter
		char *colormap; /* texture color map name */
		Coord mapsize[4];
		Coord mapview[4];
		Coord mapwind[4];
	};

	Index na;
	Attrib *a;

	/// \brief Type of primitive.
	enum ElemType{ET_Polygon, ET_Shade, ET_UVPolygon, ET_UVShade};

	/// \brief Polygon primitive.
	struct Polygon{
		struct Vertex{
			Index pos, nrm; /* position and normal coordinates. */
		};

		ElemType t;
		int n;
		Index atr; /* index into attribute list */
		Vertex v[EXTENDABLE]; /* index into vertex list, position and normal */
	};

	/// \brief UV-mapped polygon primitive.
	struct UVPolygon{
		struct Vertex : Polygon::Vertex{
			Index tex; /* position, normal and texture coordinates. */
		};

		ElemType t;
		int n;
		Index atr;
		Vertex v[EXTENDABLE];
	};

	/// \brief Union of all possible primitives.
	union Primitive{
		ElemType t;
		Polygon p;
		UVPolygon uv;
	};

	int np;
	Primitive **p;

	/// \brief Attribute cache that could be used to optimize performance (though somewhat unstable).
	struct Cache{
		Flags valid;
		float matdif[4];
		float matamb[4];
		float matemi[4];
		float matspc[4];
		float matshininess;
		GLuint texture;
		int texenabled;
	};

	/// \brief Deprecated decal data, provided only for compatibility for the time being.
	struct Decal{
		void (*drawproc)(void*, Cache *, void *global_var);
		void (*freeproc)(void*);
		int np;
		void *p[1];
	};

	/// \brief A set of vertex buffer object indices.
	struct BufferSet{
		GLuint pos; ///< Position vector buffer.
		GLuint nrm; ///< Normal vector buffer.
		GLuint tex; ///< Texture coordinates buffer.
		GLuint ind; ///< Index buffer into all three buffers above.
		unsigned count; ///< Count of vertices ind member contains.
	};
	typedef std::vector<BufferSet> Buffers;

	mutable Buffers buf;

	~Mesh();

	Attrib *find_atr(const char *name);
	void draw(Flags flags, Cache *)const;
	static void gldMaterialfv(GLenum face, GLenum pname, const GLfloat *params, Cache *c);
	void polyDraw(Flags flags, Cache *c, int i, Index *plast, const MeshTex *)const;
	void drawPoly(int i, Flags flags, Cache *c)const;
	void decalDraw(Flags flags, Cache *c, const MeshTex *tex, Decal *sd, void *sdg)const;
	Index add_vertex(const Vec &v);
	Primitive **add_poly();
	static void add_polyvert(Polygon **p, Index i, Index j);
	static void add_uvpolyvert(UVPolygon **uv, Index i, Index j, Index k);
protected:
	void beginMaterial(Attrib *atr, Flags flags, Cache *c)const;
	void beginTexture(Attrib *atr, bool mismatch, const MeshTex *tex, int ai, Cache *c)const;
	void compileVertexBuffers()const;
};

/// \brief A texture cache used for drawing a Mesh.
///
/// This class is similar to suftex_t.
///
/// Keep it a POD (Plain Old Data) or the compatibility may be broken.
struct EXPORT MeshTex{
	/// A internal structure to store texture unit and its initialization methods.
	struct MeshTexList{
		GLuint list; ///< OpenGL list executed when the beginning of texture
		GLuint tex[2]; ///< Texture names for each multitexture
		double scale; ///< Scale of the second texture (for detail texturing)
		void (*onBeginTexture)(void *data); ///< Called just before the texture is activated
		void *onBeginTextureData; ///< Argument for onBeginTexture
		void (*onInitedTexture)(void *data); ///< Called just after the texture is initialized
		void *onInitedTextureData; ///< Argument for onInitedTexture
		void (*onEndTexture)(void *data); ///< Called when the texture unit is ended.
		void *onEndTextureData; ///< Argument for onEndTexture
	};

	unsigned n;
	struct MeshTexList a[1];
};


#endif
