/// \file
/// \brief Definition and implementation of VoxelEntity class.
#define NOMINMAX
#include "Entity.h"
#include "EntityRegister.h"
#ifdef _WIN32
#include <Windows.h>
#endif
#include "draw/WarDraw.h"
#include "Viewer.h"
#include "CoordSys.h"
#include "Game.h"
#include "EntityCommand.h"
#include "sqadapt.h"
#include "draw/ShadowMap.h"
#include "draw/ShaderBind.h"
#include "draw/material.h"
#include "glstack.h"
#include "noises/simplexnoise1234d.h"
#include "SignModulo.h"
#include "draw/mqoadapt.h"
#include "draw/VBO.h"
#include "draw/MarchingCube.h"

extern "C"{
#include "clib/mathdef.h"
}

#include <cpplib/vec2.h>

const int CELLSIZE = 16;

class Game;
class VoxelEntity;

/// <summary>The atomic unit of the world.</summary>
class Cell{
public:
	enum Type{
		Air, ///< Null cell
		Rock, ///< Rock cell
		Iron, ///< Iron ore cell
		Armor,
		ArmorSlope,
		ArmorCorner,
		ArmorInvCorner,
		Engine,
		Occupied,
		NumTypes
	};

	Cell(Type t = Air, char arotation = 0) : type(t), adjacents(0), rotation(arotation){}
	Type getType()const{return type;}
	short getValue()const{return value;}
	void setValue(short avalue){value = avalue;}
	int getAdjacents()const{return adjacents;}
	char getRotation()const{return rotation;}
	bool isSolid()const{return type != Air;}
	bool isTranslucent()const{
		return type == Air || type == ArmorSlope || type == ArmorCorner || type == ArmorInvCorner
			|| type == Occupied;
	}
	void serialize(std::ostream &o);
	void unserialize(std::istream &i);

	/// \brief Query if a connection to a given direction can be secured for this block.
	/// \param direction Vector to query the information, must be unit length, single axis vector.
	bool connection(const Vec3i &direction)const;

	/// \brief Calculate rotated vector by given rotation code
	static Vec3i rotate(char rotation, const Vec3i &dir);

	/// \brief Calculate inverse rotated vector by given rotation code
	static Vec3i irotate(char rotation, const Vec3i &dir);
protected:
	enum Type type;
	short value;
	char adjacents;
	char rotation;

	friend class CellVolume;
};

struct VERTEX{
	Vec3d pos;
	Vec3d norm;
	Vec2d tex;
	bool operator==(const VERTEX &o)const{
		return pos == o.pos && norm == o.norm && tex == o.tex;
	}
};

class CellVolume{
public:
	static const Cell v0;

protected:
	VoxelEntity *world;
	Vec3i index;
	Cell v[CELLSIZE][CELLSIZE][CELLSIZE];

	enum Material{
		MatRock,
		MatIron,
		MatArmor,
		Num_Mat
	};

	int vlistCounts[Num_Mat];
	int vidxCounts[Num_Mat];
	GLuint vbo[Num_Mat];
	GLuint vboIdx[Num_Mat];
	bool vboDirty;

	mutable std::vector<Vec3i> modeledCells;

	/// <summary>
	/// Indices are in order of [X, Z, beginning and end]
	/// </summary>
	int _scanLines[CELLSIZE][CELLSIZE][2];

	/// Scanlines for transparent cells
	int tranScanLines[CELLSIZE][CELLSIZE][2];

	int _solidcount;
	int bricks[Cell::NumTypes];

	void updateAdj(int ix, int iy, int iz);
public:
	CellVolume(VoxelEntity *world = NULL, const Vec3i &ind = Vec3i(0,0,0)) : world(world), index(ind), _solidcount(0), vboDirty(true){
		for(int i = 0; i < numof(vbo); i++)
			vbo[i] = 0;
		for(int i = 0; i < numof(vboIdx); i++)
			vboIdx[i] = 0;
		for(int ix = 0; ix < CELLSIZE; ix++) for(int iy = 0; iy < CELLSIZE; iy++) for(int iz = 0; iz < 2; iz++){
			_scanLines[ix][iy][iz] = 0;
			tranScanLines[ix][iy][iz] = 0;
		}
		for(int i = 0; i < Cell::NumTypes; i++)
			bricks[i] = 0;
	}
	const Vec3i &getIndex()const{return index;}
	const Cell &operator()(int ix, int iy, int iz)const;
	const Cell &cell(int ix, int iy, int iz)const{
		return operator()(ix, iy, iz);
	}
	const Cell &cell(const Vec3i &ipos)const{
		return cell(ipos[0], ipos[1], ipos[2]);
	}
	bool isSolid(const Vec3i &ipos)const{
		return
			0 <= ipos[0] && ipos[0] < CELLSIZE &&
			0 <= ipos[1] && ipos[1] < CELLSIZE &&
			0 <= ipos[2] && ipos[2] < CELLSIZE &&
			v[ipos[0]][ipos[1]][ipos[2]].getType() != Cell::Air;
	}
	bool setCell(int ix, int iy, int iz, const Cell &newCell);
	void initialize(const Vec3i &index);
	void updateCache();
	typedef int ScanLinesType[CELLSIZE][CELLSIZE][2];
	const ScanLinesType &getScanLines()const{
		return _scanLines;
	}
	const ScanLinesType &getTranScanLines()const{
		return tranScanLines;
	}
	int getSolidCount()const{return _solidcount;}
	int getBricks(int i)const{return bricks[i];}

	void serialize(std::ostream &o);
	void unserialize(std::istream &i);

	static int cellInvokes;
	static int cellForeignInvokes;
	static int cellForeignExists;
	friend class VoxelEntity;
};

inline bool operator<(const Vec3i &a, const Vec3i &b){
	if(a[0] < b[0])
		return true;
	else if(b[0] < a[0])
		return false;
	else if(a[1] < b[1])
		return true;
	else if(b[1] < a[1])
		return false;
	else if(a[2] < b[2])
		return true;
	else
		return false;
}

/// \brief An EntityCommand for making changes to cells in VoxelEntity
///
/// It can place or remove a voxel by parameters.
struct EXPORT ModifyVoxelCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(ModifyVoxelCommand, EntityCommand);
	ModifyVoxelCommand(){}
	ModifyVoxelCommand(HSQUIRRELVM v, Entity &e);

	// Vector parameters of a ray for placing or removing a voxel.
	Vec3d src;
	Vec3d dir;

	enum Mode{Remove, Put, Preview};
	Mode mode; ///< Whether to put a voxel (false means removing)
	Cell::Type ct; ///< Cell type to place
	char rotation;

	virtual void serialize(SerializeContext &);
	virtual void unserialize(UnserializeContext &);
};

