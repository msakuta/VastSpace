/// \file
/// \brief Definition of VoxelEntity class.
#ifndef VOXELENTITY_H
#define VOXELENTITY_H
#include "Autonomous.h"
#include "EntityRegister.h"
#include "EntityCommand.h"
#include "../../src/SignModulo.h"

#include <cpplib/vec2.h>

#include <functional>

class Game;
class VoxelEntity;
class Engineer;

const int CELLSIZE = 16;

/// <summary>The atomic unit of the world.</summary>
class Cell{
public:
	enum Type{
		Empty, ///< Null cell
		Rock, ///< Rock cell
		Iron, ///< Iron ore cell
		Armor,
		ArmorSlope,
		ArmorCorner,
		ArmorInvCorner,
		Thruster,
		Reactor,
		Cockpit,
		Occupied,
		NumTypes
	};

	Cell(Type t = Empty, char arotation = 0) : type(t), adjacents(0), rotation(arotation){}
	Type getType()const{return type;}
	short getValue()const{return value;}
	void setValue(short avalue){value = avalue;}
	int getAdjacents()const{return adjacents;}
	char getRotation()const{return rotation;}
	bool isSolid()const{return type != Empty;}
	bool isTranslucent()const{
		return type == Empty || type == ArmorSlope || type == ArmorCorner || type == ArmorInvCorner
			|| type == Reactor || type == Cockpit || type == Occupied;
	}
	/// Whether this cell is made of natural asteroid materials
	bool isAsteroid()const{
		return type == Rock || type == Iron;
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

	/// \brief Obtains type name as a string from cell type
	static const char *typeName(Type type);
protected:
	enum Type type;
	short value;
	char adjacents;
	char rotation;

	friend class CellVolume;
	friend SerializeStream &operator<<(SerializeStream &, const Cell &);
	friend UnserializeStream &operator>>(UnserializeStream &, Cell &);
};

struct VERTEX{
	Vec3d pos;
	Vec3d norm;
	Vec2d tex;
	bool operator==(const VERTEX &o)const{
		return pos == o.pos && norm == o.norm && tex == o.tex;
	}
};

/// Type for smoothing vertex normals.  A pointer to a VERTEX doesn't work because
/// the VERTEX vector can be reallocated while enumerating vertices.
struct VertexMapValue{
	int material;
	int index;
};

/// Map for accumulating vertices at the same position
typedef std::map<Vec3d, std::vector<VertexMapValue>, bool (*)(const Vec3d&, const Vec3d&)> VertexMap;

/// Operator to order vectors to build a Vec3d map
static bool operator<(const Vec3d &a, const Vec3d &b){
	return a[0] < b[0] ? true : b[0] < a[0] ? false
		: a[1] < b[1] ? true : b[1] < a[1] ? false
		: a[2] < b[2];
}

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

	/// \brief Scanlines for solid cells
	///
	/// Because rendered cells and solid (content) cells are not necessarily the same,
	/// we should have distinct scanline buffers for them.
	int solidScanLines[CELLSIZE][CELLSIZE][2];

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
			solidScanLines[ix][iy][iz] = 0;
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
			v[ipos[0]][ipos[1]][ipos[2]].getType() != Cell::Empty;
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

	/// \brief The function object type for enumerating solid blocks
	typedef std::function<void (const Vec3i &idx, const Cell &)> EnumSolidProc;

	/// \brief Enumerates only solid blocks and passes them to the given function object.
	/// \param callback The function object to process the cell.
	void enumSolid(EnumSolidProc &callback);

	void serialize(std::ostream &o);
	void unserialize(std::istream &i);

	static int cellInvokes;
	static int cellForeignInvokes;
	static int cellForeignExists;
	friend class VoxelEntity;
};

/// \brief An EntityCommand for making changes to cells in VoxelEntity
///
/// It can place or remove a voxel by parameters.
struct ModifyVoxelCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(ModifyVoxelCommand, EntityCommand);
	ModifyVoxelCommand(){}
	ModifyVoxelCommand(HSQUIRRELVM v, Entity &e);

	void init(HSQUIRRELVM v, Entity &e, int offset = 1);

	// Vector parameters of a ray for placing or removing a voxel.
	Vec3d src;
	Vec3d dir;

	enum Mode{Remove, Put, Preview};
	Mode mode; ///< Whether to put a voxel (false means removing)
	Cell::Type ct; ///< Cell type to place
	char rotation;
	Entity *modifier;

	bool retModified; ///< Returned flag whether the target VoxelEntity has modified

	virtual void serialize(SerializeContext &);
	virtual void unserialize(UnserializeContext &);
};

