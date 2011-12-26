
/* Pseudo-prototype declaration of pre-defined classes and global variables
  by the application.
   Note that type of returned value and arguments are not specifiable in Squirrel language,
  but shown here to clarify semantics.



// Arithmetic vector with internal element type of double.
// Since Squirrel only supports float, you cannot directly access to
// the raw elements, but can operate them with native methods that cause
// least arithmetic errors.
class Vec3d{
	constructor(float x, float y, float z);
	float x, y, z; // They appear to be float, but internally double.
	string _tostring();
	Vec3d _add(Vec3d);
	Vec3d _sub(Vec4d);
	Vec3d _mul(float);
	Vec3d _unm();
	Vec3d norm(); // Normalize
	float sp(Vec3d); // Scalar product or Dot product
	Vec3d vp(Vec3d); // Vector product or Cross product
	float len(); // Length of vector
}

// Quaternion with element type of double.
class Quatd{
	constructor(float x, float y, float z, float w);
	float x, y, z, w; // They appear to be float, but internally double.
	string _tostring();
	Quatd norm(); // Normalize
	Quatd normin();
	Quatd _mul(Quatd);
	Quatd cnj(); // Conjugate
	Vec3d trans(Vec3d); // Transforms the given vector, assuming this Quatd represents a rotation.
	static Quatd direction(Vec3d); // Generates rotation towards given direction.
	static Quatd rotation(float angle, Vec3d axis); // Generate rotation with angle around axis.
}

class CoordSys{
	string classname;
	Vec3d getpos();
	void setpos(Vec3d);
	Vec3d getvelo(); // Linear velocity
	void setvelo(Vec3d);
	Quatd getrot(); // Rotation in quaternion
	void setrot(Quatd);
	Vec3d getomg(); // Angular velocity
	void setomg(Vec3d);
	string name();
	string[] extranames;
	CoordSys child();
	CoordSys next();
	string getpath();
	CoordSys findcspath();
	Entity addent(string classname, Vec3d pos);
	Entity entlist;
	Vec3d transPosition(Vec3d pos, CoordSys from, bool delta); // Converts position coordinates from given system to this system.
	Vec3d transVelocity(Vec3d velo, CoordSys from); // Converts velocity vector. Equivalent to transPosition(velo, from, true).
	Quatd transRotation(CoordSys from); // Converts rotation expressed in quaternion from given system to this system.
}

class Universe extends CoordSys{
	float timescale; // readonly
	float global_time; // readonly
}

::universe <- Universe();

class Astrobj extends CoordSys{
	float rad; // readonly
}

class TexSphere extends Astrobj{
	float oblateness;
}

class Star extends Astrobj{
	string spectral;
}

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
	Docker docker; // readonly, may be null
}

class Docker{
	alive; // readonly
	void addent(Entity);
}

class Player{
	string classname;
	Vec3d getpos();
	void setpos(Vec3d);
	Quatd getrot();
	void setrot(Quatd);
	void setmover(string movertype);
	string getmover();
	Entity chase;
	float viewdist;
	CoordSys cs;
}

::player <- Player();

// Registers a function as a console command. You can manually add entries to global table 'console_commands' too.
void register_console_command(string name, function);

class GLwindow{
	bool alive;
	string classname;
	int x;
	int y;
	int width;
	int height;
}

class GLWmenu extends GLwindow{
	void addItem(string title, string command);
	void close();
}

class GLWbigMenu extends GLwindowMenu{
}

class GLWbuttonMatrix extends GLwindow{
	constructor(int x, int y, int sx, int sy);
	void addButton(string command, string buttonimagefile, string tips);
	void addCvarToggleButton(string cvarname, string buttonimagefile, string pushedimagefile, string tips);
	void addMoveOrderButton(string buttonimagefile, string pushedimagefile, string tips);
}

class GLWmessage extends GLwindow{
	constructor(string message, float timer, string onDestroy);
}

int screenwidth();
int screenheight();

// An utility function that executes func and measures time spent by that.
// It uses high-resolution timer of the computer if available, so it's expected to be more precise
// than old-fashioned clock() method.
// Returns a table containing 2 members; time, represents execution time of func, and result,
// represents returned value of func.
table timemeas(function func);

// Class version of timemeas(). Does not require function form to the code being measured.
class TimeMeas{
	constructor(); // Start counting time.
	float lap(); // Returns seconds elapsed since this object is created.
}

// Registers a console command with name, defined by func.
// Variable-length arguments may be present as strings, use vargv.len() and vargv to interpret them.
void register_console_command(string name, function func);

/// Loads an external module, implemented as DLL or shared library.
/// \return Reference count of the module after loaded.
int loadModule(string path);

/// Decrements reference count of a module and possibly unload it if count is zero.
/// \return Reference count.
int unloadModule(string path);



// The application will try to call the following functions occasionary.
// Define them in this file in order to respond such events.

// Called just after the Universe is initialized.
// Please note that when this script file itself is interpreted, the universe is not yet initialized,
// so you must defer operation on the universe using this callback.
void init_Universe();

// Called each frame.
void frameproc(double frametime);

// Called when an Entity is about to be deleted.
void hook_delete_Entity(Entity e);

// Called everytime the system needs some string to be translated.
string translate(string source);

// The file name of the stellar file.
string stellar_file = "space.ssd";

*/