class EXPORT VoxelEntity : public Entity{
public:
	typedef Entity st;

	VoxelEntity(Game *game) : st(game){}
	VoxelEntity(WarField *w);

	static EntityRegister<VoxelEntity> entityRegister;

//	virtual void serialize(SerializeContext &sc);
//	virtual void unserialize(UnserializeContext &sc);
//	virtual void addRigidBody(WarSpace*);
	void anim(double dt)override;
	void draw(WarDraw *)override;
	void drawtra(WarDraw *)override;
//	virtual void drawHUD(wardraw_t *);
//	virtual void control(const input_t *, double);
//	unsigned analog_mask();
//	virtual Props props()const;
//	struct ManeuverParams;
//	virtual const ManeuverParams &getManeuve()const;
	bool isTargettable()const override{return true;}
	bool isSelectable()const override{return true;}
	bool command(EntityCommand *com)override;
	int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn)override{return 0;}

//	virtual short getDefaultCollisionMask()const;

	double getHitRadius()const override{return getBaseHeight() + getNoiseHeight() * 2.;}

	typedef std::map<Vec3i, CellVolume, bool(*)(const Vec3i &, const Vec3i &)> VolumeMap;

	Vec3i real2ind(const Vec3d &pos)const;
	Vec3d ind2real(const Vec3i &ipos)const;
	double getCellWidth()const{return 0.0025;} ///< Length of an edge of a cell, in kilometers
	double getBaseHeight()const{return 0.05;}
	double getNoiseHeight()const{return 0.015;}

	const Cell &cell(int ix, int iy, int iz){
		Vec3i ci = Vec3i(SignDiv(ix, CELLSIZE), SignDiv(iy, CELLSIZE), SignDiv(iz, CELLSIZE));
		VolumeMap::iterator it = volume.find(ci);
		if(it != volume.end()){
			return it->second(SignModulo(ix, CELLSIZE), SignModulo(iy, CELLSIZE), SignModulo(iz, CELLSIZE));
		}
		else
			return CellVolume::v0;
	}
	const Cell &cell(const Vec3i &pos){
		return cell(pos[0], pos[1], pos[2]);
	}

	bool setCell(int ix, int iy, int iz, const Cell &newCell){
		Vec3i ci = Vec3i(SignDiv(ix, CELLSIZE), SignDiv(iy, CELLSIZE), SignDiv(iz, CELLSIZE));
		VolumeMap::iterator it = volume.find(ci);
		if(it != volume.end())
			return it->second.setCell(SignModulo(ix, CELLSIZE), SignModulo(iy, CELLSIZE), SignModulo(iz, CELLSIZE), newCell);
		return false;
	}

	bool isSolid(int ix, int iy, int iz){
		return isSolid(Vec3i(ix, iy, iz));
	}

	bool isSolid(const Vec3i &v);
	bool isSolid(const Vec3d &rv);

protected:
	void drawCell(const Cell &cell, const Vec3i &pos, Cell::Type &celltype, const CellVolume *cv = NULL, const Vec3i &posInVolume = Vec3i(0,0,0),
		std::vector<VERTEX> *vlist = NULL, std::vector<GLuint> *vidx = NULL)const;
private:
	VolumeMap volume;
	int bricks[Cell::NumTypes];
	Vec3i previewCellPos;
	Cell previewCell;

	static bool texlist_init;
	static GLuint texlist_rock;
	static GLuint texlist_iron;
	static GLuint texlist_armor;
	static void initTexLists();

	friend class CellVolume;
};


Entity::EntityRegister<VoxelEntity> VoxelEntity::entityRegister("VoxelEntity");

bool VoxelEntity::texlist_init = false;
GLuint VoxelEntity::texlist_rock;
GLuint VoxelEntity::texlist_iron;
GLuint VoxelEntity::texlist_armor;



bool Cell::connection(const Vec3i &direction)const{
	if(type == Air)
		return false; // Air cannot connect to anything
	if(type != ArmorSlope && type != ArmorCorner && type != ArmorInvCorner)
		return true;
	Quatd rot = Quatd::rotation(2. * M_PI / 4. * (rotation & 3), 1, 0, 0)
		.rotate(2. * M_PI / 4. * ((rotation >> 2) & 3), 0, 1, 0)
		.rotate(2. * M_PI / 4. * ((rotation >> 4) & 3), 0, 0, 1);
	// Transform the direction vector to the cell's local coordinate system to
	// examinie if a connection to the direction can be kept.
	Vec3d dirAfterRot = rot.itrans(direction.cast<double>());
	dirAfterRot = irotate(rotation, direction).cast<double>();
	// Use 0.5 threshold to prevent floating point errors from affecting result
	if(type == ArmorSlope)
		return !(0.5 < dirAfterRot[0] || 0.5 < dirAfterRot[1]);
	else if(type == ArmorCorner)
		return !(0.5 < dirAfterRot[0] || 0.5 < dirAfterRot[1] || 0.5 < dirAfterRot[2]);
	else // if(type == ArmorInvCorner)
		return true;
}

Vec3i Cell::rotate(char rotation, const Vec3i &dir){
	Vec3i ret;
	switch(rotation & 3){
	case 1: ret = Vec3i(dir[0], -dir[2], dir[1]); break;
	case 2: ret = Vec3i(dir[0], -dir[1], -dir[2]); break;
	case 3: ret = Vec3i(dir[0], dir[2], -dir[1]); break;
	default: ret = dir; break;
	}
	switch((rotation >> 2) & 3){
	case 1: ret = Vec3i(ret[2], ret[1], -ret[0]); break;
	case 2: ret = Vec3i(-ret[0], ret[1], -ret[2]); break;
	case 3: ret = Vec3i(-ret[2], ret[1], ret[0]); break;
	}
	switch((rotation >> 4) & 3){
	case 1: ret = Vec3i(-ret[1], ret[0], ret[2]); break;
	case 2: ret = Vec3i(-ret[0], -ret[1], ret[2]); break;
	case 3: ret = Vec3i(ret[1], -ret[0], ret[2]); break;
	}
	return ret;
}

