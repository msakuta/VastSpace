/** \file
 * \brief Implementation of ContainerHead.
 */
#include "ContainerHead.h"
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



double ContainerHead::modelScale = .0002;
double ContainerHead::defaultMass = 2e7;
double ContainerHead::containerMass = 1e7;
double ContainerHead::maxHealthValue = 15000.;
int ContainerHead::maxcontainers = 6;
double ContainerHead::containerSize = 300 * modelScale;
std::vector<Vec3d> ContainerHead::engines;
GLuint ContainerHead::overlayDisp = 0;


bool EntityAI::unlink(Entity *){
	// Do nothing and allow unlinking by default
	return true;
}



ContainerHead::ContainerHead(WarField *aw) : st(aw), docksite(NULL), leavesite(NULL), ai(NULL){
	init();
}

ContainerHead::ContainerHead(Entity *docksite) : st(docksite->w), docksite(NULL), leavesite(docksite), ai(NULL){
	init();
	ai = new TransportAI(this, leavesite);
}

void ContainerHead::init(){

	static bool initialized = false;
	if(!initialized){
		sq_init(modPath() << _SC("models/ContainerHead.nut"),
			ModelScaleProcess(modelScale) <<=
			MassProcess(defaultMass) <<=
			SingleDoubleProcess(containerMass, "containerMass") <<=
			SingleDoubleProcess(maxHealthValue, "maxhealth", false) <<=
			IntProcess(maxcontainers, "maxContainerCount", false) <<=
			SingleDoubleProcess(containerSize, "containerSize", false) <<=
			Vec3dListProcess(engines, _SC("engines")) <<=
			DrawOverlayProcess(overlayDisp)
			);
		initialized = true;
	}

	RandomSequence rs((unsigned long)this);
	ncontainers = rs.next() % (maxcontainers - 1) + 1;
	containers.resize(ncontainers);
	for(int i = 0; i < ncontainers; i++)
		containers[i] = ContainerType(rs.next() % Num_ContainerType);
	undocktime = 0.f;
	health = getMaxHealth();
	mass = defaultMass + containerMass * ncontainers;

	pf.resize(engines.size());
	for(int i = 0; i < pf.size(); i++)
		pf[i] = NULL;

	if(!w)
		return;
}

ContainerHead::~ContainerHead(){
	if(ai){
		ai->unlink(this);
		ai = NULL;
	}
//	delete ai;
}

const char *ContainerHead::idname()const{
	return "ContainerHead";
}

const char *ContainerHead::classname()const{
	return "ContainerHead";
}

const unsigned ContainerHead::classid = registerClass("ContainerHead", Conster<ContainerHead>);
Entity::EntityRegister<ContainerHead> ContainerHead::entityRegister("ContainerHead");

void ContainerHead::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << ai;
	sc.o << leavesite;
	sc.o << docksite;
	sc.o << undocktime;
	sc.o << ncontainers;
	for(int i = 0; i < ncontainers; i++)
		sc.o << containers[i];
}

void ContainerHead::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> ai;
	sc.i >> leavesite;
	sc.i >> docksite;
	sc.i >> undocktime;
	sc.i >> ncontainers;
	for(int i = 0; i < ncontainers; i++)
		sc.i >> (int&)containers[i];

	// Re-create temporary entities if flying in a WarSpace. If environment is a WarField, don't restore.
	WarSpace *ws;
	if(w && (ws = (WarSpace*)w)){
		for(int i = 0; i < pf.size(); i++)
			pf[i] = ws->tepl->addTefpolMovable(this->pos, this->velo, avec3_000, &cs_orangeburn, TEP3_THICKEST | TEP3_ROUGH, cs_orangeburn.t);
	}
	else{
		for(int i = 0; i < pf.size(); i++)
			pf[i] = NULL;
	}
}

void ContainerHead::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	st::dive(sc, method);
	if(ai)
		ai->dive(sc, method);
}


const char *ContainerHead::dispname()const{
	return "Container Ship";
}

