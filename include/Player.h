#ifndef PLAYER_H
#define PLAYER_H
#include "serial.h"
#include "Entity.h"
//#include <cstring>
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/mat4.h>
#include <set>
#ifdef _WIN32
#include <windows.h>
#endif
#include <squirrel.h>

/** \file
 * \brief Definition of the Player.
 *
 * The player is modeled as a class object in C++.
 * This file defines the Player class and camera related classes.
 */

#define FEETD 0.001

struct input_t;
struct tank;
struct astrobj;
struct coordsys;
class Entity;
class CoordSys;
class Viewer;
class GLWstateButton;
class ClientApplication;

/** \brief The Teleport desination record.
 *
 * Partially obsolete. It may be replaced with Squirrel bookmarks.
 */
struct EXPORT teleport{
	teleport(CoordSys *cs = NULL, const char *name = NULL, int flags = 0, const Vec3d &pos = Vec3d(0,0,0));
	~teleport();
	void init(CoordSys *cs, const char *name, int flags, const Vec3d &pos);

	/// Serialization for save games. Not really necessary to transmit between server and client.
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &sc);

	CoordSys *cs; ///< CoordSys object of destination local space.
	gltestp::dstring name; ///< Name of this entry
	int flags;
	Vec3d pos; ///< Offset to the center of local CoordSys designating the destination.
};


/** \brief The Player in front of display.
 *
 * Conceptually, the camera and the player is separate entity, but they heavily depend on each other.
 *
 * If the game is multiplayer game over the network, there are multiple Player instances.
 */
class EXPORT Player : public EntityController, public Observable{
public:
	typedef EntityController st;

	/// Base class for camera controller classes.
	///
	/// It defines how the camera reacts to user inputs. There're multiple ways to navigate the camera
	/// in 3-D space, so we virtualize it with this class.
	class CameraController{
	public:
		Player &pl;
		CameraController(Player &a) : pl(a){}
		virtual void operator ()(const input_t &inputs, double dt);
		virtual Vec3d getpos()const;
		virtual void setpos(const Vec3d &apos){pl.pos = apos;}
		virtual Vec3d getvelo()const{return pl.velo;}
		virtual void setvelo(const Vec3d &avelo){pl.velo = avelo;}
		virtual Quatd getrot()const;
		virtual void setrot(const Quatd &arot){pl.rot = arot;}
		virtual void rotateLook(double dx, double dy);
	protected:
		Vec3d &refvelo(){return pl.velo;}
	};

	class FreelookMover;
protected:
	virtual bool control(Entity *, double dt);
	Vec3d getrawpos(CameraController*)const;
	Quatd getrawrot(CameraController*)const;
public:
	Player(Game * = NULL);
	~Player();

	static const unsigned classId;
protected:
	Vec3d pos;
	Vec3d velo;
	Vec3d accel;
	Quatd rot;
public:
	unsigned playerId; ///< Index of player list
	Vec3d cpos; ///< chase pos, used after chased object is destroyed to keep vision
	double rad; ///< Radius of bounding sphere
//	double flypower; // acceleration force
	double viewdist; ///< view distance from focused object
	double aviewdist; ///< actual viewdist approaching constantly to viewdist
	const CoordSys *cs;
	CameraController *mover; ///< virtual mover function
	CameraController *nextmover; ///< next mover function, interpolate with mover at factor of blendmover to smoothly switch modes
	float blendmover; ///< Blending factor of mover and nextmover.
	WeakPtr<Entity> chase;
	WeakPtr<Entity> controlled, lastchase; ///< Various entity lists
	int chasecamera; ///< Camera ID of chased object. Multiple cameras can be mounted on a vehicle for having fun!
	int detail;
	int mousex, mousey;
	int race;
	bool r_move_path;
	bool r_attack_path;
	bool r_overlay;
	double fov; ///< Field of view value, in cosine angle
	double gametime; ///< global time
	double velolen; ///< trivial; performance keeper
	double height; ///< trivial; indicates air pressure surrounding the player
	typedef std::set<const Entity*> ChaseSet; ///< The type to hold set of Entities for chasing camera.
	std::set<const Entity*> chases; ///< Chased group of Entities. viewing volume shows all of them.
	typedef std::set<Entity*> SelectSet; ///< The type that holds selected list of Entities.
	std::set<Entity*> selected; ///< The list of selected Entities, moved from Entity-embedded linked list, which is meaningless in multiplayer game.
	int attackorder; ///< Issueing attacking order
	int forceattackorder;
	bool moveorder; ///< Issueing moving order
	bool move_lockz; ///< 3-D movement order
	double move_z;
	double move_t; ///< ray trace factor
	Vec3d move_org;
	Vec3d move_src;
	Vec3d move_hitpos;
	Vec3d move_viewpos;
	Mat4d move_rot;
	Mat4d move_model;
	Mat4d move_proj;
	Mat4d move_trans;