Vec3i Cell::irotate(char rotation, const Vec3i &dir){
	Vec3i ret;
	switch(rotation & 3){
	case 1: ret = Vec3i(dir[0], dir[2], -dir[1]); break;
	case 2: ret = Vec3i(dir[0], -dir[1], -dir[2]); break;
	case 3: ret = Vec3i(dir[0], -dir[2], dir[1]); break;
	default: ret = dir; break;
	}
	switch((rotation >> 2) & 3){
	case 1: ret = Vec3i(-ret[2], ret[1], ret[0]); break;
	case 2: ret = Vec3i(-ret[0], ret[1], -ret[2]); break;
	case 3: ret = Vec3i(ret[2], ret[1], -ret[0]); break;
	}
	switch((rotation >> 4) & 3){
	case 1: ret = Vec3i(ret[1], -ret[0], ret[2]); break;
	case 2: ret = Vec3i(-ret[0], -ret[1], ret[2]); break;
	case 3: ret = Vec3i(-ret[1], ret[0], ret[2]); break;
	}
	return ret;
}



/// <summary>Returns Cell object indexed by coordinates in this CellVolume.</summary>
/// <remarks>
/// If the indices reach border of the CellVolume, it will recursively retrieve foreign Cells.
///
/// Note that even if two or more indices are out of range, this function will find the correct Cell
/// by recursively calling itself in turn with each axes.
/// But what's the difference between this and World::cell(), you may ask. That's the key for the next
/// step of optimization.
/// </remarks>
/// <param name="ix">Index along X axis in Cells. If in range [0, CELLSIZE), this object's member is returned.</param>
/// <param name="iy">Index along Y axis in Cells. If in range [0, CELLSIZE), this object's member is returned.</param>
/// <param name="iz">Index along Z axis in Cells. If in range [0, CELLSIZE), this object's member is returned.</param>
/// <returns>A CellInt object at ix, iy, iz</returns>
inline const Cell &CellVolume::operator()(int ix, int iy, int iz)const{
	cellInvokes++;
	if(ix < 0 || CELLSIZE <= ix){
		cellForeignInvokes++;
		Vec3i ci(index[0] + SignDiv(ix, CELLSIZE), index[1], index[2]);
		VoxelEntity::VolumeMap::iterator it = world->volume.find(ci);
		if(it != world->volume.end()){
			cellForeignExists++;
			const CellVolume &cv = it->second;
			return cv(SignModulo(ix, CELLSIZE), iy, iz);
		}
		else
			return (*this)(ix < 0 ? 0 : CELLSIZE - 1, iy, iz);
	}
	if(iy < 0 || CELLSIZE <= iy){
		cellForeignInvokes++;
		Vec3i ci(index[0], index[1] + SignDiv(iy, CELLSIZE), index[2]);
		VoxelEntity::VolumeMap::iterator it = world->volume.find(ci);
		if(it != world->volume.end()){
			cellForeignExists++;
			const CellVolume &cv = it->second;
			return cv(ix, SignModulo(iy, CELLSIZE), iz);
		}
		else
			return (*this)(ix, iy < 0 ? 0 : CELLSIZE - 1, iz);
	}
	if(iz < 0 || CELLSIZE <= iz){
		cellForeignInvokes++;
		Vec3i ci(index[0], index[1], index[2] + SignDiv(iz, CELLSIZE));
		VoxelEntity::VolumeMap::iterator it = world->volume.find(ci);
		if(it != world->volume.end()){
			cellForeignExists++;
			const CellVolume &cv = it->second;
			return cv(ix, iy, SignModulo(iz, CELLSIZE));
		}
		else
			return (*this)(ix, iy, iz < 0 ? 0 : CELLSIZE - 1);
	}
	return
		0 <= ix && ix < CELLSIZE &&
		0 <= iy && iy < CELLSIZE &&
		0 <= iz && iz < CELLSIZE 
		? v[ix][iy][iz] : v0;
}

inline bool CellVolume::setCell(int ix, int iy, int iz, const Cell &newCell){
	if (ix < 0 || CELLSIZE <= ix || iy < 0 || CELLSIZE <= iy || iz < 0 || CELLSIZE <= iz)
		return false;
	else
	{
		// Update solidcount by difference of solidity before and after cell assignment.
		int before = v[ix][iy][iz].isSolid();
		int after = newCell.isSolid();
		_solidcount += after - before;

		v[ix][iy][iz] = newCell;
		updateCache();
		if(ix <= 0)
			world->volume[Vec3i(index[0] - 1, index[1], index[2])].updateCache();
		else if(CELLSIZE - 1 <= ix)
			world->volume[Vec3i(index[0] + 1, index[1], index[2])].updateCache();
		if(iy <= 0)
			world->volume[Vec3i(index[0], index[1] - 1, index[2])].updateCache();
		else if (CELLSIZE - 1 <= iy)
			world->volume[Vec3i(index[0], index[1] + 1, index[2])].updateCache();
		if(iz <= 0)
			world->volume[Vec3i(index[0], index[1], index[2] - 1)].updateCache();
		else if (CELLSIZE - 1 <= iz)
			world->volume[Vec3i(index[0], index[1], index[2] + 1)].updateCache();
		return true;
	}
}




/// <summary>The constant object that will returned when index is out of range.</summary>
/// <remarks>What a mess.</remarks>
const Cell CellVolume::v0(Cell::Air);


int CellVolume::cellInvokes = 0;
int CellVolume::cellForeignInvokes = 0;
int CellVolume::cellForeignExists = 0;

/// Simplex Fractal Noise in 3D
static double sfnoise3(const Vec3d &basepos, int octaves, double persistence){
	assert(0 < octaves);
	double ret = 0.;
	double f = 1.;
	double fsum = 0.;
	for(int i = 0; i < octaves; i++){
		double s = (1 << i);
		f *= persistence;
		ret += f * snoise3d(basepos[0] * s, basepos[1] * s, basepos[2] * s);
		fsum += f;
	}
	return ret / fsum;
}

