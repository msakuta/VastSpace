#include "SurfaceCS.h"
#include "WarMap.h"
#include "drawmap.h"
#include "btadapt.h"
#include "war.h"
#include "Entity.h"
#include "Player.h"
#include "Game.h"
extern "C"{
#include <clib/gl/gldraw.h>
}
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

class SurfaceWar : public WarSpace{
public:
	typedef WarSpace st;
	SurfaceWar(Game *game) : st(game){}
	SurfaceWar(SurfaceCS *);
	~SurfaceWar();
	Vec3d accel(const Vec3d &pos, const Vec3d &velo)const override;
protected:
	btStaticPlaneShape *groundPlane;
	btRigidBody *groundBody;
};

ClassRegister<SurfaceCS> SurfaceCS::classRegister("Surface");
const CoordSys::Static &SurfaceCS::getStatic()const{
	return classRegister;
}

SurfaceCS::SurfaceCS(Game *game) : st(game), wm(NULL), map_top(NULL), mapshape(NULL), bbody(NULL){
	init();
}

extern btRigidBody *newbtRigidBody(const btRigidBody::btRigidBodyConstructionInfo&);

SurfaceCS::SurfaceCS(const char *path, CoordSys *root) : st(path, root), wm(NULL), map_top(NULL), mapshape(NULL), bbody(NULL){
	init();
}

void SurfaceCS::init(){
#if 1
	tin = new TIN("triangles.all");
#else
	wm = OpenHGTMap("N36W113.av.zip");
	if(game->isClient())
		dmc = CacheDrawMap(wm);

	int sx, sy;
	wm->size(&sx, &sy);
	mapshape = new btHeightfieldTerrainShape(sx, sy, const_cast<void*>(wm->rawData()), .001, 0., 2., 1, wm->rawType(), false);
	if(mapshape){
		// I hope this margin helps to prevent objects falling below the ground.
		mapshape->setMargin(.01);

		// The heightfield in Bullet assumes (one raw element length) = (one space unit) so we need to adjust it
		// by size of the heightfield.
		mapshape->setLocalScaling(btVector3(wm->width() / sx, 1., wm->width() / sy));

		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btVector3(0, 1 + .03, 0));

		//using motionstate is recommended by the Bullet documentation, but I'm not sure it applies here.
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(0.,myMotionState, mapshape);
		bbody = newbtRigidBody(rbInfo);

		// Just a debug code to confirm how the AABB's values are.
		btVector3 aabbMin, aabbMax;
		bbody->getAabb(aabbMin, aabbMax);
		assert(aabbMin.x() < aabbMax.x());
	}
#endif
	w = new SurfaceWar(this);
}

SurfaceCS::~SurfaceCS(){
	delete wm;
	delete dmc;
	delete bbody;
	delete tin;
}

void SurfaceCS::anim(double dt){
	st::anim(dt);
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
		if(wm)
			drawmap(wm, *vw, 0, vw->viewtime, vw->gc, &map_top, &map_checked, dmc);
		glPopMatrix();
	}
}







SurfaceWar::SurfaceWar(SurfaceCS *cs) : st(cs){
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

SurfaceWar::~SurfaceWar(){
	delete groundBody;
	delete groundPlane;
}
