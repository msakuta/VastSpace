/** \file
 * \brief Implementation of SpacePlane.
 */
#include "SpacePlane.h"
#include "vastspace.h"
#include "motion.h"
#include "btadapt.h"
#include "Island3.h"
#include "sqadapt.h"
#include "tefpol3d.h"
#include "SqInitProcess-ex.h"




#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.2, COLOR32RGBA(255,255,191,0)},
	{0.2, COLOR32RGBA(255,255,191,255)},
	{0.3, COLOR32RGBA(255,255,31,191)},
	{0.9, COLOR32RGBA(255,127,31,95)},
	{0.6, COLOR32RGBA(255,31,0,63)},
};
static const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 2.2);

std::vector<Vec3d> SpacePlane::engines;
double SpacePlane::modelScale = .0004;
double SpacePlane::hitRadius = 0.080;
double SpacePlane::defaultMass = 2e7;
double SpacePlane::maxHealthValue = 15000.;
GLuint SpacePlane::overlayDisp = 0;





SpacePlane::SpacePlane(WarField *aw) : st(aw), ai(NULL){
	init();
}

SpacePlane::SpacePlane(Entity *docksite) : st(docksite->w), ai(NULL){
	init();
	ai = new TransportAI(this, docksite);
}

void SpacePlane::init(){

	static bool initialized = false;
	if(!initialized){
		sq_init(modPath() << _SC("models/SpacePlane.nut"),
			ModelScaleProcess(modelScale) <<=
			SingleDoubleProcess(hitRadius, _SC("hitRadius")) <<=
			MassProcess(defaultMass) <<=
			SingleDoubleProcess(maxHealthValue, _SC("maxhealth"), false) <<=
			Vec3dListProcess(engines, _SC("engines")) <<=
			DrawOverlayProcess(overlayDisp)
			);
		initialized = true;
	}

	undocktime = 0.f;
	health = getMaxHealth();
	mass = defaultMass;
	people = RandomSequence((unsigned long)this).next() % 100 + 100;
	engineHeat = 0.f;

	pf.resize(engines.size());
	for(int i = 0; i < pf.size(); i++)
		pf[i] = NULL;
}

SpacePlane::~SpacePlane(){
	if(ai){
		ai->unlink(this);
		ai = NULL;
	}
//	delete ai;
}

const char *SpacePlane::idname()const{
	return "SpacePlane";
}

const char *SpacePlane::classname()const{
	return "SpacePlane";
}

const unsigned SpacePlane::classid = registerClass("SpacePlane", Conster<SpacePlane>);
Entity::EntityRegister<SpacePlane> SpacePlane::entityRegister("SpacePlane");

void SpacePlane::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << ai;
	sc.o << undocktime;
	sc.o << people;
}

void SpacePlane::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> ai;
	sc.i >> undocktime;
	sc.i >> people;

	// Re-create temporary entities if flying in a WarSpace.
	// If surrounding WarSpace is unserialized after this function, it would cause errors,
	// but while WarSpace::dive defines that the WarSpace itself go first, it will never happen.
	WarSpace *ws;
	if(w && (ws = (WarSpace*)w)){
		buildBody();
		ws->bdw->addRigidBody(bbody, 1, ~2);
		setPosition(&pos, &rot, &this->velo, &omg);
		for(int i = 0; i < pf.size(); i++)
			pf[i] = ws->tepl->addTefpolMovable(this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICKEST | TEP3_ROUGH, cs_orangeburn.t);
	}
	else{
		for(int i = 0; i < pf.size(); i++)
			pf[i] = NULL;
	}
}

void SpacePlane::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	if(ai)
		ai->dive(sc, method);
}

const char *SpacePlane::dispname()const{
	return "SpacePlane";
}