//local tm = TimeMeas();

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

function foreachselectedents(proc){
	local e;
	for(e = player.selected; e != null; e = e.selectnext)
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
		function(e)/*:(a,classname)*/{ if(e.race == a.team && e.classname == classname) a.ents++; });
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
	Move="Move",
	["Move order"]="Move order\nHold shift to set z direction",
	System="System",
	Control="Direct Control\n(Escape to exit)",
	["Follow Camera"]="Follow Selected Object with Camera",
	["Solarmap"]="Solarsystem browser",
}

jpn <- {
	command="コマンド",
	camera="カメラ",
	Dock="ドッキング",
	Undock="アンドッキング",
	["Entity List"]="実体リスト",
	Move="移動",
	["Move order"]="移動命令\nShiftでZ方向指定",
	Halt="停止命令",
	System="システム",
	Control="直接制御\n(Escで終了)",
	["Follow Camera"]="選択オブジェクトをカメラで追跡",
	["Switch Camera Mode"]="モード切替",
	["Reset Camera Rotation"]="回転初期化",
	["Eject Camera"]="脱出！",
	["Exit game"]="ゲーム終了",
	["Toggle Pause"]="ポーズ",
	["Solarmap"]="太陽系ブラウザ",
	["Attack order"]="攻撃命令",
	["Force Attack order"]="強制攻撃命令",
	["Tutorial"]="チュートリアル",
	["Proceed"]="次へ",
	["Restart"]="最初から",
	["End"]="終わる",
	Deploy="展開",
	Undeploy="展開解除",
	["Chase Camera"]="追跡カメラ",
	["Properties"]="属性",
	["Military Parade Formation"]="行進隊形",
	["Cloak"]="隠密",
	["Delta Formation"]="Δ隊形",
}

// Set default language to english
lang <- eng;

function translate(id){
	if(id in lang)
		return lang[id];
	else
		return id;
}

// Alias for scripts
tlate <- translate;

// load the module
{
	local dllpath = debugBuild() ?
		x64Build() ? "gltestdll.dll" : "..\\gltestplus\\Debug\\gltestdll.dll" :
		x64Build() ? "gltestdll.dll" : "gltestdll.dll";
//		x64Build() ? "x64\\Debug\\gltestdll.dll" : "..\\gltestplus\\Debug\\gltestdll.dll" :
//		x64Build() ? "x64\\Release\\gltestdll.dll" : "gltestdll.dll";
	local gltestdll = loadModule(dllpath);
	print("x64: " + x64Build() + ", \"" + dllpath + "\"l refc is " + gltestdll);
}

// load the module
{
	local dllpath = debugBuild() ?
		x64Build() ? "surface.dll" : "..\\gltestplus\\Debug\\surface.dll" :
		x64Build() ? "surface.dll" : "surface.dll";
//		x64Build() ? "x64\\Debug\\gltestdll.dll" : "..\\gltestplus\\Debug\\gltestdll.dll" :
//		x64Build() ? "x64\\Release\\gltestdll.dll" : "gltestdll.dll";
	local gltestdll = loadModule(dllpath);
	print("\"" + dllpath + "\"l refc is " + gltestdll);
}

// load the module
{
	local dllpath = debugBuild() ?
		x64Build() ? "gundam.dll" : "..\\gltestplus\\Debug\\gundam.dll" :
		x64Build() ? "gundam.dll" : "gundam.dll";
	local gltestdll = loadModule(dllpath);
	print("\"" + dllpath + "\"l refc is " + gltestdll);
}