void ContainerHead::anim(double dt){
	try{
	WarSpace *ws = *w;
	if(!ws){
		st::anim(dt);
		return;
	}
	buildBody();

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
			if(!leavesite || leavesite->w != w || undocktime < 0.){
				inputs.press &= ~PL_W;
				task = sship_idle;
			}
			else{
				inputs.press |= PL_W;
				undocktime -= dt;
			}
		}
		else if(task == sship_dockqueque || task == sship_dockque || task == sship_dock){
			if(docksite == NULL){
				std::vector<Entity*> set;
				for(WarField::EntityList::iterator it = w->entlist().begin(); it != w->entlist().end(); it++) if(*it){
					Entity *e = *it;
					if(!strcmp(e->classname(), "Island3Entity") && e != leavesite){
						set.push_back(e);
					}
				}
				if(set.size()){
					int i = RandomSequence((unsigned long)this + (unsigned long)(w->war_time() / .0001)).next() % set.size();
					docksite = set[i];
				}
			}
			if(docksite && !this->warping){
				if(docksite->w->cs != w->cs){
					WarpCommand com;
					com.destcs = docksite->w->cs;
					Vec3d yhat = docksite->rot.trans(Vec3d(0,1,0));
					com.destpos = docksite->pos + yhat * (-16. - 3.25 - 1.5 - 1.);
					Vec3d target = com.destpos;
					Vec3d delta = target - com.destcs->tocs(pos, w->cs);
					Vec3d parallel = yhat * delta.sp(yhat);
					Vec3d lateral = delta - parallel;
					if(task == sship_dockqueque && delta.sp(yhat) < 0)
						com.destpos += lateral.normin() * -10.;
					command(&com);
				}
				else{
					Vec3d yhat = docksite->rot.trans(Vec3d(0,1,0));
					Vec3d target = yhat * (task == sship_dockque || task == sship_dockqueque ? -16. - 3.25 - 1.5 : -16. - 3.25) + docksite->pos;
					Vec3d delta = target - pos;
					Vec3d parallel = yhat * delta.sp(yhat);
					Vec3d lateral = delta - parallel;
					if(task == sship_dockqueque && delta.sp(yhat) < 0)
						target += lateral.normin() * -5.;
					steerArrival(dt, target, docksite->velo, 1. / 10., .001);
					if((target - pos).slen() < .2 * .2){
						if(task == sship_dockqueque)
							task = sship_dockque;
						else if(task == sship_dockque)
							task = sship_dock;
						else{
							this->w = NULL;
							for(int i = 0; i < ncontainers; i++)
								docksite->command(&TransportResourceCommand(containers[i] == gascontainer, containers[i] == hexcontainer));
							return;
						}
					}
				}
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
	for(int i = 0; i < pf.size(); i++) if(pf[i]){
		pf[i]->move(mat.vp3(engines[i] + Vec3d(0, 0, ncontainers * containerSize / 2)), avec3_000, cs_orangeburn.t, !(inputs.press & PL_W));
	}


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
	catch(...){
		std::cerr << __FILE__"(%d) Exception ?\n" << __LINE__;
	}
}

void ContainerHead::postframe(){
	if((task == sship_dock || task == sship_dockque) && docksite && docksite->w != w)
		docksite = NULL;
	if((task == sship_undock || task == sship_undockque) && leavesite && leavesite->w != w)
		leavesite = NULL;
	st::postframe();
}

void ContainerHead::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	pos = this->rot.trans(Vec3d(0, 120, 150 * ncontainers + 50) * modelScale) + this->pos;
	rot = this->rot;
}

void ContainerHead::enterField(WarField *target){
	WarSpace *ws = *target;
	if(ws && ws->bdw){
		if(!buildBody())
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

bool ContainerHead::buildBody(){
	if(!bbody){
		static std::vector<btCompoundShape*> shapes = std::vector<btCompoundShape*>(maxcontainers);
		btCompoundShape *&shape = shapes[ncontainers];
		if(!shape){
			shape = new btCompoundShape();
			Vec3d sc = Vec3d(100, 100, 250 + 150 * ncontainers) * modelScale;
			const Quatd rot = quat_u;
			const Vec3d pos = Vec3d(0,0,0);
			btBoxShape *box = new btBoxShape(btvc(sc));
			btTransform trans = btTransform(btqc(rot), btvc(pos));
			shape->addChildShape(trans, box);
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
//		rbInfo.m_linearDamping = .5;
//		rbInfo.m_angularDamping = .25;
		bbody = new btRigidBody(rbInfo);

		// If bbody is not initialized but surrounding dynamics world is initialized, assume it's because
		// the object is just loaded from a save file.
		// Other means (creation or transpoting) always use enterField method, where adding is already done.
		WarSpace *ws = w ? static_cast<WarSpace*>(*w) : NULL;
		if(ws){
			ws->bdw->addRigidBody(bbody, 1, ~2);
			setPosition(&pos, &rot, &this->velo, &omg);
			return true;
		}
	}
	return false;
}

/// if we are transitting WarField or being destroyed, trailing tefpols should be marked for deleting.
void ContainerHead::leaveField(WarField *w){
	for(int i = 0; i < pf.size(); i++) if(this->pf[i]){
		this->pf[i]->immobilize();
		this->pf[i] = NULL;
	}
	st::leaveField(w);
}

bool ContainerHead::command(EntityCommand *com){
	if(DockToCommand *dtc = InterpretCommand<DockToCommand>(com)){
		task = sship_dockque;
		docksite = dtc->deste;
		return true;
	}
	else if(InterpretCommand<DockCommand>(com)){
		task = (sship_task)sship_dockqueque;
		docksite = NULL;
		return true;
	}
	else return st::command(com);
}

bool ContainerHead::dock(Docker *d){
	for(int i = 0; i < ncontainers; i++)
		d->e->command(&TransportResourceCommand(containers[i] == gascontainer, containers[i] == hexcontainer));
	return true;
}

bool ContainerHead::undock(Docker *d){
	task = sship_undock;
	mother = d;
	for(int i = 0; i < ncontainers; i++)
		d->e->command(&TransportResourceCommand(-(containers[i] == gascontainer), -(containers[i] == hexcontainer)));
	return true;
}

void ContainerHead::post_warp(){
	if(race != 0 && docksite)
		task = (sship_task)sship_dockqueque;
}

double ContainerHead::getMaxHealth()const{return maxHealthValue;}

#ifdef DEDICATED
void ContainerHead::draw(WarDraw *wd){}
void ContainerHead::drawtra(wardraw_t *wd){}
void ContainerHead::drawOverlay(wardraw_t *){}
Entity::Props ContainerHead::props()const{return Props();}
#endif


IMPLEMENT_COMMAND(DockToCommand, "DockTo")
IMPLEMENT_COMMAND(TransportResourceCommand, "TransportResource")

DockToCommand::DockToCommand(HSQUIRRELVM v, Entity &e){
	int argc = sq_gettop(v);
	if(argc < 2)
		throw SQFArgumentError();
	Entity *pe = Entity::sq_refobj(v, 2);
	if(!pe)
		throw SQFArgumentError();
	deste = pe;
}

TransportResourceCommand::TransportResourceCommand(HSQUIRRELVM v, Entity &e){
	int argc = sq_gettop(v);
	if(argc < 4)
		throw SQFArgumentError();
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 3, &i)))
		throw SQFArgumentError();
	gases = i;
	if(SQ_FAILED(sq_getinteger(v, 4, &i)))
		throw SQFArgumentError();
	solids = i;
}


TransportAI::TransportAI(Entity *ch, Entity *leavesite) : EntityAI(game), phase(Undock), docksite(NULL), leavesite(leavesite), dockAI(ch){
	std::vector<Entity *> set;
	findIsland3(ch->w->cs->findcspath("/"), set);
	if(set.size()){
		int i = RandomSequence((unsigned long)this + (unsigned long)(ch->w->war_time() / .0001)).next() % set.size();
		docksite = set[i];
	}
}

void TransportAI::serialize(SerializeContext &sc){
	sc.o << phase;
	sc.o << docksite;
	sc.o << leavesite;
	dockAI.serialize(sc);
}

void TransportAI::unserialize(UnserializeContext &sc){
	sc.i >> (int&)phase;
	sc.i >> docksite;
	sc.i >> leavesite;
	dockAI.unserialize(sc);
}

const unsigned TransportAI::classid = registerClass("TransportAI", Conster<TransportAI>);
const char *TransportAI::classname()const{return "TransportAI";}



bool TransportAI::control(Entity *ch, double dt){
	if(!docksite)
		return true;
	switch(phase){
		case Undock:
		{
			Vec3d target = dest(ch);
			MoveCommand com;
			com.destpos = target;
			ch->command(&com);
//				ch->steerArrival(dt, target, Vec3d(0,0,0), 1. / 10., .001);
			if((target - ch->pos).slen() < .2 * .2){
				phase = Avoid;
//					ch->task = sship_idle;
			}
			break;
		}

		case Avoid:
		{
			CoordSys *destcs = docksite->w->cs;
			Vec3d yhat = docksite->rot.trans(Vec3d(0,1,0));
			Vec3d destpos = docksite->pos + yhat * (-16. - 3.25 - 1.5 - 1.);
			Vec3d delta = destpos - destcs->tocs(ch->pos, ch->w->cs);
			Vec3d parallel = yhat * delta.sp(yhat);
			Vec3d lateral = delta - parallel;
			if(delta.sp(destcs->tocs(leavesite->pos, leavesite->w->cs) - destcs->tocs(ch->pos, ch->w->cs)) < 0.)
				phase = Warp;
			else{
				yhat = leavesite->rot.trans(Vec3d(0,1,0));
				Vec3d target = ch->w->cs->tocs(target, destcs);
				Vec3d source = yhat * (-16. - 3.25 - 1.5) + leavesite->pos;
				delta = target - source;
				parallel = yhat * delta.sp(yhat);
				lateral = delta - parallel;
				double fpos = ch->pos.sp(yhat);
				double fsrc = source.sp(yhat);
				Vec3d destination = (fpos < fsrc ? source + parallel.norm() * (fpos - fsrc) : source) + lateral.norm() * 5.;
				if((destination - ch->pos).slen() < .2 * .2)
					phase = Warp;
				else{
					MoveCommand com;
					com.destpos = destination;
					ch->command(&com);
				}
			}
			break;
		}

		case Warp:
		{
			WarpCommand com;
			com.destcs = docksite->w->cs;
			Vec3d yhat = docksite->rot.trans(Vec3d(0,1,0));
			com.destpos = docksite->pos + yhat * (-16. - 3.25 - 1.5 - 1.);
			Vec3d target = com.destpos;
			Vec3d delta = target - com.destcs->tocs(ch->pos, ch->w->cs);
			Vec3d parallel = yhat * delta.sp(yhat);
			Vec3d lateral = delta - parallel;
			if(delta.sp(yhat) < 0)
				com.destpos += lateral.normin() * -10.;
			ch->command(&com);
			phase = WarpWait;
			break;
		}

		case WarpWait:
			if(!ch->toWarpable() || ch->toWarpable()->warping)
				phase = Dockque;
			break;

		case Dockque:
			dockAI = DockAI(ch, docksite);
			phase = Dock;
			// Fall through
		case Dock:
			return dockAI.control(ch, dt);
	}
	return false;
}

bool TransportAI::unlink(Entity *){
	delete this;
	return true;
}


Vec3d TransportAI::dest(Entity *ch){
	Vec3d yhat = leavesite->rot.trans(Vec3d(0,1,0));
	return yhat * (-16. - 3.25 - 1.5) + leavesite->pos;
}

void TransportAI::findIsland3(CoordSys *root, std::vector<Entity *> &ret)const{
	for(CoordSys *cs = root->children; cs; cs = cs->next){
		findIsland3(cs, ret);
	}
	if(root->w) for(WarField::EntityList::iterator it = root->w->entlist().begin(); it != root->w->entlist().end(); it++) if(*it){
		Entity *e = *it;
		if(!strcmp(e->classname(), "Island3Entity") && e != leavesite){
			ret.push_back(e);
		}
	}
}

DockAI::DockAI(Entity *ch, Entity *docksite) : EntityAI(ch->getGame()), phase(Dockque2), docksite(docksite){
	if(!docksite){
		std::vector<Entity*> set;
		for(WarField::EntityList::iterator it = ch->w->entlist().begin(); it != ch->w->entlist().end(); it++) if(*it){
			Entity *e = *it;
			if(!strcmp(e->classname(), "Island3Entity")){
				set.push_back(e);
			}
		}
		if(set.size()){
			int i = RandomSequence((unsigned long)this + (unsigned long)(ch->w->war_time() / .0001)).next() % set.size();
			this->docksite = set[i];
		}
	}
}

const unsigned DockAI::classid = registerClass("DockAI", Conster<DockAI>);
const char *DockAI::classname()const{return "DockAI";}

void DockAI::serialize(SerializeContext &sc){
	sc.o << phase;
	sc.o << docksite;
}

void DockAI::unserialize(UnserializeContext &sc){
	sc.i >> (int&)phase;
	sc.i >> docksite;
}

bool DockAI::control(Entity *ch, double dt){
	if(!docksite)
		return true;
	switch(phase){
		case Dockque2:
		case Dockque:
		{
			Vec3d yhat = docksite->rot.trans(Vec3d(0,1,0));
			Vec3d target = yhat * (phase == Dockque2 || phase == Dockque ? -16. - 3.25 - 1.5 : -16. - 3.25) + docksite->pos;
			Vec3d delta = target - ch->pos;
			Vec3d parallel = yhat * delta.sp(yhat);
			Vec3d lateral = delta - parallel;
			if(phase == Dockque2 && delta.sp(yhat) < 0 && sqrt(DBL_EPSILON) < lateral.slen())
				target += lateral.normin() * -5.;
			MoveCommand com;
			com.destpos = target;
			ch->command(&com);
			if((target - ch->pos).slen() < .2 * .2){
				if(phase == Dockque2)
					phase = Dockque;
				else if(phase == Dockque)
					phase = Dock;
			}
			break;
		}

		case Dock:
		{
			Vec3d yhat = docksite->rot.trans(Vec3d(0,1,0));
			Vec3d target = docksite->pos;
			Docker *docker = docksite->getDocker();
			if(docker)
				target = docker->getPortPos(ch);
			Vec3d delta = target - ch->pos;
			Vec3d parallel = yhat * delta.sp(yhat);
			Vec3d lateral = delta - parallel;
			MoveCommand com;
			com.destpos = target;
			ch->command(&com);
			if((target - ch->pos).slen() < .2 * .2){
				if(docker)
					docker->dock(ch);
				return true;
			}
			break;
		}
	}
	return false;
}

bool DockAI::unlink(Entity *e){
	delete this;
	return true;
}
