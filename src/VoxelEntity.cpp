/// \file
/// \brief Definition and implementation of VoxelEntity class.
#include "Entity.h"
#include "EntityRegister.h"
#include "perlinNoise3d.h"

const int CELLSIZE = 16;

class Game;
class VoxelEntity;

/// <summary>The atomic unit of the world.</summary>
class Cell{
public:
	enum Type{
		Air, ///< Null cell
		Grass, ///< Grass cell
		Dirt, ///< Dirt cell
		Gravel, ///< Gravel cell
		Rock, ///< Rock cell
		Water, ///< Water cell
		HalfBit = 0x8, ///< Bit indicating half height of base material.
		HalfAir = HalfBit | Air, ///< This does not really exist, just a theoretical entity.
		HalfGrass = HalfBit | Grass, ///< Half-height Grass.
		HalfDirt = HalfBit | Dirt,
		HalfGravel = HalfBit | Gravel,
		HalfRock = HalfBit | Rock,
		NumTypes
	};

	Cell(Type t = Air) : type(t), adjacents(0), adjacentWater(0){}
	Type getType()const{return type;}
	short getValue()const{return value;}
	void setValue(short avalue){value = avalue;}
	int getAdjacents()const{return adjacents;}
	int getAdjacentWaterCells()const{return adjacentWater;}
	bool isSolid()const{return type != Air && type != Water;}
	bool isTranslucent()const{return type == Air || type == Water || type & HalfBit;}
	void serialize(std::ostream &o);
	void unserialize(std::istream &i);
protected:
	enum Type type;
	short value;
	char adjacents;
	char adjacentWater;

	friend class CellVolume;
};