// set stellar file
stellar_file = 1 || debugBuild() ? "space_debug.ssd" : "space.ssd";

function deltaFormation(classname, team, rot, offset, spacing, count, cs, proc){
	for(local i = 1; i < count + 1; i++){
		local epos = Vec3d(
			(i % 2 * 2 - 1) * (i / 2) * spacing, 0.,
			(i / 2 * spacing));
		local e = cs.addent(classname, rot.trans(epos) + offset);
		e.race = team;
//		foreachdockedents(e, function(e):(team){e.race = team;}); // nonsense since no docker initially has a Entity.
		e.setrot(rot);
		if(proc != null)
			proc(e);
//		print(e.classname + ": " + e.race + ", " + e.pos);
	}
}

function bool(a){
	return typeof(a) == "bool" ? a : typeof(a) == "integer" ? a != 0 : typeof(a) == "string" ? a == "true" : a != null;
}

register_console_command("coordsys", function(...){
	if(vargv.len() == 0){
		print("identity: " + player.cs.name());
		print("path: " + player.cs.getpath());
		print("formal name: " + player.cs.name());
		local en = player.cs.extranames;
		foreach(name in en)
			print("aka: " + name);
		return 0;
	}
	print("Arg[" + vargv.len() + "] " + vargv[0]);
	local cs = player.cs.findcspath(vargv[0]);
	local transit = true;
	if(1 < vargv.len()){
		if(bool(vargv[1]))
			transit = false;
		print("Transit " + bool(vargv[1]));
	}
	if(cs != null && cs.alive && player.cs != cs){
		if(transit){
			local newrot = player.getrot() * cs.transRotation(player.cs);
			local newpos = cs.transPosition(player.getpos(), player.cs);
			local newvelo = cs.transVelocity(player.getvelo(), player.cs);
			player.setrot(newrot);
			player.setpos(newpos);
			player.setvelo(newvelo);
		}
		player.cs = cs;
		print("CoordSys changed to " + cs.getpath());
	}
	else
		print("Specified coordinate system is not existing or currently selected");
});

register_console_command("position", function(...){
	if(vargv.len() < 3){
		print(player.getpos());
		return;
	}
	player.setpos(Vec3d(vargv[0], vargv[1], vargv[2]));
});

register_console_command("velocity", function(...){
	if(vargv.len() < 3){
		print(player.getvelo());
		return;
	}
	player.setvelo(Vec3d(vargv[0], vargv[1], vargv[2]));
});

register_console_command("halt", function(...){
	local e = player.selected;
	for(; e != null; e = e.selectnext){
		e.command("Halt");
		CMHalt(e);
	}
});

