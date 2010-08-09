
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
	Entity dockedEntList; 
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

class GLwindow{
	string classname;
	int x;
	int y;
	int width;
	int height;
}

class GLwindowMenu extends GLwindow{
	void addItem(string title, string command);
	void close();
}

class GLwindowBigMenu extends GLwindowMenu{
}

class GLWbuttonMatrix extends GLwindow{
	constructor(int x, int y, int sx, int sy);
	void addButton(string command, string buttonimagefile, string tips);
	void addCvarToggleButton(string cvarname, string buttonimagefile, string pushedimagefile, string tips);
	void addMoveOrderButton(string buttonimagefile, string pushedimagefile, string tips);
}

int screenwidth();
int screenheight();




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

function foreachsubents(cs, proc){
	local cs1 = cs.child();
	for(; cs1 != null; cs1 = cs1.next())
		foreachsubents(cs1, proc);

	// Bottom up, with no reason
	foreachents(cs, proc);
}

function countents(cs, team, classname){
	local a = { ents = 0, team = team };
	foreachsubents(cs,
		function(e):(a,classname){ if(e.race == a.team && e.classname == classname) a.ents++; });
	return a.ents;
}

function foreachdockedents(docker, proc){
	local e;
	for(e = docker.dockedEntList; e != null; e = e.next)
		proc(e);
}

eng <- {
	command="Command"
	camera="Camera",
	["Entity List"]="Entity List",
	move="Move",
	System="System",
	["Follow Camera"]="Follow Selected Object with Camera"
}

jpn <- {
	command="コマンド",
	camera="カメラ",
	Dock="ドッキング",
	Undock="アンドッキング",
	["Entity List"]="実体リスト",
	move="移動",
	["Move order"]="移動命令",
	Halt="停止命令",
	System="システム",
	["Follow Camera"]="選択オブジェクトをカメラで追跡",
	["Switch Camera Mode"]="モード切替",
	["Reset Camera Rotation"]="回転初期化",
	["Eject Camera"]="脱出！",
	["Exit game"]="ゲーム終了",
	["Toggle Pause"]="ポーズ",
}

// Set default language to english
lang <- eng;

function tlate(id){
	if(id in lang)
		return lang[id];
	else
		return id;
}


function deltaFormation(classname, team, rot, offset, spacing, count, cs){
	for(local i = 1; i < count + 1; i++){
		local epos = Vec3d(
			(i % 2 * 2 - 1) * (i / 2) * spacing, 0.,
			(i / 2 * spacing));
		local e = cs.addent(classname, rot.trans(epos) + offset);
		e.race = team;
		foreachdockedents(e, function(e):(team){e.race = team;});
		e.setrot(rot);
		e.command("SetAggressive");
		e.command("Move", rot.trans(epos + Vec3d(0,0,-2)) + offset);
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

register_console_command("undock", function(...){
	local e = player.selected;
	for(; e != null; e = e.selectnext)
		e.command("RemainDockedCommand", false);
});

register_console_command("parade_formation", function(...){
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
	deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0, 0.1, -0.2), 0.1, 3, player.cs);
	deltaFormation("Sceptor", 1, Quatd(0,1,0,0), Vec3d(0, 0.1, 1.2), 0.1, 3, player.cs);
//	deltaFormation("Destroyer", 1, Quatd(0,0,0,1), Vec3d(0, 1.1, -0.2), 0.3);
}

function ass(){
	deltaFormation("Assault", 0, Quatd(0,0,0,1), Vec3d(0,0.1,-0.1), 0.3, 3, player.cs);
}

function des(){
	deltaFormation("Destroyer", 0, Quatd(0,0,0,1), Vec3d(0,0.1,-0.1), 0.3, 3, player.cs);
}

function att(){
	deltaFormation("Attacker", 0, Quatd(0,1,0,0), Vec3d(0,0.1,-0.8), 0.5, 3, player.cs);
}
function sce(){
	deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0,0.03,-0.8), 0.5, 3, player.cs);
}

register_console_command("ass", ass);
register_console_command("att", att);

mainmenu <- GLwindowBigMenu();

function loadmission(script){
	mainmenu.close();
	print("loading " + script);
	local exe = loadfile(script);
	if(exe == null){
		print("Failed to load file " + script);
	}
	else
		exe();
	return exe;
}

register_console_command("loadmission", loadmission);

//tutorial1 <- loadmission("scripts/tutorial1.nut");

function init_Universe(){
	//des();
	//att();
	//sce();
	//deltaFormation("Destroyer", 1, Quatd(0,1,0,0), Vec3d(0,0.1,-2.1), 0.3, 3);
//	foreach(value in lang)
//		print(value);

	player.setpos(Vec3d(0.0, 0.2, 5.5));
	player.setrot(Quatd(0., 1., 0., 0.));

	local scw = screenwidth();
	local sch = screenheight();

	mainmenu.title = "Select Mission";
	mainmenu.addItem("Tutorial 1", "loadmission \"scripts/tutorial1.nut\"");
	mainmenu.addItem("test", "loadmission \"scripts/eternalFight.nut\"");

	// Adjust window position to center of screen, after all menu items are added.
	mainmenu.x = scw / 2 - mainmenu.width / 2;
	mainmenu.y = sch / 2 - mainmenu.height / 2;
}

function initUI(){
	local scw = screenwidth();
	local sch = screenheight();

	local entlist = GLWentlist();
	entlist.title = tlate("Entity List");
	entlist.x = scw - 300;
	entlist.width = 300;
	entlist.y = 100;
	entlist.height = 200;
	entlist.closable = true;
	entlist.pinnable = true;
	entlist.pinned = true;

	local but = GLWbuttonMatrix(3, 3);
	but.title = tlate("command");
	but.x = 0;
	but.y = sch - but.height;
	but.addButton("halt", "textures/halt.png", tlate("Halt"));
	but.addButton("dock", "textures/dock.png", tlate("Dock"));
	but.addButton("undock", "textures/undock.png", tlate("Undock"));
	but.addMoveOrderButton("textures/move2.png", "textures/move.png", tlate("Move order"));
	but.pinned = true;

	local cambut = GLWbuttonMatrix(4, 1);
	cambut.title = tlate("camera");
	cambut.x = but.width;
	cambut.y = sch - cambut.height;
	cambut.addButton("chasecamera", "textures/focus.jpg", tlate("Follow Camera"));
	cambut.addButton("mover cycle", "textures/cammode.jpg", tlate("Switch Camera Mode"));
	cambut.addButton("originrotation", "textures/resetrot.jpg", tlate("Reset Camera Rotation"));
	cambut.addButton("eject", "textures/eject.png", tlate("Eject Camera"));
	cambut.pinned = true;

	local sysbut = GLWbuttonMatrix(3, 2, 32, 32);
	sysbut.title = tlate("System");
	sysbut.x = scw - sysbut.width;
	sysbut.y = sch - sysbut.height;
	sysbut.addButton("exit", "textures/exit.jpg", tlate("Exit game"));
	sysbut.addToggleButton("pause", "textures/pause.jpg", "textures/unpause.jpg", tlate("Toggle Pause"));
	sysbut.pinned = true;
}

showdt <- false;