void SpacePlane::anim(double dt){
	WarSpace *ws = *w;
	if(!ws){
		st::anim(dt);
		return;
	}

	if(bbody && !warping){
		const btTransform &tra = bbody->getCenterOfMassTransform();
		pos = btvc(tra.getOrigin());
		rot = btqc(tra.getRotation());
		velo = btvc(bbody->getLinearVelocity());
	}

	/* forget about beaten enemy */
	if(enemy && (enemy->getHealth() <= 0. || enemy->w != w))
		enemy = NULL;

	Mat4d mat;
	transform(mat);

	if(0 < health){
		Entity *collideignore = NULL;
		if(ai){
			if(ai->control(this, dt)){
				ai->unlink(this);
				ai = NULL;
			}
			if(!w)
				return;
		}
		else if(task == sship_undock){
			if(undocktime < 0.){
				inputs.press &= ~PL_W;
				task = sship_idle;
			}
			else{
				inputs.press |= PL_W;
				undocktime -= dt;
			}
		}
		else if(controller){
		}
		else if(!enemy && task == sship_parade){
			Entity *pm = mother ? mother->e : NULL;
			if(mother){
				if(paradec == -1)
					paradec = mother->enumParadeC(mother->Frigate);
				Vec3d target, target0(1.5, -1., -1.);
				Quatd q2, q1;
				target0[0] += paradec % 10 * .30;
				target0[2] += paradec / 10 * -.30;
				target = pm->rot.trans(target0);
				target += pm->pos;
				Vec3d dr = this->pos - target;
				if(dr.slen() < .10 * .10){
					q1 = pm->rot;
					inputs.press &= ~PL_W;
//					parking = 1;
					this->velo += dr * (-dt * .5);
					q2 = Quatd::slerp(this->rot, q1, 1. - exp(-dt));
					this->rot = q2;
				}
				else{
	//							p->throttle = dr.slen() / 5. + .01;
					steerArrival(dt, target, pm->velo, 1. / 10., .001);
				}
			}
			else
				task = sship_idle;
		}
		else if(task == sship_idle){
			if(race != 0 /*RandomSequence((unsigned long)this + (unsigned long)(w->war_time() / .0001)).nextd() < .0001*/){
				command(&DockCommand());
			}
			inputs.press = 0;
		}
		else{
			inputs.press = 0;
		}
	}
	else{
		this->w = NULL;
		return;
	}

	st::anim(dt);

	// inputs.press is filtered in st::anim, so we put tefpol updates after it.
	for(int i = 0; i < engines.size(); i++) if(pf[i]){
		pf[i]->move(mat.vp3(engines[i]), avec3_000, cs_orangeburn.t, !(inputs.press & PL_W));
	}

//	engineHeat = approach(engineHeat, direction & PL_W ? 1.f : 0.f, dt, 0.);
	// Exponential approach is more realistic (but costs more CPU cycles)
	engineHeat = direction & PL_W ? engineHeat + (1. - engineHeat) * (1. - exp(-dt)) : engineHeat * exp(-dt);


#if 0
	if(p->pf){
		int i;
		avec3_t pos, pos0[numof(p->pf)] = {
			{.0, -.003, .045},
		};
		for(i = 0; i < numof(p->pf); i++){
			MAT4VP3(pos, mat, pos0[i]);
			MoveTefpol3D(p->pf[i], pos, avec3_000, cs_orangeburn.t, 0);
		}
	}
#endif
}

void SpacePlane::postframe(){
	st::postframe();
}

void SpacePlane::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	pos = this->rot.trans(Vec3d(0, 120, 50) * modelScale) + this->pos;
	rot = this->rot;
}

void SpacePlane::enterField(WarField *target){
	WarSpace *ws = *target;
	if(ws && ws->bdw){
		buildBody();
		ws->bdw->addRigidBody(bbody, 1, ~2);
	}
	if(ws){
		TefpolList *tepl = w ? w->getTefpol3d() : NULL;
		for(int i = 0; i < pf.size(); i++){
			if(this->pf[i])
				this->pf[i]->immobilize();
			if(tepl)
				pf[i] = tepl->addTefpolMovable(this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICKEST | TEP3_ROUGH, cs_orangeburn.t);
			else
				pf[i] = NULL;
		}
	}
}

bool SpacePlane::buildBody(){
	if(!bbody){
		static btCompoundShape *shape = NULL;
		if(!shape){
			shape = new btCompoundShape();
			Vec3d sc = Vec3d(50, 40, 150) * modelScale;
			const Quatd rot = quat_u;
			const Vec3d pos = Vec3d(0,0,0);
			btBoxShape *box = new btBoxShape(btvc(sc));
			btTransform trans = btTransform(btqc(rot), btvc(pos));
			shape->addChildShape(trans, box);
			shape->addChildShape(btTransform(btqc(rot), btVector3(0, 0, 100) * modelScale), new btBoxShape(btVector3(150, 10, 50) * modelScale));
		}

		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btvc(pos));

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
		bbody = new btRigidBody(rbInfo);
		return true;
	}
	return false;
}

/// if we are transitting WarField or being destroyed, trailing tefpols should be marked for deleting.
void SpacePlane::leaveField(WarField *w){
	for(int i = 0; i < 3; i++) if(this->pf[i]){
		this->pf[i]->immobilize();
		this->pf[i] = NULL;
	}
	st::leaveField(w);
}

bool SpacePlane::command(EntityCommand *com){
	if(DockToCommand *dtc = InterpretCommand<DockToCommand>(com)){
		if(ai && !ai->unlink(this))
			return false;
		ai = new DockAI(this, dtc->deste);
		return true;
	}
	else if(InterpretCommand<DockCommand>(com)){
		if(ai && !ai->unlink(this))
			return false;
		ai = new DockAI(this);
		return true;
	}
	else return st::command(com);
}

bool SpacePlane::dock(Docker *d){
	d->e->command(&TransportPeopleCommand(people));
	return true;
}

bool SpacePlane::undock(Docker *d){
	task = sship_undock;
	mother = d;
	d->e->command(&TransportPeopleCommand(-people));
	return true;
}

double SpacePlane::getMaxHealth()const{return maxHealthValue;}

#ifdef DEDICATED
void SpacePlane::draw(WarDraw *wd){}
void SpacePlane::drawtra(wardraw_t *wd){}
void SpacePlane::drawOverlay(wardraw_t *){}
Entity::Props SpacePlane::props()const{return Props();}
#endif


IMPLEMENT_COMMAND(TransportPeopleCommand, "TransportPeople")

TransportPeopleCommand::TransportPeopleCommand(HSQUIRRELVM v, Entity &e){
	int argc = sq_gettop(v);
	if(argc < 3)
		throw SQFArgumentError();
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 3, &i)))
		throw SQFArgumentError();
	people = i;
}