register_console_command("moveto", function(...){
	if(vargv.len() < 3){
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

// Unit group bindings
groups <- {}

register_console_command("setgroup", function(...){
	if(vargv.len() < 1){
		print("Usage: setgroup groupid");
		return;
	}
	local groupid = vargv[0];
	local set = [];
	local e = player.selected;
	for(; e != null; e = e.selectnext)
		set.insert(set.len(), e);
	groups[groupid] <- set;
});

register_console_command("recallgroup", function(...){
	if(vargv.len() < 1){
		print("Usage: recallgroup groupid");
		return;
	}
	local groupid = vargv[0];
	if(!(groupid in groups) || groups[groupid].len() == 0){
		print("group not bound: " + groupid);
		return;
	}
	player.select(groups[groupid]);
});

register_console_command("showgroup", function(...){
	if(vargv.len() == 0){
		foreach(key, group in groups){
			print("group " + key + " count: " + group.len());
		}
		return;
	}
	local groupid = vargv[0];
	if(!(groupid in groups) || groups[groupid].len() == 0){
		print("group not bound: " + groupid);
		return;
	}
	print("group " + groupid + " count: " + groups[groupid].len());
	foreach(e in groups[groupid])
		print("[" + e.classname + "]");
});

register_console_command("height", function(...){
	local earth = universe.findcspath("/sol/earth/Earth");
	if(earth != null){
		local lpos = player.cs.tocs(Vec3d(0,0,0), earth);
		print("height = " + ((lpos - player.getpos()).len() - earth.rad));
	}
});

register_console_command("cscript", function(){
	local ret = timemeas(function(){
		writeclosuretofile("scripts/init.dat", loadfile("scripts/init.nut"));
	});
	print("write time " + ret.time);
});

register_console_command("comp", function(){
	local ret = timemeas(function(){loadfile("scripts/init.nut");});
	print("compile time " + ret.time);
	ret = timemeas(function(){loadfile("scripts/init.dat");});
	print("load time " + ret.time);
});

register_console_command("oblate", function(){
	local earth = universe.findcspath("/sol/earth/Earth");
	if(earth && earth.alive){
		earth.oblateness = 0.001;
	}
});

register_console_command("disoblate", function(){
	local earth = universe.findcspath("/sol/earth/Earth");
	if(earth && earth.alive){
		earth.oblateness = 0.0;
	}
});

register_console_command("typecs", function(...){
	local cs;
	if(vargv.len() == 0)
		cs = player.cs;
	else{
		cs = player.cs.findcspath(vargv[0]);
	}

	if(cs != null && cs.alive){
		local root = getroottable();
		local cls = cs.getclass();
		foreach(name, value in root) if(value == cls)
			print(cs.name() + ": " + cs.classname + " " + name);
	}
});

register_console_command("matchcs", function(...){
	/// Class that pattern matches names and aliases of all systems.
	/// Note that this class is local to this function scope.
	/// Also note that bare function couldn't recursively call itself without
	/// namespace contaminated, so there is encapsulating class.
	local PatCall = class{
		names = [];
		function patcall(cs, pat){
			local en = cs.extranames;
			for(local i = 0; i < en.len(); i++) if(en[i].find(pat) != null)
				names.append([en[i], cs]);
			for(local cs2 = cs.child(); cs2 != null; cs2 = cs2.next())
				patcall(cs2, pat);
		}
	};
	local pat = "";
	if(vargv.len() == 0)
		pat = "";
	else
		pat = vargv[0];

	local patCall = PatCall();
	patCall.patcall(universe, pat);

	// Sort by name
	patCall.names.sort(function(a,b){
		if(a[0].slice(0, 4) == "HIP " && b[0].slice(0, 4) == "HIP "){
			local ai = a[0].slice(4).tointeger();
			local bi = b[0].slice(4).tointeger();
			return ai - bi;
		}
		return a[0] > b[0] ? 1 : a[0] < b[0] ? -1 : 0;
	});

	// Print matched name and its path in order of name.
	foreach(entry in patCall.names){
		local name = entry[0];
		local cs = entry[1];
		print("\"" + name + "\": " + cs.getpath());
		if(cs.classname == "Star")
			print("spectral: " + cs.spectral);
	}
});

register_console_command("locate", function(){
	local ReCall = class{
		function recall(cs){
			print(cs.getpath() + ": " + cs.classname);
			for(local cs2 = cs.child(); cs2 != null; cs2 = cs2.next())
				recall(cs2);
		}
	}
	ReCall().recall(universe);
});


class Bookmark{
	function cs(){return null;} // Pure virtual would be appropriate
	pos = Vec3d(0,0,0)
	rot = Quatd(0,0,0,1)
	function _tostring(){
		return path() + " " + pos + " " + rot;
	}
}

class BookmarkCoordSys extends Bookmark{
	src = null
	constructor(acs){
		src = acs;
	}
	function cs(){
		return src;
	}
	function path(){
		return src != null ? src.getpath() : "(null)";
	}
}

class BookmarkSymbolic extends Bookmark{
	m_path = "/"
	constructor(apath){
		m_path = apath;
	}
	function cs(){
		return ::player.cs.findcspath(m_path);
	}
	function path(){
		return m_path;
	}
}

bookmarks <- {
	["Earth surface"] = BookmarkSymbolic("/sol/earth/Earth/earths")
};

/// Recall a bookmark instantly.
register_console_command("jump_bookmark", function(...){
	if(vargv.len() < 1){
		print("Usage: jump_bookmark bookmark_name");
		return;
	}
	if(vargv[0] in bookmarks){
		local item = bookmarks[vargv[0]];
		local newcs = item.cs();
		if(newcs != null){
			player.cs = newcs;
			player.setpos(item.pos);
			player.setvelo(Vec3d(0,0,0));
			player.setrot(item.rot);
			player.chase = null;
		}
	}
});

/// Shows bookmarks window to let the player pick one.
register_console_command("bookmarks", function(){
	if(bookmarks == null || bookmarks.len() == 0)
		return;
	local menu = GLWmenu();
	local names = [];
	foreach(key, item in bookmarks)
		names.append([key, item]);

	// Sort bookmarks by name
	names.sort(function(a,b){return a[0] > b[0] ? 1 : a[0] < b[0] ? -1 : 0;});

	for(local i = 0; i < names.len(); i++){
		local entry = names[i];
		menu.addItem(entry[0], function()/*:(entry)*/{
			local item = entry[1];
			local newcs = item.cs();
			if(newcs != null){
				player.cs = newcs;
				player.setpos("pos" in item ? item.pos : Vec3d(0,0,0));
				player.setvelo(Vec3d(0,0,0));
				player.setrot(item.rot);
				player.chase = null;
			}
		});
	}
});

/// Adds a bookmark of given path.
function addBookmark(symbolic, argv){
	local argc = argv.len();
	if(argc <= 1){
		print("Usage: add_bookmark[_s] name path pos");
		print("  Append \"_s\" to make symbolic bookmark.");
		foreach(key, item in ::bookmarks){
			print("\"" + key + "\" " + (item instanceof BookmarkSymbolic ? "s" : "-")
				+ " path=\"" + item.path() + "\""
				+ " pos=" + ("pos" in item ? item.pos : Vec3d(0,0,0))
				+ " rot=" + ("rot" in item ? item.rot : Quatd(0,0,0,1)));
		}
		return;
	}
	if(::bookmarks == null)
		::bookmarks <- {};

	local item = symbolic ? BookmarkSymbolic(argv[1]) : BookmarkCoordSys(player.cs.findcspath(argv[1]));
	item.pos = Vec3d(2 < argc ? argv[2].tofloat() : 0, 3 < argc ? argv[3].tofloat() : 0, 4 < argc ? argv[4].tofloat() : 0);
	item.rot = 4 < argc ? compilestring("return (" + argv[4] + ")")() : Quatd(0,0,0,1);

	// By default, bookmark entry is referenced by object, which means it follows the marked system
	// even if the system changes path relative to the universe, but fail to switch to replaced system
	// with the former one's path.
	// Adding "symbolic" argument overrides this method to reference by path string, much like the way
	// symbolic links differ from hard links in Unix like filesystems.
	bookmarks[argv[0]] <- item;
}

/// Adds a bookmark with object binding.
register_console_command_a("add_bookmark", function(args){
	return addBookmark(false, args);
});

/// Adds a bookmark with path binding.
register_console_command_a("add_bookmark_s", function(args){
	return addBookmark(true, args);
});

stellarContext <- {
	LY= 9.4605284e12
};

// Assign event handler for CoordSys::readFile. Because Squirrel does not allow class static variables to be altered
// after definition, we must introduce one extra layer of indirection to make it modifiable, or give it up to make class
// member.
::CoordSys.readFile[0] = function(cs, varlist, name, ...){

	// Define temporary function that take varlist as a free variable.
	local eval = function(s)/*:(varlist)*/{
		return compilestring("return (" + s + ")").call(varlist);
	};

	if(name == "bookmark"){
		if(::bookmarks == null)
			::bookmarks <- {};
		local item = BookmarkCoordSys(cs);
		item.pos = Vec3d(1 < vargv.len() ? eval(vargv[1]).tofloat() : 0, 2 < vargv.len() ? eval(vargv[2]).tofloat() : 0, 3 < vargv.len() ? eval(vargv[3]).tofloat() : 0);
		item.rot = 4 < vargv.len() ? eval(vargv[4]) : Quatd(0,0,0,1);
		if(0 < vargv.len()){
			print(vargv[0] + ": " + item);
			::bookmarks[vargv[0]] <- item;
		}
		else{
			local csname = cs.name();
			local i = 0;
			while(csname in ::bookmarks)
				csname = cs.name() + " (" + i++ + ")";
			::bookmarks[csname] <- item;
		}
		return 1;
	}
	else if(name == "equatorial_coord"){
		local RA = vargv[0].tofloat() * 2 * PI / 360;
		local dec = vargv[1].tofloat() * 2 * PI / 360;
		local dist = eval(vargv[2]);
		local pos = Vec3d(-sin(RA) * cos(dec), cos(RA) * cos(dec), sin(dec)) * dist;
//		print(cs.name() + ": " + RA + ", " + dec + pos);
		local earths = universe.findcspath("/sol/earth/Earth");
		local sol = universe.findcspath("/sol");
		print(universe.transRotation(earths) + "=" + sol.transRotation(earths) + "*" + universe.transRotation(sol));
		if(earths != null)
			pos = 
			Quatd.rotation(-PI / 2, Vec3d(1,0,0)).trans
//			(pos);
//			(universe.transRotation(earths).cnj().trans(pos));
			(universe.transPosition(pos, earths));
		pos = /*Quatd.rotation(-PI / 2, Vec3d(1,0,0)).trans*/(pos);
		cs.setpos(pos);
		return 1;
	}
	else if(name == "sol_coord"){
	}
	return 0;
};

::cslabelpat <- "";

register_console_command("cslabel", function(...){
	if(vargv.len() == 0){
		print("cslabel: " + ::cslabelpat);
		return;
	}
	::cslabelpat = vargv[0];
});


function drawCoordSysLabel(cs){
	local names = cs.extranames;
	if(names != null && names.len() && ::cslabelpat.len()){
		local ret = "";
		for(local i = 0; i < names.len(); i++) if(names[i].find(::cslabelpat) != null){
			if(ret == "")
				ret += names[i];
			else
				ret += " / " + names[i];
		}
		if(ret != "")
			return ret;
	}
	return cs.name();
}


function earth(){
	return universe.findcspath("/sol/earth/Earth");
}

function sol(){
	return universe.findcspath("/sol");
}

function Polaris(){
	return universe.findcspath("/ns/Polaris");
}



function ae(){
//	deltaFormation("Assault", 0, Quatd(0,1,0,0));
//	deltaFormation("Assault", 1, Quatd(0,0,0,1), Vec3d(0, 1.9, 0), 0.2);
//	player.cs.addent("Assault", Vec3d(-1, 0,0));
	deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0, 0.1, -0.2), 0.1, 3, player.cs, null);
	deltaFormation("Sceptor", 1, Quatd(0,1,0,0), Vec3d(0, 0.1, 1.2), 0.1, 3, player.cs, null);
//	deltaFormation("Destroyer", 1, Quatd(0,0,0,1), Vec3d(0, 1.1, -0.2), 0.3);
}

function ass(){
	deltaFormation("Assault", 0, Quatd(0,0,0,1), Vec3d(0,0.1,-0.1), 0.3, 3, player.cs, null);
}

function des(){
	deltaFormation("Destroyer", 0, Quatd(0,0,0,1), Vec3d(0,0.1,-0.1), 0.3, 3, player.cs, null);
}

function att(){
	deltaFormation("Attacker", 0, Quatd(0,1,0,0), Vec3d(0,0.1,-0.8), 0.5, 3, player.cs, null);
}
function sce(){
	deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0,0.03,-0.8), 0.5, 3, player.cs, null);
}