/// <summary>
/// Initialize this CellVolume with Perlin Noise with given position index.
/// </summary>
/// <param name="ci">The position of new CellVolume</param>
void CellVolume::initialize(const Vec3i &ci){

	double noiseHeight = world->getNoiseHeight();
	double baseHeight = world->getBaseHeight();
	double maxHeight = baseHeight + noiseHeight * 2.; // Theoretical limit height
	int maxCellCount = int(maxHeight / world->getCellWidth());

	_solidcount = 0;
	for(int ix = 0; ix < CELLSIZE; ix++){
		for(int iz = 0; iz < CELLSIZE; iz++){
			for(int iy = 0; iy < CELLSIZE; iy++){

				// Obtain spatial position of a given cell.
				Vec3d pos = (Vec3d(ix, iy, iz) + ci.cast<double>() * CELLSIZE) * world->getCellWidth();
				double height = pos.len();
				Vec3d basepos = height ? pos / height : pos;
				double s = 1.;
				double noiseVal = sfnoise3(basepos * s, 3, 0.5);

				if(noiseVal * noiseHeight + baseHeight < height)
					v[ix][iy][iz] = Cell(Cell::Air);
				else{
					Cell::Type ct = sfnoise3(pos / world->getCellWidth() / CELLSIZE, 3, 0.5) < 0.25 ? Cell::Rock : Cell::Iron;
					world->bricks[ct]++;
					v[ix][iy][iz] = Cell(ct);
					_solidcount++;
				}
			}
		}
	}
	for(int ix = 0; ix < CELLSIZE; ix++) for(int iy = 0; iy < CELLSIZE; iy++) for(int iz = 0; iz < CELLSIZE; iz++){
		updateAdj(ix, iy, iz);
	}

}

void CellVolume::updateAdj(int ix, int iy, int iz){
	v[ix][iy][iz].adjacents =
		(!cell(ix - 1, iy, iz).isTranslucent() ? 1 : 0) +
        (!cell(ix + 1, iy, iz).isTranslucent() ? 1 : 0) +
        (!cell(ix, iy - 1, iz).isTranslucent() ? 1 : 0) +
        (!cell(ix, iy + 1, iz).isTranslucent() ? 1 : 0) +
        (!cell(ix, iy, iz - 1).isTranslucent() ? 1 : 0) +
        (!cell(ix, iy, iz + 1).isTranslucent() ? 1 : 0);
}

void CellVolume::updateCache()
{
	for (int ix = 0; ix < CELLSIZE; ix++) for (int iy = 0; iy < CELLSIZE; iy++) for (int iz = 0; iz < CELLSIZE; iz++)
		updateAdj(ix, iy, iz);
    
	// Build up scanline map
	for (int ix = 0; ix < CELLSIZE; ix++) for (int iz = 0; iz < CELLSIZE; iz++)
	{
		// Find start and end points for this scan line
		bool begun = false;
		bool transBegun = false;
		for (int iy = 0; iy < CELLSIZE; iy++)
		{
			Cell &c = v[ix][iy][iz];
			// If this cell is not air but translucent (i.e. partially visible),
			// we must draw it however their surronding voxels are.
			if (c.type != Cell::Air && (c.isTranslucent() || c.adjacents != 0 && c.adjacents != 6))
			{
				if (!begun)
				{
					begun = true;
					_scanLines[ix][iz][0] = iy;
				}
				_scanLines[ix][iz][1] = iy + 1;
			}
		}
	}

	vboDirty = true;
}


VoxelEntity::VoxelEntity(WarField *w) : st(w), volume(operator<){
}

static const int maxViewDistance = 10.;

void VoxelEntity::anim(double dt){
}