	void free(); ///< Frees internal memories but keep the object memory
	Vec3d getpos()const;
	void setpos(const Vec3d &apos){mover->setpos(apos);}
	Vec3d getvelo()const{return mover->getvelo();}
	void setvelo(const Vec3d &apos){mover->setvelo(apos);}
	Quatd getrot()const;
	void setrot(const Quatd &arot){mover->setrot(arot);}
	const CoordSys *getcs()const{return cs;}
	const char *classname()const; ///< returned string storage must be static
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &usc);
	void anim(double dt);
	void transit_cs(CoordSys *cs); ///< Explicitly change current CoordSys, keeping position, velocity and rotation.
	bool unlink(Observable *);
	void rotateLook(double dx, double dy);
	FreelookMover *const freelook;
	CameraController *const cockpitview;
	CameraController *const tactical;
	void draw(Viewer *wd);
	void drawtra(Viewer *wd);
	void drawindics(Viewer *vw);
#ifdef _WIN32
	void mousemove(HWND hWnd, int deltax, int deltay, WPARAM wParam, LPARAM lParam);
#endif
	/// Quit controlling an Entity.
	void uncontrol(){
		if(controlled && controlled->controller == this)
			controlled->controller = NULL;
		controlled = NULL;
	}
	Quatd orientation()const;
	const std::vector<teleport> &getTplist()const{return tplist;}

	static float camera_mode_switch_time;
	static int g_overlay; ///< Overlay display level
	static void cmdInit(ClientApplication &);
	static int cmd_mover(int argc, char *argv[], void *pv);
	static int cmd_teleport(int argc, char *argv[], void *pv);
	static int cmd_moveorder(int argc, char *argv[], void *pv);
	static int cmd_control(int argc, char *argv[], void *pv); ///< Switch control of selected Entity
	teleport *findTeleport(const char *, int flags = ~0); ///< returns teleport node found
	teleport *addTeleport(); ///< returns allocated uninitialized struct
	typedef unsigned teleport_iterator;
	teleport_iterator beginTeleport();
	teleport *getTeleport(teleport_iterator);
	teleport_iterator endTeleport();

	/// Define class Player for Squirrel.
	static void sq_define(HSQUIRRELVM v);

	/// Creates a button to toggle control of selected Entity.
	static GLWstateButton *newControlButton(Game *game, const char *filename, const char *filename2, const char *tips);

	/// Creates a button to toggle move ordering mode.
	static GLWstateButton *newMoveOrderButton(Game *game, const char *filename, const char *filename2, const char *tips);

	/// Creates and pushes an Entity object to Squirrel stack.
	static void sq_pushobj(HSQUIRRELVM, Player *);

	/// Returns an Player object being pointed to by an object in Squirrel stack.
	static Player *sq_refobj(HSQUIRRELVM v, SQInteger idx = 1);

protected:
	std::vector<teleport> tplist;
private:
	static SQInteger sqf_players(HSQUIRRELVM v);
	static SQInteger sqf_select(HSQUIRRELVM v);
	static SQInteger sqf_tostring(HSQUIRRELVM v);

	static SQInteger sqf_get(HSQUIRRELVM v);
	static SQInteger sqf_set(HSQUIRRELVM v);

	static SQInteger sqf_getpos(HSQUIRRELVM v);
	static SQInteger sqf_setpos(HSQUIRRELVM v);

	static SQInteger sqf_getrot(HSQUIRRELVM v);
	static SQInteger sqf_setrot(HSQUIRRELVM v);

	static SQInteger sqf_setmover(HSQUIRRELVM v);
	static SQInteger sqf_getmover(HSQUIRRELVM v);
//	int gear; /* acceleration gear in ghost mode */
};

typedef class Player::FreelookMover : public Player::CameraController{
public:
	typedef Player::CameraController st;
	double flypower;
	int gear;
	Vec3d pos;
	FreelookMover(Player &a);
	void operator ()(const input_t &inputs, double dt);
	Quatd getrot()const;
	void setpos(const Vec3d &apos);
	Vec3d getpos()const;
	void setGear(int g);
	int getGear()const;
} FreelookMover;


//-----------------------------------------------------------------------------
//     Implementation
//-----------------------------------------------------------------------------

inline int FreelookMover::getGear()const{return gear;}

extern void capture_mouse();

#endif
