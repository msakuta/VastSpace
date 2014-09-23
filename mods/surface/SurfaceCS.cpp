/** \file
 * \brief Implementation of SurfaceCS class.
 */
#include "SurfaceCS.h"
#include "EntityRegister.h"
#include "WarMap.h"
#include "drawmap.h"
#include "sqadapt.h"
#include "btadapt.h"
#include "war.h"
#include "Entity.h"
#include "Player.h"
#include "Game.h"
#include "cmd.h"
#include "stellar_file.h"
extern "C"{
#include <clib/timemeas.h>
#include <clib/gl/gldraw.h>
}
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <fstream>

/// \brief A placeholder Entity that receives Bullets as the ground terrain.
class SurfaceEntity : public Entity{
public:
	typedef Entity st;
	SurfaceEntity(Game *game) : st(game){}
	SurfaceEntity(WarField *aw) : st(aw){race = -1;} ///< Ground surface is everyone's friend.
	static EntityRegister<SurfaceEntity> entityRegister;
	const char *idname()const override{return "SurfaceEntity";}
	const char *classname()const override{return "SurfaceEntity";}
	int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal)override;
	double getHitRadius()const override{return 100.;}
	bool isTargettable()const override{return true;}
	bool isSelectable()const override{return false;}
};

class SurfaceWar : public WarSpace{
public:
	typedef WarSpace st;
	SurfaceWar(Game *game) : st(game), entity(NULL){}
	SurfaceWar(SurfaceCS *);
	~SurfaceWar();
	Vec3d accel(const Vec3d &pos, const Vec3d &velo)const override;
	double atmosphericPressure(const Vec3d &pos)const override;
	bool sendMessage(Message &)override;
protected:
	btStaticPlaneShape *groundPlane;
	btRigidBody *groundBody;
	SurfaceEntity *entity;
};

Entity::EntityRegister<SurfaceEntity> SurfaceEntity::entityRegister("SurfaceEntity");

int SurfaceEntity::tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal){
#if 1
	double t;
	if(static_cast<SurfaceCS*>(w->cs)->traceHit(start, dir, rad, dt, &t, NULL, retnormal)){
		// The position vector returned by SurfaceCS::traceHit() is in the heightfield's local coordinates,
		// which is somewhat complex to convert back to world coordinates.  So we just use the ray's vector
		// equaion parameter when the ray hits the terrain to calculate hit position.
		if(retp)
			*retp = start + dir * t;
		if(ret)
			*ret = t;
		return 1;
	}
#else
	double height = ((SurfaceCS*)w->cs)->getHeight(start[0], start[2], retnormal);
	// If the starting point is under the ground, it's obviously hit.
	if(start[1] < height){
		if(ret)
			*ret = 0;
		if(retp)
			*retp = start;
		return 1;
	}
	else
#endif
		return 0;
}


ClassRegister<SurfaceCS> SurfaceCS::classRegister("Surface");
const CoordSys::Static &SurfaceCS::getStatic()const{
	return classRegister;
}

SurfaceCS::SurfaceCS(Game *game) : st(game),
	wm(NULL),
	map_top(NULL),
	mapshape(NULL),
	bbody(NULL),
	tin(NULL),
	tinResample(16),
	initialized(false),
	cbody(NULL)
{
//	init();
}

extern btRigidBody *newbtRigidBody(const btRigidBody::btRigidBodyConstructionInfo&);

SurfaceCS::SurfaceCS(const char *path, CoordSys *root) : st(path, root),
	wm(NULL),
	map_top(NULL),
	mapshape(NULL),
	bbody(NULL),
	tin(NULL),
	tinResample(16),
	initialized(false),
	cbody(NULL)
{
//	init();
}