void VoxelEntity::draw(WarDraw *wd){

	const Vec3i inf = VoxelEntity::real2ind(wd->vw->pos - this->pos);
	std::vector<CellVolume*> changed;
	double noiseHeight = getNoiseHeight();
	double baseHeight = getBaseHeight();
	double maxHeight = baseHeight + noiseHeight * 2.; // Theoretical limit height
	int maxCellCount = int(maxHeight / getCellWidth() / CELLSIZE) + 1;
	int radius = maxViewDistance / getCellWidth() / CELLSIZE;
	for (int ix = -maxCellCount; ix <= maxCellCount; ix++){
		for (int iy = -maxCellCount; iy <= maxCellCount; iy++){
			for (int iz = -maxCellCount; iz <= maxCellCount; iz++){
				Vec3i ci(ix, iy, iz);
				if(volume.find(ci) == volume.end()){
					volume[ci] = CellVolume(this, ci);
					CellVolume &cv = volume[ci];
					cv.initialize(ci);
					changed.push_back(&volume[ci]);
				}
			}
		}
	}

	for(std::vector<CellVolume*>::iterator it = changed.begin(); it != changed.end(); it++)
		(*it)->updateCache();

	vboInitBuffers();

	{
		GLmatrix glm;
		GLattrib gla(GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
		Mat4d mat;
		transform(mat);
		glMultMatrixd(mat);

		// Set some ambient illumination for showing features in the shadow.
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Vec4<float>(.25,.25,.25,1.));
		// Reset specular and shininess parameters to prevent strange shading in GeForce boards with shaders.
		glMaterialfv(GL_FRONT, GL_SPECULAR, Vec4f(0,0,0,1));
		glMaterialfv(GL_FRONT, GL_SHININESS, Vec3f(50,0,0));

		const ShaderBind *sb = NULL;
		if(wd->shadowMap){
			sb = wd->shadowMap->getShader();
			if(sb){
				sb->enableTextures(true, false);
			}
		}

		Cell::Type celltype = Cell::Air;

		const double maxViewCells = maxViewDistance / getCellWidth();

		for(VoxelEntity::VolumeMap::iterator it = this->volume.begin(); it != this->volume.end(); it++){
			const Vec3i &key = it->first;
			CellVolume &cv = it->second;

			initTexLists();

			// If all content is air, skip drawing
			if(cv.getSolidCount() == 0)
				continue;

			// Examine if intersects or included in viewing frustum
//			if(!FrustumCheck(xmvc(VoxelEntity::ind2real(key * CELLSIZE).cast<float>()), xmvc(World::ind2real((key + Vec3i(1,1,1)) * CELLSIZE).cast<float>()), frustum))
//				continue;

			// Cull too far CellVolumes
			if ((key[0] + 1) * CELLSIZE + maxViewCells < inf[0])
				continue;
			if (inf[0] < key[0] * CELLSIZE - maxViewCells)
				continue;
			if ((key[1] + 1) * CELLSIZE + maxViewCells < inf[1])
				continue;
			if (inf[1] < key[1] * CELLSIZE - maxViewCells)
				continue;
			if ((key[2] + 1) * CELLSIZE + maxViewCells < inf[2])
				continue;
			if (inf[2] < key[2] * CELLSIZE - maxViewCells)
				continue;

			if(cv.vboDirty){
				std::vector<VERTEX> vlist[cv.Num_Mat];
				std::vector<GLuint> vidx[cv.Num_Mat];
				cv.modeledCells.clear();

				for(int ix = 0; ix < CELLSIZE; ix++){
					for(int iz = 0; iz < CELLSIZE; iz++){
						// This detail culling is not much effective.
						//if (bf.Contains(new BoundingBox(ind2real(keyindex + new Vec3i(ix, kv.Value.scanLines[ix, iz, 0], iz)), ind2real(keyindex + new Vec3i(ix + 1, kv.Value.scanLines[ix, iz, 1] + 1, iz + 1)))) != ContainmentType.Disjoint)
						const int (&scanLines)[CELLSIZE][CELLSIZE][2] = cv.getScanLines();
						for (int iy = 0; iy < CELLSIZE; iy++){

							// Cull too far Cells
/*							if (cv(ix, iy, iz).getType() == Cell::Air)
								continue;
							if (maxViewCells < abs(ix + it->first[0] * CELLSIZE - inf[0]))
								continue;
							if (maxViewCells < abs(iy + it->first[1] * CELLSIZE - inf[1]))
								continue;
							if (maxViewCells < abs(iz + it->first[2] * CELLSIZE - inf[2]))
								continue;*/

							// If the Cell is buried under ground, it's no use examining each face of the Cell.
//							if(6 <= cv(ix, iy, iz).getAdjacents())
//								continue;

							const Cell &cell = cv(ix, iy, iz);
							Vec3i posInVolume(ix, iy, iz);

							drawCell(cell, posInVolume + it->first * CELLSIZE, celltype, &cv, posInVolume, vlist, vidx);
						}
					}
				}

				for(int i = 0; i < cv.Num_Mat; i++){
					if(0 < vidx[i].size()){
						if(cv.vbo[i] == 0)
							glGenBuffers(1, &cv.vbo[i]);
						if(cv.vboIdx[i] == 0)
							glGenBuffers(1, &cv.vboIdx[i]);

						glBindBuffer(GL_ARRAY_BUFFER, cv.vbo[i]);
						glBufferData(GL_ARRAY_BUFFER, vlist[i].size() * sizeof(vlist[i][0]), &vlist[i].front(), GL_STATIC_DRAW);
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cv.vboIdx[i]);
						glBufferData(GL_ELEMENT_ARRAY_BUFFER, vidx[i].size() * sizeof(vidx[i][0]), &vidx[i].front(), GL_STATIC_DRAW);
					}

					// We can forget about the actual buffer content, but must remember the number of elements
					// in the buffer in order to perform glDrawElements() afterwards.
					cv.vlistCounts[i] = vlist[i].size();
					cv.vidxCounts[i] = vidx[i].size();
				}

				cv.vboDirty = false;
			}

			for(int i = 0; i < cv.Num_Mat; i++){
				if(cv.vboIdx[i]){
					GLmatrix glm;

					switch(i){
					case CellVolume::MatRock: glCallList(texlist_rock); break;
					case CellVolume::MatIron: glCallList(texlist_iron); break;
					case CellVolume::MatArmor: glCallList(texlist_armor); break;
					}

					glScaled(getCellWidth(), getCellWidth(), getCellWidth());

					glEnableClientState(GL_VERTEX_ARRAY);
					glEnableClientState(GL_NORMAL_ARRAY);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);

					glPushAttrib(GL_POLYGON_BIT);
					glFrontFace(GL_CW);
//					glDisable(GL_CULL_FACE);

					glBindBuffer(GL_ARRAY_BUFFER, cv.vbo[i]);
					glVertexPointer(3, GL_DOUBLE, sizeof(VERTEX), (void*)offsetof(VERTEX, pos)); // Vertex array
					glNormalPointer(GL_DOUBLE, sizeof(VERTEX), (void*)offsetof(VERTEX, norm)); // Normal array
					glTexCoordPointer(2, GL_DOUBLE, sizeof(VERTEX), (void*)offsetof(VERTEX, tex)); // Texture coordinates array

					// Index array
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cv.vboIdx[i]);

					glDrawElements(GL_TRIANGLES, cv.vidxCounts[i], GL_UNSIGNED_INT, 0);

					glPopAttrib();

					glDisableClientState(GL_VERTEX_ARRAY);
					glDisableClientState(GL_NORMAL_ARRAY);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
			}

			for(auto ipos : cv.modeledCells)
				drawCell(cv.cell(ipos), ipos + it->first * CELLSIZE, celltype, &cv, ipos);
		}
	}
}

void VoxelEntity::drawtra(WarDraw *wd){
	if(previewCell.getType() != Cell::Air){
		GLmatrix glm;
		GLattrib gla(GL_TEXTURE_BIT | GL_CURRENT_BIT);
		Mat4d mat;
		transform(mat);
		glMultMatrixd(mat);

		glColor4f(1,1,1,0.5f);

		glEnable(GL_CULL_FACE);

		initTexLists();

		const ShaderBind *sb = NULL;
		if(wd->shadowMap){
			sb = wd->shadowMap->getShader();
			if(sb){
				sb->enableTextures(true, false);
			}
		}

		Cell::Type celltype = Cell::Air;

		drawCell(previewCell, previewCellPos, celltype);
	}
}

