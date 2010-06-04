
/* Pseudo-prototype declaration of pre-defined classes and global variables
  by the application.



// Arithmetic vector with internal element type of double.
// Since Squirrel only supports float, you cannot directly access to
// the raw elements, but can operate them with native methods that cause
// least arithmetic errors.
class Vec3d{
	constructor(float x, float y, float z);
	float x, y, z;
	string _tostring();
	Vec3d _add(Vec3d);
	Vec3d _mul(Vec3d);
	float sp(Vec3d); // Scalar product or Dot product
	float vp(Vec3d); // Vector product or Cross product
}

// Quaternion with element type of double.
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
	Vec3d getpos();
	void setpos(Vec3d);
	Quatd getrot();
	void setpos(Quatd);
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
	Vec3d getpos();
	void setpos(Vec3d);
	Vec3d getvelo();
	void setvelo(Vec3d);
	Quatd getrot();
	void setrot(Quatd);
	void command(string, ...);
	int race;
	Entity next;
}

class Player{
	string classname;
	Vec3d getpos();
	void setpos(Vec3d);
	Quatd getrot();
	void setrot(Quatd);
	CoordSys getcs();
	Entity chase;
}

::player <- Player();

// Registers a function as a console command. You can manually add entryies to global table 'console_commands' too.
void register_console_command(string name, function);




// The application will try to call the following functions occasionary.
// Define them in this file in order to respond such events.

// Called just after the Universe is initialized.
// Please note that when this file is interpreted, the universe is not initialized,
// so you must defer operation on the universe using this callback.
void init_Universe();

// Called each frame.
void frameproc(double frametime);

// Called when an Entity is about to be deleted.
void hook_delete_Entity(Entity e);

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

/*
local a = Vec3d(0,1,2), b = Vec3d(2,3,4);
print(a + " + " + b + " = " + (a + b));
print(a + ".sp(" + b + ") = " + a.sp(b));
print(a + ".vp(" + b + ") = " + a.vp(b));
*/

function fact(n){
	if(1 < n)
		return n * fact(n-1);
	else
		return 1;
}

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


function deltaFormation(classname, team, rot, offset, spacing, count){
	local cs = player.cs;
	for(local i = 1; i < count + 1; i++){
		local epos = Vec3d(
			(i % 2 * 2 - 1) * (i / 2) * spacing, 0.,
			(team * 2 - 1) * (i / 2 * spacing));
		local e = cs.addent(classname, rot.trans(epos) + offset);
		e.race = team;
		e.setrot(rot);
		e.command("SetAggressive");
//		print(e.classname + ": " + e.race + ", " + e.pos);
	}
}

register_console_command("coordsys", function(...){
	if(vargc == 0){
		print("identity: " + player.cs.name());
		print("path: " + player.cs.getpath());
		print("formal name: " + player.cs.name());
		return 0;
	}
	local cs = universe.findcspath(vargv[0]);
	if(cs != null && player.cs != cs){
/*		player.setrot(player.getrot() * cs.getrot());
		player.setpos(player.getpos() + cs.getpos());
		player.setvelo(player.getvelo() + cs.getvelo());*/
		player.cs = cs;
		print("CoordSys changed to " + cs.getpath());
	}
	else
		print("Specified coordinate system is not existing or currently selected");
});

register_console_command("position", function(...){
	if(vargc < 3){
		print(player.getpos());
		return;
	}
	player.setpos(Vec3d(vargv[0], vargv[1], vargc[2]));
});

register_console_command("velocity", function(...){
	if(vargc < 3){
		print(player.getvelo());
		return;
	}
	player.setvelo(Vec3d(x, y, z));
});

register_console_command("halt", function(...){
	local e = player.selected;
	for(; e != null; e = e.selectnext)
		e.command("Halt");
});

register_console_command("moveto", function(...){
	if(vargc < 3){
		print("Usage: moveto x y z");
		return;
	}
	local e = player.selected;
	local pos = Vec3d(vargv[0], vargv[1], vargv[2]);
	print(vargv[0] + " " + vargv[1] + " " + vargv[2] + " " + pos);
	for(; e != null; e = e.selectnext)
		e.command("Move", pos);
});

register_console_command("dock", function(...){
	local e = player.selected;
	for(; e != null; e = e.selectnext)
		e.command("Dock");
});

register_console_command("parade", function(...){
	local e = player.selected;
	for(; e != null; e = e.selectnext)
		e.command("Parade");
});

register_console_command("sqcmdlist", function(...){
	foreach(name,proc in console_commands)
		print(name);
	print(console_commands.len() + " commands listed");
});


function ae(){
//	deltaFormation("Assault", 0, Quatd(0,1,0,0));
//	deltaFormation("Assault", 1, Quatd(0,0,0,1), Vec3d(0, 1.9, 0), 0.2);
//	player.cs.addent("Assault", Vec3d(-1, 0,0));
	deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0, 0.1, -0.2), 0.1, 3);
	deltaFormation("Sceptor", 1, Quatd(0,1,0,0), Vec3d(0, 0.1, 1.2), 0.1, 3);
//	deltaFormation("Destroyer", 1, Quatd(0,0,0,1), Vec3d(0, 1.1, -0.2), 0.3);
}

function ass(){
	deltaFormation("Assault", 0, Quatd(0,0,0,1), Vec3d(0,0.1,-0.1), 0.3, 3);
}

function des(){
	deltaFormation("Destroyer", 0, Quatd(0,0,0,1), Vec3d(0,0.1,-0.1), 0.3, 3);
}

function att(){
	deltaFormation("Attacker", 0, Quatd(0,0,0,1), Vec3d(0,0.1,-0.8), 0.5, 3);
}
function sce(){
	deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0,0.03,-0.8), 0.5, 3);
}

function init_Universe(){
//des();
att();
sce();
deltaFormation("Destroyer", 1, Quatd(0,1,0,0), Vec3d(0,0.1,-2.1), 0.3, 3);

player.setpos(Vec3d(0.0, 0.2, 1.5));

}

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
//		player.setpos(rotation.trans(Vec3d(0,0,2)));
//		player.setrot(rotation);
//		local phase = (global_time - PI/2.);
//		player.rot = Quatd(0., sin(phase/2.), 0., cos(phase/2.));
//		foreachents(player.cs, function(e):(player){ player.chase = e; });
	}

	local currenttime = universe.global_time;

	if(false && checktime + 1. < currenttime){
		checktime = currenttime;
		local racec = [countents(0), countents(1)];

		print("time " + currenttime + ": " + racec[0] + ", " + racec[1]);

		if(racec[0] < 5)
			deltaFormation("Sceptor", 0, Quatd(0, 0, 0, 1)
				, Vec3d(0, 0.1,  0.5), 0.1, 15);
		if(racec[1] < 3)
			deltaFormation("Assault", 1, Quatd(0, 1, 0, 0)
				, Vec3d(0, 0.1, -0.5), 0.1, 3);

		foreach(key,value in deaths) foreach(key1,value1 in value)
			print("[team" + key + "][" + key1 + "] " + value1);
	}
}

function hook_delete_Entity(e){
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