void SurfaceCS::init(){
	if(initialized)
		return;
#if 1
	int sx, sy;
	btScalar fmax = 0;
	if(tinFileName.len() != 0){
		timemeas_t tm;
		TimeMeasStart(&tm);

		wm = tin = new TIN(tinFileName);

		// File name for architecture-dependent cached DEM file to enhance speed of repeated
		// program startup.
		const gltestp::dstring demFileName = gltestp::dstring("cache/") + tinFileName + ".dem";

		int trunc = tinResample; // Resampled height field is truncated to this frequency.
		tin->size(&sx, &sy);
		sx /= trunc;
		sy /= trunc;
		std::vector<btScalar> *heightField = new std::vector<btScalar>(sx * sy);

		bool validDem = false;

		do{
			// Load cached DEM file if available.
			std::ifstream ifs(demFileName, std::ios::binary);

			// The DEM file can be absent, so check it here.
			if(ifs.good()){
				int sx2;
				int sy2;
				ifs.read((char*)&sx2, sizeof sx2);
				ifs.read((char*)&sy2, sizeof sy2);
				// If we have a cached DEM with dimensions that do not match our expectation,
				// re-cache the data without further reading.
				if(sx2 != sx || sy2 != sy)
					break;
				ifs.read((char*)&fmax, sizeof fmax);
				ifs.read((char*)&heightField->front(), sx * sy * sizeof(heightField->front()));
				validDem = true;
			}
		} while(0);

		if(!validDem){
			// Resampling TIN to create a DEM is very time-consuming process, because it's not
			// very optimized in terms of algorithms.
			for(int y = 0; y < sy; y++) for(int x = 0; x < sx; x++){
				WarMapTile t;
				if(tin->getat(&t, x * trunc, y * trunc) == 0)
					assert(0);
				btScalar v = t.height * 1e-3 / 4.;
				(*heightField)[x + (y) * sx] = v;
				if(fmax < v)
					fmax = v;
			}

			// If we have successfully resampled the TIN to create a DEM, save it to a file.
			std::ofstream ofs(demFileName, std::ios::binary);
			ofs.write((char*)&sx, sizeof sx);
			ofs.write((char*)&sy, sizeof sy);
			ofs.write((char*)&fmax, sizeof fmax);
			ofs.write((char*)&heightField->front(), sx * sy * sizeof(heightField->front()));
		}
		mapshape = new btHeightfieldTerrainShape(sx, sy, &heightField->front(), .0001, 0., fmax, 1, PHY_FLOAT, false);

		//	delete heightField;

		// First invocation (without cache) can take minutes!
		CmdPrint(gltestp::dstring("TIN load time: ") << TimeMeasLap(&tm) << " sec");
	}
#else
	wm = OpenHGTMap("N36W113.av.zip");
	if(game->isClient())
		dmc = CacheDrawMap(wm);

	int sx, sy;
	wm->size(&sx, &sy);
	mapshape = new btHeightfieldTerrainShape(sx, sy, const_cast<void*>(wm->rawData()), .001, 0., 2., 1, wm->rawType(), false);
#endif
	if(mapshape){
		// I hope this margin helps to prevent objects falling below the ground.
		mapshape->setMargin(.01);

		static const double longscale = cos(35 / deg_per_rad); // Longitude scaling

		// The heightfield in Bullet assumes (one raw element length) = (one space unit) so we need to adjust it
		// by size of the heightfield.
		mapshape->setLocalScaling(btVector3(wm->width() / (sx - 1) * longscale, 1., wm->width() / (sy - 1)));

		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btVector3(0, fmax / 2., 0));
		startTransform.setRotation(btQuaternion(btVector3(0,1,0), M_PI));

		//using motionstate is recommended by the Bullet documentation, but I'm not sure it applies here.
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(0.,myMotionState, mapshape);
		bbody = newbtRigidBody(rbInfo);

		// Just a debug code to confirm how the AABB's values are.
		btVector3 aabbMin, aabbMax;
		bbody->getAabb(aabbMin, aabbMax);
		assert(aabbMin.x() < aabbMax.x());
	}
	w = new SurfaceWar(this);
	initialized = true;
}

SurfaceCS::~SurfaceCS(){
	delete wm;
	delete dmc;
	delete bbody;
	if(wm != tin)
		delete tin;
}

double SurfaceCS::getHeight(double x, double z, Vec3d *normal)const{
	if(wm){
		static const double longscale = cos(35 / deg_per_rad); // Longitude scaling

		int sx, sy;
		tin->size(&sx, &sy);
		double width = tin->width() / 2;
		Vec3d scales(-width * 2 * longscale / sx, -width * 2 / sy, 1e-3 / 4.);
		Vec3d temp;
		double ret = tin->getHeight(((-x / width / 2 / longscale + 0.5 - 0.5 / sx) * sx), (-z / width / 2 + 0.5 - 0.5 / sy) * sy, &scales, &temp);
		if(normal){
			(*normal)[0] = -temp[0];
			(*normal)[1] = -temp[2];
			(*normal)[2] = -temp[1];
			normal->normin();
		}
		return ret * 1e-3 / 4.;
	}
	else if(cbody){
		Vec3d basepos = cbody->tocs(Vec3d(x, 0, z), this);
		Vec3d basenorm = basepos.norm();
		return cbody->getTerrainHeight(basenorm) * cbody->rad - basepos.len();
	}
	else
		return 0.;
}

bool SurfaceCS::traceHit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal)const{
	if(tin){
		static const double longscale = cos(35 / deg_per_rad); // Longitude scaling
		int sx, sy;
		tin->size(&sx, &sy);
		double width = tin->width() / 2;
		Vec3d scales(-width * 2 * longscale / sx, -width * 2 / sy, 1e-3 / 4.);
		Vec3d lstart(((-start[0] / width / 2 / longscale + 0.5 - 0.5 / sx) * sx), (-start[2] / width / 2 + 0.5 - 0.5 / sy) * sy, start[1] / scales[2]);
		Vec3d ldir(dir[0] / scales[0], dir[2] / scales[1], dir[1] / scales[2]);
		WarMap::TraceResult tr;
		bool bret = tin->traceHit(WarMap::TraceParams(lstart, ldir, dt, rad), tr);
		if(bret){
			if(ret) *ret = tr.hitTime;
			if(retp) *retp = tr.position;
			if(retnormal){
				(*retnormal)[0] = -tr.normal[0];
				(*retnormal)[1] = -tr.normal[2];
				(*retnormal)[2] = -tr.normal[1];
				retnormal->normin();
			}
		}
		return bret;
	}
	else
		return false;
}