/// \brief An internal function to draw a single cell in VoxelEntity
/// \param cell A cell to draw
/// \param pos Position vector (integral) in voxel space
/// \param celltype A rendering state variable to minimize number of texture switching
/// \param cv A CellVolume object for adjacent cell checking
/// \param posInVolume The position vector in the CellVolume object
/// \param vlist A pointer to an array with size 3 of vertex buffer for list of vertices, can be NULL for direct drawing
/// \param vidx A pointer to an array with size 3 of index buffer for list of primitives
void VoxelEntity::drawCell(const Cell &cell, const Vec3i &pos, Cell::Type &celltype, const CellVolume *cv, const Vec3i &posInVolume,
						   std::vector<VERTEX> *vlist, std::vector<GLuint> *vidx)const
{
	static const Vec3d slopeNormal = Vec3d(1,1,0).norm();
	static const Vec3d cornerNormal = Vec3d(1,1,1).norm();

	// Create vertex buffer
	static const VERTEX vertices[] =
	{
		{Vec3d(0, 0, 0), Vec3d(0, -1, 0), Vec2d(0, 0)},
		{Vec3d(0, 0, 1), Vec3d(0, -1, 0), Vec2d(0, 1)},
		{Vec3d(1, 0, 1), Vec3d(0, -1, 0), Vec2d(1, 1)},
		{Vec3d(1, 0, 0), Vec3d(0, -1, 0), Vec2d(1, 0)},

		{Vec3d(0, 1, 0), Vec3d(0, 1, 0), Vec2d(0, 0)},
		{Vec3d(1, 1, 0), Vec3d(0, 1, 0), Vec2d(1, 0)},
		{Vec3d(1, 1, 1), Vec3d(0, 1, 0), Vec2d(1, 1)},
		{Vec3d(0, 1, 1), Vec3d(0, 1, 0), Vec2d(0, 1)},

		{Vec3d(0, 0, 0), Vec3d(0, 0, -1), Vec2d(0, 0)},
		{Vec3d(1, 0, 0), Vec3d(0, 0, -1), Vec2d(1, 0)},
		{Vec3d(1, 1, 0), Vec3d(0, 0, -1), Vec2d(1, 1)},
		{Vec3d(0, 1, 0), Vec3d(0, 0, -1), Vec2d(0, 1)},

		{Vec3d(0, 0, 1), Vec3d(0, 0, 1), Vec2d(0, 0)},
		{Vec3d(0, 1, 1), Vec3d(0, 0, 1), Vec2d(0, 1)},
		{Vec3d(1, 1, 1), Vec3d(0, 0, 1), Vec2d(1, 1)},
		{Vec3d(1, 0, 1), Vec3d(0, 0, 1), Vec2d(1, 0)},

		{Vec3d(0, 0, 0), Vec3d(-1, 0, 0), Vec2d(0, 0)},
		{Vec3d(0, 1, 0), Vec3d(-1, 0, 0), Vec2d(1, 0)},
		{Vec3d(0, 1, 1), Vec3d(-1, 0, 0), Vec2d(1, 1)},
		{Vec3d(0, 0, 1), Vec3d(-1, 0, 0), Vec2d(0, 1)},

		{Vec3d(1, 0, 0), Vec3d(1, 0, 0), Vec2d(0, 0)},
		{Vec3d(1, 0, 1), Vec3d(1, 0, 0), Vec2d(0, 1)},
		{Vec3d(1, 1, 1), Vec3d(1, 0, 0), Vec2d(1, 1)},
		{Vec3d(1, 1, 0), Vec3d(1, 0, 0), Vec2d(1, 0)},

		{Vec3d(1, 0, 0), slopeNormal, Vec2d(0, 0)},
		{Vec3d(0, 1, 0), slopeNormal, Vec2d(0, 1)},
		{Vec3d(0, 1, 1), slopeNormal, Vec2d(1, 1)},
		{Vec3d(1, 0, 1), slopeNormal, Vec2d(1, 0)},

		{Vec3d(1, 0, 0), cornerNormal, Vec2d(0, 0)},
		{Vec3d(0, 1, 0), cornerNormal, Vec2d(0, 1)},
		{Vec3d(0, 0, 1), cornerNormal, Vec2d(1, 1)},

		{Vec3d(1, 1, 0), cornerNormal, Vec2d(0, 0)},
		{Vec3d(0, 1, 1), cornerNormal, Vec2d(0, 1)},
		{Vec3d(1, 0, 1), cornerNormal, Vec2d(1, 1)},
	};

	static const unsigned indices[] =
	{
		2,1,0,
		0,3,2,

		6,5,4,
		4,7,6,

		10,9,8,
		8,11,10,

		14,13,12,
		12,15,14,

		18,17,16,
		16,19,18,

		22,21,20,
		20,23,22
	};

	static const unsigned slopeIndices[] = {
		2,1,0,
		0,3,2,

		18,17,16,
		16,19,18,

		11,9,8,

		15,13,12,

		24,25,26,
		26,27,24
	};

	static const unsigned cornerIndices[] = {
		0,3,1,

		16,19,17,

		8,11,9,

		28,29,30,
	};

	static const unsigned invCornerIndices[] = {
		2,1,0,
		0,3,2,

		10,9,8,
		8,11,10,

		18,17,16,
		16,19,18,

		20,23,21,

		4,7,5,

		12,15,13,

		31,32,33,
	};

	int material = 0;
	if(!vlist){
		if(celltype != cell.getType()){
			celltype = cell.getType();
			switch(celltype){
			case Cell::Rock: glCallList(texlist_rock); break;
			case Cell::Iron: glCallList(texlist_iron); break;
			case Cell::Armor:
			case Cell::ArmorSlope:
			case Cell::ArmorCorner:
			case Cell::ArmorInvCorner: glCallList(texlist_armor); break;
			case Cell::Occupied: return; // The occupied cell is not rendered because another cell should draw it.
			}
		}
	}
	else{
		celltype = cell.getType();
		switch(celltype){
		case Cell::Rock: material = CellVolume::MatRock; break;
		case Cell::Iron: material = CellVolume::MatIron; break;
		case Cell::Armor:
		case Cell::ArmorSlope:
		case Cell::ArmorCorner:
		case Cell::ArmorInvCorner: material = CellVolume::MatArmor; break;
		case Cell::Occupied: return; // The occupied cell is not rendered because another cell should draw it.
		}
	}

	Mat4d rotmat;

	auto drawIndexedGeneral = [cv, vlist, vidx, &pos, material, &rotmat](int cnt, int base, const unsigned indices[]){
		for(int i = base; i < base + cnt; i++){
			if(vlist){
				std::vector<VERTEX> &vl = vlist[material];
				VERTEX vtx = vertices[indices[i]];
				vtx.pos = rotmat.vp3(vtx.pos) + pos.cast<double>();
				vtx.norm = rotmat.dvp3(vtx.norm);
				bool match = false;
				for(int j = MAX(0, vl.size() - 16); j < vl.size(); j++){
					if(vl[j] == vtx){
						vidx[material].push_back(j);
						match = true;
					}
				}
				if(!match){
					vidx[material].push_back(vl.size());
					vl.push_back(vtx);
				}
			}
			else{
				glNormal3dv(vertices[indices[i]].norm);
				glTexCoord2dv(vertices[indices[i]].tex);
				glVertex3dv(vertices[indices[i]].pos);
			}
		}
	};

	auto drawIndexed = [&](int cnt, int base){
		drawIndexedGeneral(cnt, base, indices);
	};

	auto drawIndexedSlope = [&](int cnt, int base){
		drawIndexedGeneral(cnt, base, slopeIndices);
	};

	GLmatrix glm;
	Vec3d ofs(pos.cast<double>());
	if(!vlist){
		glScaled(getCellWidth(), getCellWidth(), getCellWidth());
		glTranslated(ofs[0], ofs[1], ofs[2]);

		if(cell.getRotation()){
			glTranslated(0.5, 0.5, 0.5);
			if(char c = (cell.getRotation() & 0x3))
				glRotatef(90 * c, 1, 0, 0);
			if(char c = ((cell.getRotation() >> 2) & 0x3))
				glRotatef(90 * c, 0, 1, 0);
			if(char c = ((cell.getRotation() >> 4) & 0x3))
				glRotatef(90 * c, 0, 0, 1);
			glTranslated(-0.5, -0.5, -0.5);
		}

		if(cell.getType() == Cell::Engine){
			static Model *model = LoadMQOModel("models/block_engine.mqo");
			static const double modelScale = 0.01;
			if(model){
				glTranslated(0.5, 0.5, 0.5);
				glScaled(modelScale, modelScale, modelScale);
				DrawMQOPose(model, NULL);
			}
			return;
		}
	}
	else{
		if(cell.getRotation()){
			Mat4d mid = mat4_u.translate(0.5, 0.5, 0.5);
			if(char c = (cell.getRotation() & 0x3))
				mid = mid.rotx(M_PI / 2. * c);
			if(char c = ((cell.getRotation() >> 2) & 0x3))
				mid = mid.roty(M_PI / 2. * c);
			if(char c = ((cell.getRotation() >> 4) & 0x3))
				mid = mid.rotz(M_PI / 2. * c);
			rotmat = mid.translate(-0.5, -0.5, -0.5);
		}
		else
			rotmat = mat4_u;

		if(cell.getType() == Cell::Engine){
			cv->modeledCells.push_back(posInVolume);
			return;
		}
	}

	bool x0 = false;
	bool x1 = false;
	bool y0 = false;
	bool y1 = false;
	bool z0 = false;
	bool z1 = false;
	if(cv){
		int ix = posInVolume[0];
		int iy = posInVolume[1];
		int iz = posInVolume[2];
		x0 = !(*cv)(ix - 1, iy, iz).isTranslucent();
		x1 = !(*cv)(ix + 1, iy, iz).isTranslucent();
		y0 = !(*cv)(ix, iy - 1, iz).isTranslucent();
		y1 = !(*cv)(ix, iy + 1, iz).isTranslucent();
		z0 = !(*cv)(ix, iy, iz - 1).isTranslucent();
		z1 = !(*cv)(ix, iy, iz + 1).isTranslucent();
	}

	if(!vlist)
		glBegin(GL_TRIANGLES);
	if(cell.getType() == Cell::ArmorSlope){
		drawIndexedSlope(numof(slopeIndices), 0);
	}
	else if(cell.getType() == Cell::ArmorCorner){
		drawIndexedGeneral(numof(cornerIndices), 0, cornerIndices);
	}
	else if(cell.getType() == Cell::ArmorInvCorner){
		drawIndexedGeneral(numof(invCornerIndices), 0, invCornerIndices);
	}
	else if(vlist){
		using namespace MarchingCube;
		GRIDCELL gcell;
		for(int k = 0; k < 8; k++){
			gcell.p[k] = vertexOffsets[k].cast<double>();
			gcell.val[k] = (*cv).cell(posInVolume + vertexOffsets[k]).isTranslucent();
		}
		TRIANGLE tris[5];
		int ntris = Polygonise(gcell, 0.5, tris);
		for(int i = 0; i < ntris; i++) for(int j = 0; j < 3; j++){
			std::vector<VERTEX> &vl = vlist[material];
			VERTEX vtx;
			vtx.pos = tris[i].p[j] + pos.cast<double>();
			vtx.norm = Vec3d(0,1,0);
			vtx.tex = Vec2d(tris[i].p[j][0], tris[i].p[j][1]);
			bool match = false;
			for(int k = std::max(0, (int)vl.size() - 16); k < vl.size(); k++){
				if(vl[k] == vtx){
					vidx[material].push_back(k);
					match = true;
				}
			}
			if(!match){
				vidx[material].push_back(vl.size());
				vl.push_back(vtx);
			}
		}
	}
	else{
		if(!x0 && !x1 && !y0 && !y1){
			drawIndexed(36, 0);
		}
		else{
			if(!x0)
				drawIndexed(2 * 3, 8 * 3);
			if(!x1)
				drawIndexed(2 * 3, 10 * 3);
			if(!y0)
				drawIndexed(2 * 3, 0);
			if(!y1)
				drawIndexed(2 * 3, 2 * 3);
			if(!z0)
				drawIndexed(2 * 3, 4 * 3);
			if(!z1)
				drawIndexed(2 * 3, 6 * 3);
		}
	}
	if(!vlist)
		glEnd();

}