register_console_command("ass", ass);
register_console_command("att", att);

mainmenu <- GLWbigMenu();

function loadmission(script){
	mainmenu.close();
	print("loading " + script);
	local ret = timemeas(function()/*:(script)*/{return loadfile(script);});
	print("compile time " + ret.time);
	local exe = ret.result;
	if(exe == null){
		print("Failed to load file " + script);
	}
	else{
		ret = timemeas(exe);
		print("execution time " + ret.time);
	}
	squirrelBind.mission = script;
	return exe;
}

register_console_command("loadmission", loadmission);

function clientMessage(){
	print("calling demo1...");
	loadmission("scripts/demo1.nut");
}

//tutorial1 <- loadmission("scripts/tutorial1.nut");

function init_Universe(){

	player.setpos(Vec3d(0.0, 0.2, 5.5));
	player.setrot(Quatd(0., 1., 0., 0.));

	cmd("r_overlay 0");
	cmd("r_move_path 1");

	local earths = universe.findcspath("/sol/earth/Earth/earths");
	if(!earths)
		earths = universe.findcspath("/earth/Earth/earths");
	if(earths){
		earths.setrot(Quatd.direction(earths.getpos()) * Quatd.rotation(PI / 2, Vec3d(1, 0, 0)));
	}
}

sysbut <- null;

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
	but.addMoveOrderButton("textures/move2.png", "textures/move.png", tlate("Move order"));
	but.addToggleButton("attackorder", "textures/attack2.png", "textures/attack.png", tlate("Attack order"));
	but.addToggleButton("forceattackorder", "textures/forceattack2.png", "textures/forceattack.png", tlate("Force Attack order"));
	but.addButton("halt", "textures/halt.png", tlate("Halt"));
	but.addButton("dock", "textures/dock.png", tlate("Dock"));
	but.addButton("undock", "textures/undock.png", tlate("Undock"));
	but.addControlButton("textures/control2.png", "textures/control.png", tlate("Control"));
	but.pinned = true;

	local cambut = GLWbuttonMatrix(5, 1);
	cambut.title = tlate("camera");
	cambut.x = but.width;
	cambut.y = sch - cambut.height;
	cambut.addButton("chasecamera", "textures/focus.png", tlate("Follow Camera"));
	cambut.addButton("mover cycle", "textures/cammode.png", tlate("Switch Camera Mode"));
	cambut.addButton("originrotation", "textures/resetrot.png", tlate("Reset Camera Rotation"));
	cambut.addButton("eject", "textures/eject.png", tlate("Eject Camera"));
	cambut.addButton("bookmarks", "textures/eject.png", tlate("Teleport"));
	cambut.pinned = true;

	sysbut = GLWbuttonMatrix(3, 2, 32, 32);
	sysbut.title = tlate("System");
	sysbut.x = scw - sysbut.width;
	sysbut.y = sch - sysbut.height;
	sysbut.addButton("exit", "textures/exit.png", tlate("Exit game"));
	sysbut.addToggleButton("pause", "textures/pause.png", "textures/unpause.png", tlate("Toggle Pause"));
	sysbut.addButton("togglesolarmap", "textures/solarmap.png", tlate("Solarmap"));
	sysbut.addButton("r_overlay t", "textures/overlay.png", tlate("Show Overlay"));
	sysbut.addButton("r_move_path t", "textures/movepath.png", tlate("Show Move Path"));
	sysbut.addButton("toggle g_drawastrofig", "textures/showorbit.png", tlate("Show Orbits"));
	sysbut.pinned = true;
}

