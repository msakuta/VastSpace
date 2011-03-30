#ifndef CONTAINERHEAD_H
#define CONTAINERHEAD_H

#include "Frigate.h"
#include "EntityCommand.h"


struct EntityAI{
	virtual Vec3d dest(Entity *) = 0;
	virtual bool control(Entity *, double dt) = 0;
};


class Island3;

/// \brief A civilian ship transporting containers.
///
/// This ship routinely transports goods such as fuel, air, meal, etc in the background to add
/// the game a bit of taste of space era.
///
/// The ship can have variable number of containers. Random number and types of containers are generated
/// for variation.
/// Participating containers are connected linearly, sandwitched by head and tail module.
/// The head module is merely a terminator, while the tail module has cockpit and thrust engines.
///
/// The base class is Frigate, which is usually for military ships.
class ContainerHead : public Frigate{
public:
	typedef Frigate st;
	static const int maxcontainers = 6;
	enum ContainerType{gascontainer, hexcontainer, Num_ContainerType};
	ContainerType containers[maxcontainers];
	int ncontainers; ///< Count of containers connected.
protected:
	EntityAI *ai;
	float undocktime;
	Entity *docksite;
	Entity *leavesite;
	struct tent3d_fpol *pf[3]; ///< Trailing smoke
	static const double sufscale;
public:
	ContainerHead(){init();}
	ContainerHead(WarField *w);
	ContainerHead(Entity *docksite);
	void init();
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid, entityid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void anim(double);
	virtual void postframe();
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void enterField(WarField *);
	virtual void leaveField(WarField *);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual double maxhealth()const;
	virtual Props props()const;
	virtual bool command(EntityCommand *);
	virtual bool undock(Docker*);
	virtual void post_warp();
	static Entity *create(WarField *w, Builder *);
	btRigidBody *get_bbody(){return bbody;}
	enum sship_task_ch{sship_dockqueque = num_sship_task};
};


struct TransportResourceCommand : EntityCommand{
	typedef EntityCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	TransportResourceCommand(HSQUIRRELVM v, Entity &e);
	TransportResourceCommand(int gases, int solids) : gases(gases), solids(solids){}
	int gases;
	int solids;
};



struct DockToCommand : DockCommand{
	typedef DockCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	DockToCommand(Entity *e) : deste(e){}
	DockToCommand(HSQUIRRELVM v, Entity &e);
	Entity *deste;
};


struct TransportAI : EntityAI{
	enum Phase{Undock, Avoid, Warp, WarpWait, Dockque2, Dockque, Dock, num_Phase} phase;
	Entity *docksite;
	Entity *leavesite;
	TransportAI(Entity *ch, Entity *leavesite) : phase(Undock), docksite(NULL), leavesite(leavesite){
		std::vector<Entity *> set;
		findIsland3(ch->w->cs->findcspath("/"), set);
		if(set.size()){
			int i = RandomSequence((unsigned long)this + (unsigned long)(ch->w->war_time() / .0001)).next() % set.size();
			docksite = set[i];
		}
	}
	Vec3d dest(Entity *ch){
		Vec3d yhat = leavesite->rot.trans(Vec3d(0,1,0));
		return yhat * (-16. - 3.25 - 1.5) + leavesite->pos;
	}
	bool control(Entity *ch, double dt){
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

			case Dockque2:
			case Dockque:
			{
				Vec3d yhat = docksite->rot.trans(Vec3d(0,1,0));
				Vec3d target = yhat * (phase == Dockque2 || phase == Dockque ? -16. - 3.25 - 1.5 : -16. - 3.25) + docksite->pos;
				Vec3d delta = target - ch->pos;
				Vec3d parallel = yhat * delta.sp(yhat);
				Vec3d lateral = delta - parallel;
				if(phase == Dockque2 && delta.sp(yhat) < 0)
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
				ch->command(&DockToCommand(docksite));
				return true;
			}
		}
		return false;
	}

	void findIsland3(CoordSys *root, std::vector<Entity *> &ret)const{
		for(CoordSys *cs = root->children; cs; cs = cs->next){
			findIsland3(cs, ret);
		}
		if(root->w) for(Entity *e = root->w->entlist(); e; e = e->next){
			if(!strcmp(e->classname(), "Island3Entity") && e != leavesite){
				ret.push_back(e);
			}
		}
	}
};


#endif