void VoxelEntity::initTexLists(){
	if(!texlist_init){
		texlist_init = true;
		texlist_rock = CallCacheBitmap("rock.jpg", "textures/rock.jpg", NULL, NULL);
		texlist_iron = CallCacheBitmap("iron.jpg", "textures/iron.jpg", NULL, NULL);
		texlist_armor = CallCacheBitmap("armor.png", "textures/armor.png", NULL, NULL);
	}
}

bool VoxelEntity::command(EntityCommand *com){
	if(ModifyVoxelCommand *mvc = InterpretCommand<ModifyVoxelCommand>(com)){
		static const double boundHeight = 0.001;
		Vec3d src = mvc->src - this->pos;
		Vec3d dir = rot.itrans(mvc->dir);
		if(mvc->mode == mvc->Put || mvc->mode == mvc->Preview){

			if(mvc->mode == mvc->Preview){
				// Reset preview cell before proceeding
				previewCell = Cell::Air;
			}

			int length = mvc->ct == Cell::Engine ? 2 : 1;
			for (int i = 1; i < 8; i++){
				Vec3i baseIdx = real2ind(src + dir * i * getCellWidth() / 2);

				bool supported = false;
				for(int iz = 0; iz < length; iz++){
					Cell::Type ct = iz == 0 ? mvc->ct : Cell::Occupied;
					Vec3i ci = baseIdx + Cell::rotate(mvc->rotation, Vec3i(0, 0, iz));

					if(isSolid(ci[0], ci[1], ci[2])){
						supported = false;
						break;
					}

					// If this placing makes the Player to be stuck in a brick, do not allow it.
					bool buried = false;
					for(int ix = 0; ix < 2 && !buried; ix++){
						for(int iy = 0; iy < 2 && !buried; iy++){
							for(int iz = 0; iz < 2 && !buried; iz++){
								// Position to check collision with the walls
								Vec3d hitcheck(
									src[0] + (ix * 2 - 1) * boundHeight,
									src[1] + (iy * 2 - 1) * boundHeight,
									src[2] + (iz * 2 - 1) * boundHeight);

								if(ci == real2ind(hitcheck))
									buried = true;
							}
						}
					}
					if(buried){
						supported = false;
						break;
					}

					static const Vec3i directions[] = {
						Vec3i(1,0,0),
						Vec3i(-1,0,0),
						Vec3i(0,1,0),
						Vec3i(0,-1,0),
						Vec3i(0,0,1),
						Vec3i(0,0,-1),
					};

					for(int j = 0; j < numof(directions); j++){
						Vec3i dirAfterRot = Cell::rotate(mvc->rotation, directions[j]);
						if(!isSolid(ci + directions[j]))
							continue;
						if(!Cell(ct, mvc->rotation).connection(directions[j]))
							continue;
						if(cell(ci + directions[j]).connection(-directions[j])){
							supported = true;
							break;
						}
					}
					if(supported)
						break;
				}
				if(!supported)
					continue;

				if(mvc->mode == mvc->Put){
					bool notdone = false;
					for(int iz = 0; iz < length; iz++){
						Cell::Type ct = iz == 0 ? mvc->ct : Cell::Occupied;
						Vec3i ci = baseIdx + Cell::rotate(mvc->rotation, Vec3i(0, 0, iz));
						if(setCell(ci[0], ci[1], ci[2], Cell(ct, mvc->rotation))){
		//					player->addBricks(curtype, -1);
						}
						else{
							notdone = true;
							break;
						}
					}
					if(!notdone)
						break;
				}
				else{ // Preview
					previewCellPos = baseIdx;
					previewCell = Cell(mvc->ct, mvc->rotation);
					break;
				}
			}
		}
		else if(mvc->mode == mvc->Remove){
			// Dig the cell forward
			for(int i = 1; i < 8; i++){
				Vec3i ci = real2ind(src + dir * i * getCellWidth() / 2);
				Cell c = cell(ci[0], ci[1], ci[2]);
				if (c.isSolid() && setCell(ci[0], ci[1], ci[2], Cell::Air))
				{
//					player->addBricks(c.getType(), 1);
					break;
				}
			}
		}

		return false;
	}
	else
		return st::command(com);
}