showdt <- false;


/// An experimental function that dumps list of all defined symbols in a table.
/// Also prints classes' base class names and instances' class names, if available in the root table.
/// Specifying this as default value of the argument seems to cause problems.
function traceSymbols(table){
	foreach(i,v in table){
		local ext = "";
		if(typeof(v) == "class"){
			local baseclass = v.getbase();
			if(baseclass != null){
				foreach(i2,v2 in this){
					if(v2 == baseclass){
						ext = "extends " + i2;
						break;
					}
				}
				if(ext == "")
					ext = "extends " + baseclass.tostring();
			}
		}
		else if(typeof(v) == "instance"){
			try{
				local baseclass = v.getclass();
				foreach(i2,v2 in this){
					if(v2 == baseclass){
						ext = "instanceof " + i2;
						break;
					}
				}
				if(ext == "")
					ext = "instanceof " + baseclass.tostring();
			}
			catch(e){
				ext = "";
				foreach(i2,v2 in this){
					if(typeof(v2) == "class" && v instanceof v2){
						ext = "instanceof " + i2;
						break;
					}
				}
			}
		}
		print(typeof(v) + " " + i + " = " + v + " " + ext);
	}
}

function tst(){
	traceSymbols(this);
}

/// A functionoid (function-like class object) that groups symbols in the root table
/// and dumps the groups using ::traceSymbols().
class GroupTraceSymbols{
	static Classes = 1;
	static Instances = 2;
	static Functions = 4;
	function _call(othis, flags = 7){
		local classes = {};
		local instances = {};
		local functions = {};

		foreach(i,v in othis){
			local typestring = typeof(v);
			if(typestring == "class")
				classes[i] <- v;
			else if(typestring == "instance")
				instances[i] <- v;
			else if(typestring == "function")
				functions[i] <- v;
		}

		if(flags & Classes)
			::traceSymbols(classes);
		if(flags & Instances)
			::traceSymbols(instances);
		if(flags & Functions)
			::traceSymbols(functions);
	}
}

groupTraceSymbols <- GroupTraceSymbols();

gts <- groupTraceSymbols;

//print("init.nut execution time: " + tm.lap() + " sec");