struct GainItemCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(GainItemCommand, EntityCommand);
	GainItemCommand(){}
	GainItemCommand(HSQUIRRELVM v, Entity &e);

	gltestp::dstring typeString;
	double amount;

	virtual void serialize(SerializeContext &);
	virtual void unserialize(UnserializeContext &);
};

struct EnterCockpitCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(EnterCockpitCommand, EntityCommand);
	EnterCockpitCommand(){}
	EnterCockpitCommand(HSQUIRRELVM v, Entity &e);

	Engineer *requester;

	virtual void serialize(SerializeContext &);
	virtual void unserialize(UnserializeContext &);
};

class VoxelEntity : public Autonomous{
public:
	typedef Autonomous st;

	VoxelEntity(Game *game);
	VoxelEntity(WarField *w);
	~VoxelEntity()override;

	static EntityRegister<VoxelEntity> entityRegister;
	EntityStatic &getStatic()const override{return entityRegister;}

	void init();

	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
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
	bool buildBody()override;

//	virtual short getDefaultCollisionMask()const;

	double getHitRadius()const override{return getBaseHeight() + getNoiseHeight() * 2.;}

	typedef std::map<Vec3i, CellVolume, bool(*)(const Vec3i &, const Vec3i &)> VolumeMap;

	Vec3i real2ind(const Vec3d &pos)const;
	Vec3d ind2real(const Vec3i &ipos)const;
	double getCellWidth()const{return cellWidth;} ///< Length of an edge of a cell, in kilometers
	double getBaseHeight()const{return baseHeight;}
	double getNoiseHeight()const{return noiseHeight;}

	const Cell &cell(int ix, int iy, int iz)const{
		Vec3i ci = Vec3i(SignDiv(ix, CELLSIZE), SignDiv(iy, CELLSIZE), SignDiv(iz, CELLSIZE));
		VolumeMap::const_iterator it = volume.find(ci);
		if(it != volume.end()){
			return it->second(SignModulo(ix, CELLSIZE), SignModulo(iy, CELLSIZE), SignModulo(iz, CELLSIZE));
		}
		else
			return CellVolume::v0;
	}
	const Cell &cell(const Vec3i &pos)const{
		return cell(pos[0], pos[1], pos[2]);
	}

	bool setCell(int ix, int iy, int iz, const Cell &newCell);

	bool isSolid(int ix, int iy, int iz){
		return isSolid(Vec3i(ix, iy, iz));
	}

	bool isSolid(const Vec3i &v);
	bool isSolid(const Vec3d &rv);

	static gltestp::dstring modPath(){
		return "mods/engineers/";
	}

	/// \brief Returns position and rotation of the cockpit being used
	/// \param rot A reference to a variable to hold rotation in world coordinates after this function returns
	Vec3d getControllerCockpitPos(Quatd &rot)const;

	const ManeuverParams &getManeuve()const override;

	/// Serialize only blocks configuration, without belonging WarField, position or orientation information
	void serializeBlocks(SerializeStream &ss);
	/// Unserialize only blocks configuration
	void unserializeBlocks(UnserializeStream &us);

protected:
	SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;

	void drawCell(const Cell &cell, const Vec3i &pos, Cell::Type &celltype, const CellVolume *cv = NULL, const Vec3i &posInVolume = Vec3i(0,0,0),
		std::vector<VERTEX> *vlist = NULL, std::vector<GLuint> *vidx = NULL, VertexMap *vertexMap = NULL)const;

	/// \brief Update the maneuverParams variable specific to this VoxelEntity.
	///
	/// A VoxelEntity's maneuverability depends on its block configuration, so it's different
	/// between instances.  We cannot universally define a fixed ManeuverParams to properly
	/// adapt to this situation, but Autonomous class tree assumes it's stored in a static-like
	/// memory storage.  To address this issue, we define a member variable for each VoxelEntity
	/// to hold ManeuverParams and occasionally update it.
	void updateManeuverParams();

	double cellWidth;
	double baseHeight;
	double noiseHeight;
	mutable ManeuverParams maneuverParams;
	static const ManeuverParams defaultManeuverParams;
	btCompoundShape *shape;
	btDefaultMotionState* myMotionState;
	bool shapeDirty;
private:
	VolumeMap volume;
	int bricks[Cell::NumTypes];
	Vec3i previewCellPos;
	Cell previewCell;
	bool volumeInitialized;
	WeakPtr<Engineer> controllerEngineer;
	Vec3i controllerCockpitPos;

	static gltestp::dstring texRockName;
	static gltestp::dstring texIronName;
	static gltestp::dstring texArmorName;

	static bool texlist_init;
	static GLuint texlist_rock;
	static GLuint texlist_iron;
	static GLuint texlist_armor;
	static void initTexLists();

	friend class CellVolume;
};

#endif