/// <summary>
/// Convert from real world coords to massvolume index vector
/// </summary>
/// <param name="pos">world vector</param>
/// <returns>indices</returns>
Vec3i VoxelEntity::real2ind(const Vec3d &pos)const{
	Vec3d tpos = pos / getCellWidth();
	Vec3i vi((int)floor(tpos[0]), (int)floor(tpos[1]), (int)floor(tpos[2]));
	return vi;
}

/// <summary>
/// Convert from massvolume index to real world coords
/// </summary>
/// <param name="ipos">indices</param>
/// <returns>world vector</returns>
Vec3d VoxelEntity::ind2real(const Vec3i &ipos)const{
	Vec3i tpos = (ipos) * getCellWidth();
	return tpos.cast<double>();
}

/// <summary>Solidity check for given index coordinates</summary>
bool VoxelEntity::isSolid(const Vec3i &v){
	Vec3i ci(SignDiv(v[0], CELLSIZE), SignDiv(v[1], CELLSIZE), SignDiv(v[2], CELLSIZE));
	if(volume.find(ci) != volume.end()){
		CellVolume &cv = volume[ci];
		return cv.isSolid(Vec3i(SignModulo(v[0], CELLSIZE), SignModulo(v[1], CELLSIZE), SignModulo(v[2], CELLSIZE)));
	}
	else
		return false;
}

/// <summary>Solidity check for given world coordinates</summary>
bool VoxelEntity::isSolid(const Vec3d &rv){
	Vec3i v = real2ind(rv);
	Vec3i ci(SignDiv(v[0], CELLSIZE), SignDiv(v[1], CELLSIZE), SignDiv(v[2], CELLSIZE));
	if(volume.find(ci) != volume.end()){
		CellVolume &cv = volume[ci];
		const Cell &c = cv(SignModulo(v[0], CELLSIZE), SignModulo(v[1], CELLSIZE), SignModulo(v[2], CELLSIZE));
		return c.isSolid();
	}
	else
		return false;
}

IMPLEMENT_COMMAND(ModifyVoxelCommand, "ModifyVoxel")

void ModifyVoxelCommand::serialize(SerializeContext &sc){
	sc.o << src;
	sc.o << dir;
	sc.o << mode;
	sc.o << int(ct);
}

void ModifyVoxelCommand::unserialize(UnserializeContext &sc){
	sc.i >> src;
	sc.i >> dir;
	sc.i >> (int&)mode;
	sc.i >> (int&)ct;
}

ModifyVoxelCommand::ModifyVoxelCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 2)
		throw SQFArgumentError();
	SQVec3d qvsrc;
	qvsrc.getValue(v, 3);
	src = qvsrc.value;
	SQVec3d qvdir;
	qvdir.getValue(v, 4);
	dir = qvdir.value;

	const SQChar *sqstr;
	if(SQ_FAILED(sq_getstring(v, 5, &sqstr)))
		throw SQFError(_SC("4th argument must be string"));
	if(!scstrcmp(sqstr, _SC("Put")))
		mode = Put;
	else if(!scstrcmp(sqstr, _SC("Remove")))
		mode = Remove;
	else if(!scstrcmp(sqstr, _SC("Preview")))
		mode = Preview;

	ct = Cell::Rock;
	SQInteger sqct;
	if(SQ_SUCCEEDED(sq_getinteger(v, 6, &sqct)))
		ct = Cell::Type(sqct);

	rotation = 0;
	SQInteger sqrotation;
	if(SQ_SUCCEEDED(sq_getinteger(v, 7, &sqrotation)))
		rotation = sqrotation;
}