/// \file
/// \brief Implementation of VoxelEntity class.
#define NOMINMAX
#include "VoxelEntity.h"
#ifdef _WIN32
#include <Windows.h>
#endif
#include "Viewer.h"
#include "CoordSys.h"
#include "Game.h"
#include "sqadapt.h"
#include "../../src/noises/simplexnoise1234d.h"
#include "SqInitProcess.h"

extern "C"{
#include "clib/mathdef.h"
}



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

template<> void Entity::EntityRegister<VoxelEntity>::sq_defineInt(HSQUIRRELVM v){
	register_closure(v, _SC("modifyVoxel"), [](HSQUIRRELVM v){
		VoxelEntity *e = static_cast<VoxelEntity*>(sq_refobj(v));
		if(!e){
			sq_pushbool(v, SQFalse);
			return SQInteger(1);
		}
		try{
			ModifyVoxelCommand mvc;
			mvc.init(v, *e, 0);
			if(!e->command(&mvc))
				sq_pushbool(v, SQFalse);
			else
				sq_pushbool(v, mvc.retModified ? SQTrue : SQFalse);
			return SQInteger(1);
		}
		catch(SQFError &err){
			return sq_throwerror(v, err.what());
		}
		return SQInteger(0);
	});
	register_closure(v, _SC("setVoxel"), [](HSQUIRRELVM v){
		VoxelEntity *e = static_cast<VoxelEntity*>(sq_refobj(v));
		if(!e){
			sq_pushbool(v, SQFalse);
			return SQInteger(1);
		}
		try{
			Cell::Type ct = Cell::Rock;
			SQInteger sqct;
			if(SQ_SUCCEEDED(sq_getinteger(v, 2, &sqct)))
				ct = Cell::Type(sqct);

			e->setCell(0,0,0, ct);

			sq_pushbool(v, SQTrue);
			return SQInteger(1);
		}
		catch(SQFError &e){
			return sq_throwerror(v, e.what());
		}
		return SQInteger(0);
	});
}

Entity::EntityRegister<VoxelEntity> VoxelEntity::entityRegister("VoxelEntity");

gltestp::dstring VoxelEntity::texRockName;
gltestp::dstring VoxelEntity::texIronName;
gltestp::dstring VoxelEntity::texArmorName;

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

const char *Cell::typeName(Cell::Type type){
	switch(type){
	case Cell::Air: return "Air";
	case Cell::Rock: return "RockOre";
	case Cell::Iron: return "IronOre";
	case Cell::Armor: return "Armor";
	case Cell::ArmorSlope: return "ArmorSlope";
	case Cell::ArmorCorner: return "ArmorCorner";
	case Cell::ArmorInvCorner: return "ArmorInvCorner";
	case Cell::Engine: return "Engine";
	case Cell::Reactor: return "Reactor";
	default: return "Unknown";
	}
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

		auto updateIfExists = [this](Vec3i &ci){
			auto it = world->volume.find(ci);
			CellVolume *cv;
			if(it == world->volume.end())
				cv = &(world->volume[ci] = CellVolume(world, ci));
			else
				cv = &it->second;
			cv->updateCache();
		};

		if(ix <= 0)
			updateIfExists(Vec3i(index[0] - 1, index[1], index[2]));
		else if(CELLSIZE - 1 <= ix)
			updateIfExists(Vec3i(index[0] + 1, index[1], index[2]));
		if(iy <= 0)
			updateIfExists(Vec3i(index[0], index[1] - 1, index[2]));
		else if (CELLSIZE - 1 <= iy)
			updateIfExists(Vec3i(index[0], index[1] + 1, index[2]));
		if(iz <= 0)
			updateIfExists(Vec3i(index[0], index[1], index[2] - 1));
		else if (CELLSIZE - 1 <= iz)
			updateIfExists(Vec3i(index[0], index[1], index[2] + 1));
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


VoxelEntity::VoxelEntity(WarField *w) : st(w), volume(operator<), cellWidth(0.0025), baseHeight(0.01), noiseHeight(0.005), volumeInitialized(false){
	init();
}

void VoxelEntity::anim(double dt){
	if(!volumeInitialized){
		volumeInitialized = true;

		std::vector<CellVolume*> changed;
		double noiseHeight = getNoiseHeight();
		double baseHeight = getBaseHeight();
		double maxHeight = baseHeight + noiseHeight * 2.; // Theoretical limit height
		int maxCellCount = int(maxHeight / getCellWidth() / CELLSIZE) + 1;
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

	}
}

void VoxelEntity::init(){
	static bool initialized = false;
	if(!initialized){
		SqInit(game->sqvm, modPath() << _SC("scripts/VoxelEntity.nut"),
				StringProcess(texRockName, "rockTexture") <<=
				StringProcess(texIronName, "ironTexture") <<=
				StringProcess(texArmorName, "armorTexture")
				);
		initialized = true;
	}
}



bool VoxelEntity::command(EntityCommand *com){
	if(ModifyVoxelCommand *mvc = InterpretCommand<ModifyVoxelCommand>(com)){
		static const double boundHeight = 0.001;
		Vec3d src = mvc->src - this->pos;
		Vec3d dir = rot.itrans(mvc->dir);
		mvc->retModified = false;
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
							mvc->retModified = true;
							if(mvc->modifier){
								GainItemCommand gic;
								switch(ct){
								case Cell::Iron: gic.typeString = "IronOre"; break;
								case Cell::Rock: gic.typeString = "RockOre"; break;
								case Cell::ArmorSlope:
								case Cell::ArmorCorner:
								case Cell::ArmorInvCorner:
								case Cell::Armor: gic.typeString = "SteelPlate"; break;
								case Cell::Engine: gic.typeString = "Engine"; break;
								case Cell::Reactor: gic.typeString = "ReactorComponent"; break;
								}
								if(gic.typeString.len() != 0){
									gic.amount = -1.;
									mvc->modifier->command(&gic);
								}
							}
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
					mvc->retModified = true;
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
					mvc->retModified = true;
					if(mvc->modifier){
						GainItemCommand gic;
						gic.typeString = c.getType() == c.Iron ? "IronOre" : "RockOre";
						gic.amount = 1.;
						mvc->modifier->command(&gic);
					}
//					player->addBricks(c.getType(), 1);
					break;
				}
			}
		}

		return true;
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