class CellVolume{
public:
	static const Cell v0;

protected:
	VoxelEntity *world;
	Vec3i index;
	Cell v[CELLSIZE][CELLSIZE][CELLSIZE];

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
	CellVolume(VoxelEntity *world = NULL, const Vec3i &ind = Vec3i(0,0,0)) : world(world), index(ind), _solidcount(0){
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

class EXPORT VoxelEntity : public Entity{
public:
	typedef Entity st;

	VoxelEntity(Game *game) : st(game){}
	VoxelEntity(WarField *w);

	static EntityRegister<VoxelEntity> entityRegister;

//	virtual void serialize(SerializeContext &sc);
//	virtual void unserialize(UnserializeContext &sc);
//	virtual void addRigidBody(WarSpace*);
//	virtual void anim(double dt);
//	virtual void drawtra(wardraw_t *);
//	virtual void drawHUD(wardraw_t *);
//	virtual void control(const input_t *, double);
//	unsigned analog_mask();
//	virtual Props props()const;
//	struct ManeuverParams;
//	virtual const ManeuverParams &getManeuve()const;
	virtual bool isTargettable()const override{return false;}
//	virtual bool isSelectable()const;
//	virtual bool command(EntityCommand *com);
//	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);

//	virtual short getDefaultCollisionMask()const;

	double getHitRadius()const override{return 0.5;}

	typedef std::map<Vec3i, CellVolume, bool(*)(const Vec3i &, const Vec3i &)> VolumeMap;

private:
	VolumeMap volume;
	int bricks[Cell::NumTypes];
	friend class CellVolume;
};


Entity::EntityRegister<VoxelEntity> VoxelEntity::entityRegister("VoxelEntity");


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


/// <summary>
/// Initialize this CellVolume with Perlin Noise with given position index.
/// </summary>
/// <param name="ci">The position of new CellVolume</param>
void CellVolume::initialize(const Vec3i &ci){
	float field[CELLSIZE][CELLSIZE];

	PerlinNoise::PerlinNoiseParams3D pnp(12321, 0.5);
	pnp.octaves = 8;
	pnp.xofs = ci[0] * CELLSIZE;
	pnp.yofs = ci[2] * CELLSIZE;
	pnp.zofs = ci[1] * CELLSIZE;
	PerlinNoise::perlin_noise<CELLSIZE>(pnp, PerlinNoise::FieldAssign<CELLSIZE>(field));

	pnp.octaves = 7;
	pnp.yofs = ci[1] * CELLSIZE;
	pnp.zofs = ci[2] * CELLSIZE;
	float cellFactorTable[4][CELLSIZE][CELLSIZE][CELLSIZE];
	const unsigned long seeds[4] = {54123, 112398, 93532, 3417453};
	for(int i = 0; i < 4; i++){
		pnp.seed = seeds[i];
		PerlinNoise::perlin_noise_3D<CELLSIZE>(pnp, PerlinNoise::FieldAssign3D<CELLSIZE>(cellFactorTable[i]));
	}

#if 0
	int values[CELLSIZE][CELLSIZE][CELLSIZE] = {0};
	for(int iz = 0; iz < CELLSIZE; iz++){
		int fz = iz + ci[2] * CELLSIZE + 115;
		if(fz < 0)
			continue;
		if(230 <= fz)
			break;
		char cbuf[256];
		sprintf(cbuf, "s2\\Co-2-%d.txt", iz);
		FILE *fp = fopen(cbuf, "r");
		for(int fy = 0; fy < 2048; fy++){
			int iy = fy - ci[1] * CELLSIZE - 385;
			for(int fx = 0; fx < 224; fx++){
				int ix = fx - ci[0] * CELLSIZE - 112;
				int val;
				fscanf(fp, "%d\t", &val);
				if(0 <= iy && iy < CELLSIZE && 0 <= ix && ix < CELLSIZE)
					values[iz][iy][ix] = val;
			}
		}
		fclose(fp);
	}
#endif

	_solidcount = 0;
	for(int ix = 0; ix < CELLSIZE; ix++) for(int iz = 0; iz < CELLSIZE; iz++){
		// Compute the height first. The height is distance from the surface just below the cell of interest,
		// can be negative when it's below surface.
		int baseHeight = ci[1] * CELLSIZE - ((int)floor(field[ix][iz] * CELLSIZE * 4) - 16);

		for(int iy = 0; iy < CELLSIZE; iy++){
			int height = iy + baseHeight;

			if(0 < height)
				v[ix][iy][iz] = Cell(ci[1] * CELLSIZE + iy < 0 ? Cell::Water : Cell::Air);
			else{
				float grassness = 0 < height || height < -10 ? 0 : cellFactorTable[0][ix][iy][iz] / (1 << -height);
				float dirtness = cellFactorTable[1][ix][iy][iz];
				float gravelness = cellFactorTable[2][ix][iy][iz];
				float rockness = cellFactorTable[3][ix][iy][iz] * (height < 0 ? 1.25 - 0.5 / (1. - height / 16.) : 0.75);

				Cell::Type ct;
				if (dirtness < grassness && gravelness < grassness && rockness < grassness)
					ct = Cell::Grass;
				else if (gravelness < dirtness && rockness < dirtness)
					ct = Cell::Dirt;
				else if(rockness < gravelness)
					ct = Cell::Gravel;
				else
					ct = Cell::Rock;
				world->bricks[ct]++;
				v[ix][iy][iz] = Cell(ct);
				_solidcount++;
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
	v[ix][iy][iz].adjacentWater =
		(cell(ix - 1, iy, iz).getType() == Cell::Water ? 1 : 0) +
        (cell(ix + 1, iy, iz).getType() == Cell::Water ? 1 : 0) +
        (cell(ix, iy - 1, iz).getType() == Cell::Water ? 1 : 0) +
        (cell(ix, iy + 1, iz).getType() == Cell::Water ? 1 : 0) +
        (cell(ix, iy, iz - 1).getType() == Cell::Water ? 1 : 0) +
        (cell(ix, iy, iz + 1).getType() == Cell::Water ? 1 : 0);
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
			if (c.type != Cell::Air && (c.adjacents != 0 && c.adjacents != 6))
			{
				if (!begun)
				{
					begun = true;
					_scanLines[ix][iz][0] = iy;
				}
				_scanLines[ix][iz][1] = iy + 1;
			}

			if(c.type == Cell::Water && c.adjacents != 6 && c.adjacentWater != 6 && c.adjacents + c.adjacentWater != 6){
				if(!transBegun){
					transBegun = true;
					tranScanLines[ix][iz][0] = iy;
				}
				tranScanLines[ix][iz][1] = iy + 1;
			}
		}
	}
}


VoxelEntity::VoxelEntity(WarField *w) : st(w){
}
