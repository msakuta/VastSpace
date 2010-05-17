
/* Pseudo-prototype declaration of pre-defined classes and global variables
  by the application.

IMPORTANT NOTE:
  All properties marked by '***' returns value instead of reference when read.
  This means altering the contents of those properties does not take effect to
  the real entity.
  For example, writing like

    player.pos.x = 1.;

  won't take effect as intended.
  You must write like

    local localpos = player.pos;
    localpos.x = 1.;
    player.pos = localpos;

  to get it worked.
  This issue is not intuitive, but getting around this problem introduces very
  subtle problems too. It requires referenced object's lifetime management,
  that will be as tough as implementing memory management system of Squirrel.


class Vec3d{
	constructor(float x, float y, float z);
	float x, y, z;
	string _tostring();
	Vec3d _add(Vec3d);
	Vec3d _mul(Vec3d);
	float sp(Vec3d); // Scalar product or Dot product
	float vp(Vec3d); // Vector product or Cross product
}

class Quatd{
	constructor(float x, float y, float z, float w);
	float x, y, z, w;
	string _tostring();
	Quatd normin();
	Quatd _mul(Quatd);
	Vec3d trans(Vec3d); // Transforms the given vector, assuming this Quatd represents a rotation.
}

class CoordSys{
	string classname;
	Vec3d pos; // ***
	Quatd rot; // ***
	string name();
	CoordSys child();
	CoordSys next();
	string getpath();
	CoordSys findcspath();
	Entity addent(string classname, Vec3d pos);
	Entity entlist;
}

class Universe extends CoordSys{
	float timescale; // readonly
	float global_time; // readonly
}

::universe <- Universe();

class Entity{
	string classname;
	Vec3d pos; // ***
	Quatd rot; // ***
	int race;
	Entity next;
}

class Player{
	string classname;
	Vec3d pos; // ***
	Quatd rot; // ***
	CoordSys getcs();
	Entity chase;
}

::player <- Player();

*/

class Cvar{
	function _set(idx,val){
		::set_cvar(idx,val);
	}
	function _get(idx){
		return ::get_cvar(idx);
	}
}

print("Squirrel script initialized!"); 

cvar <- Cvar();
cvar.pause = "1";

local a = Vec3d(0,1,2), b = Vec3d(2,3,4);
print(a + " + " + b + " = " + (a + b));
print(a + ".sp(" + b + ") = " + a.sp(b));
print(a + ".vp(" + b + ") = " + a.vp(b));

function fact(n){
	if(1 < n)
		return n * fact(n-1);
	else
		return 1;
}

function deltaFormation(team, rot){
	local cs = player.cs;
	for(local i = 0; i < 11; i++){
		local e = cs.addent("Sceptor", Vec3d((i - 5) * 0.2, 1.,
			(team * 2 - 1) * (2 -(i <= 5 ? i * 0.1 : 1.0 - i * 0.1))));
		e.race = team;
		e.rot = rot;
//		print(e.classname + ": " + e.race + ", " + e.pos);
	}
}

function ae(){
	deltaFormation(0, Quatd(0,1,0,0));
	deltaFormation(1, Quatd(0,0,0,1));
}

ae();

function printtree(cs){
	local child;
	for(child = cs.child(); child != null; child = child.next()){
		print(child.getpath());
		printtree(child);
	}
}

function foreachents(cs, proc){
	local e;
	for(e = cs.entlist; e != null; e = e.next)
		proc(e);
}

function countents(team){
	local a = { ents = 0, team = team };
	foreachents(player.cs,
		function(e):(a){ if(e.race == a.team) a.ents++; });
	return a.ents;
}

local p = player.pos;
p.x = p.y = p.z = 1;
player.pos = p;
print("x = " + player.pos.x);
print("y = " + player.pos.y);
print("z = " + player.pos.z);
print("qx = " + player.rot.x);
print("qy = " + player.rot.y);
print("qz = " + player.rot.z);
print("qw = " + player.rot.w);

showdt <- false;
framecount <- 0;
checktime <- 0;
autochase <- true;
deaths <- {};

rotation <- Quatd(0,0,0,1);

function frameproc(dt){
	framecount++;
	local global_time = universe.global_time;

	if(showdt)
		print("DT = " + dt + ", FPS = " + (1. / dt) + ", FC = " + framecount);

	if(autochase && player.chase == null){
//		rotation = (rotation * Quatd(sin(dt/2.), 0., 0., cos(dt/2.))).normin();
//		player.pos = rotation.trans(Vec3d(0,0,2));
//		player.rot = rotation;
//		local phase = (global_time - PI/2.);
//		player.rot = Quatd(0., sin(phase/2.), 0., cos(phase/2.));
		foreachents(player.cs, function(e):(player){ player.chase = e; });
	}

	local currenttime = universe.global_time;

	if(checktime + 1. < currenttime){
		checktime = currenttime;
		local racec = [countents(0), countents(1)];

		print("time " + currenttime + ": " + racec[0] + ", " + racec[1]);

		local i;
		for(i = 0; i < 2; i++){
			if(racec[i] < 5)
				deltaFormation(i, i == 0 ? Quatd(0, 0, 0, 1) : Quatd(0, 1, 0, 0));
		}

		foreach(key,value in deaths) foreach(key1,value1 in value)
			print("[team" + key + "][" + key1 + "] " + value1);
	}
}

function hook_delete_Entity(e){
//	if(e.ptr == null)
//		return;
	if(!(e.race in deaths))
		deaths[e.race] <- {};
	if(!(e.classname in deaths[e.race]))
		deaths[e.race][e.classname] <- 0;
	deaths[e.race][e.classname]++;
/*	if(!(e.classname in deaths))
		deaths[e.classname] <- 0;
	deaths[e.classname]++;*/
}

/*
foreachents(player.cs,
	function(e){ print(e.classname() + ", " + e.pos + ", " + e.race); });
*/