void SurfaceCS::anim(double dt){
	st::anim(dt);
	if(cbody == NULL){
		for(auto c = parent; c; c = c->next){
			auto t = dynamic_cast<RoundAstrobj*>(c);
			if(t){
				cbody = t;
				break;
			}
		}
	}
}

/// Reset the flag to instruct the drawmap function to recalculate the pyramid buffer.
void SurfaceCS::predraw(const Viewer *vw){
	map_checked = 0;
}

/// Note that this function is going to be called at least twice a frame.
void SurfaceCS::draw(const Viewer *vw){
	if((wm || tin) && vw->zslice == 0){
		glPushMatrix();
		if(vw->zslice == 1){
			gldTranslate3dv(-vw->pos);
		}
		if(tin)
			tin->draw();
//		if(wm)
//			drawmap(wm, *vw, 0, vw->viewtime, vw->gc, &map_top, &map_checked, dmc);
		glPopMatrix();
	}
}

bool SurfaceCS::readFile(StellarContext &sc, int argc, const char *argv[]){
	const char *s = argv[0], *ps = argv[1];
	if(!strcmp(s, "tin_file")){
		if(s = ps/*strtok(ps, " \t\r\n")*/){
			this->tinFileName = s;
		}
		return true;
	}
	else if(!strcmp(s, "tin_resample")){
		if(s = ps){
			StackReserver st2(sc.v);
			cpplib::dstring dst = cpplib::dstring("return(") << s << ")";
			if(SQ_SUCCEEDED(sq_compilebuffer(sc.v, dst, dst.len(), _SC("rotation"), SQTrue))){
				sq_push(sc.v, -2);
				if(SQ_SUCCEEDED(sq_call(sc.v, 1, SQTrue, SQTrue))){
					SQInteger i;
					if(SQ_SUCCEEDED(sq_getinteger(sc.v, -1, &i)))
						this->tinResample = int(i);
				}
			}
			return true;
		}
		return true;
	}
	else
		return st::readFile(sc, argc, argv);
}

bool SurfaceCS::readFileEnd(StellarContext &sc){
	init();
	return st::readFileEnd(sc);
}







SurfaceWar::SurfaceWar(SurfaceCS *cs) : st(cs), entity(new SurfaceEntity(this)){
	addent(entity);

	// Just an experiment. Really should be pulled out from planet's mass and radius.
	bdw->setGravity(btVector3(0, -.0098,  0));

	// The heightfield should have been already initialized at this time.
	if(cs->bbody)
		bdw->addRigidBody(cs->bbody);

	// Create a ground (y=0) plane to prevent objects from falling forever.
	groundPlane = new btStaticPlaneShape(btVector3(0,1,0),50);

	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(0,-50,0));

	// The plane is static; never moves.
	btScalar mass(0.);
	btVector3 localInertia(0,0,0);

	// I think the motionstate thing isn't really necessary here.
	btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,groundPlane,localInertia);
	groundBody = new btRigidBody(rbInfo);

	// Don't forget to add the body to the dynamics world
	bdw->addRigidBody(groundBody);
}

Vec3d SurfaceWar::accel(const Vec3d &pos, const Vec3d &velo)const{
	return Vec3d(0, -0.0098, 0);
}

double SurfaceWar::atmosphericPressure(const Vec3d &pos)const{
	const double scaleHeight = 8.5;
	return exp(-pos[1] / scaleHeight);
}

bool SurfaceWar::sendMessage(Message &m){
	if(GetCoverPointsMessage *p = InterpretMessage<GetCoverPointsMessage>(m)){
		CoverPointVector &ret = p->cpv;
		for(EntityList::iterator it = el.begin(); it != el.end(); ++it){
			GetCoverPointsCommand com;
			Entity *e = *it;
			if(e->command(&com)){
				for(CoverPointVector::iterator it2 = com.cpv.begin(); it2 != com.cpv.end(); ++it2)
					ret.push_back(*it2);
			}
		}
		return true;
	}
	else
		return st::sendMessage(m);
}

SurfaceWar::~SurfaceWar(){
	delete groundBody;
	delete groundPlane;
}

IMPLEMENT_COMMAND(GetCoverPointsCommand, "GetCoverPointsCommand");

GetCoverPointsCommand::GetCoverPointsCommand(HSQUIRRELVM v, Entity &e){}