bool VoxelEntity::setCell(int ix, int iy, int iz, const Cell &newCell){
	Vec3i ci = Vec3i(SignDiv(ix, CELLSIZE), SignDiv(iy, CELLSIZE), SignDiv(iz, CELLSIZE));
	VolumeMap::iterator it = volume.find(ci);
	CellVolume *cv;
	if(it == volume.end())
		cv = &(volume[ci] = CellVolume(this, ci));
	else
		cv = &it->second;
	volumeInitialized = true;
	return cv->setCell(SignModulo(ix, CELLSIZE), SignModulo(iy, CELLSIZE), SignModulo(iz, CELLSIZE), newCell);
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

SQInteger VoxelEntity::sqGet(HSQUIRRELVM v, const SQChar *name)const{
	if(!scstrcmp(name, _SC("cellWidth"))){
		sq_pushfloat(v, cellWidth);
		return 1;
	}
	else if(!scstrcmp(name, _SC("baseHeight"))){
		sq_pushfloat(v, baseHeight);
		return 1;
	}
	else if(!scstrcmp(name, _SC("noiseHeight"))){
		sq_pushfloat(v, noiseHeight);
		return 1;
	}
	else
		return st::sqGet(v, name);
}

SQInteger VoxelEntity::sqSet(HSQUIRRELVM v, const SQChar *name){
	SQInteger ret;
	auto setter = [this, &ret, &v](const SQChar *name, const SQChar *vname, double &vvalue){
		if(scstrcmp(name, vname))
			return false;
		if(volume.size()){
			ret = sq_throwerror(v, gltestp::dstring("Could not set ") << vname << _SC(" because VoxelEntity has already materialized"));
			return true;
		}
		SQFloat retf;
		if(SQ_FAILED(sq_getfloat(v, 3, &retf))){
			ret = sq_throwerror(v, gltestp::dstring("Value not convertible to float for ") << vname);
			return true;
		}
		vvalue = retf;
		ret = 0;
		return true;
	};

	if(setter(name, _SC("cellWidth"), cellWidth)
		|| setter(name, _SC("baseHeight"), baseHeight)
		|| setter(name, _SC("noiseHeight"), noiseHeight))
		return ret;
	else
		return st::sqSet(v, name);
}


#ifdef DEDICATED
void VoxelEntity::draw(WarDraw *){}
void VoxelEntity::drawtra(WarDraw *){}
#endif


IMPLEMENT_COMMAND(ModifyVoxelCommand, "ModifyVoxel")

void ModifyVoxelCommand::serialize(SerializeContext &sc){
	sc.o << src;
	sc.o << dir;
	sc.o << mode;
	sc.o << int(ct);
	sc.o << modifier;
}

void ModifyVoxelCommand::unserialize(UnserializeContext &sc){
	sc.i >> src;
	sc.i >> dir;
	sc.i >> (int&)mode;
	sc.i >> (int&)ct;
	sc.i >> modifier;
}

ModifyVoxelCommand::ModifyVoxelCommand(HSQUIRRELVM v, Entity &e){
	init(v, e);
}

void ModifyVoxelCommand::init(HSQUIRRELVM v, Entity &e, int offset){
	int argc = sq_gettop(v);
	if(argc < 2)
		throw SQFArgumentError();
	SQVec3d qvsrc;
	qvsrc.getValue(v, 2 + offset);
	src = qvsrc.value;
	SQVec3d qvdir;
	qvdir.getValue(v, 3 + offset);
	dir = qvdir.value;

	const SQChar *sqstr;
	if(SQ_FAILED(sq_getstring(v, 4 + offset, &sqstr)))
		throw SQFError(_SC("4th argument must be string"));
	if(!scstrcmp(sqstr, _SC("Put")))
		mode = Put;
	else if(!scstrcmp(sqstr, _SC("Remove")))
		mode = Remove;
	else if(!scstrcmp(sqstr, _SC("Preview")))
		mode = Preview;

	ct = Cell::Rock;
	SQInteger sqct;
	if(SQ_SUCCEEDED(sq_getinteger(v, 5 + offset, &sqct)))
		ct = Cell::Type(sqct);

	rotation = 0;
	SQInteger sqrotation;
	if(SQ_SUCCEEDED(sq_getinteger(v, 6 + offset, &sqrotation)))
		rotation = sqrotation;

	modifier = Entity::sq_refobj(v, 7 + offset);
}


IMPLEMENT_COMMAND(GainItemCommand, "GainItem")

void GainItemCommand::serialize(SerializeContext &sc){
	sc.o << typeString;
	sc.o << amount;
}

void GainItemCommand::unserialize(UnserializeContext &sc){
	sc.i >> typeString;
	sc.i >> amount;
}

GainItemCommand::GainItemCommand(HSQUIRRELVM v, Entity &){
	int argc = sq_gettop(v);
	if(argc < 2)
		throw SQFArgumentError();
	const SQChar *str;
	if(SQ_FAILED(sq_getstring(v, 2, &str)))
		throw SQFError(_SC("typeString required"));
	typeString = str;

	SQFloat f;
	if(SQ_SUCCEEDED(sq_getfloat(v, 3, &f)))
		amount = f;
}

