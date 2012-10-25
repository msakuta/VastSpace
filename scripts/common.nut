
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
	Entity bulletlist;
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
	float health; // readonly
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
	bool isControlling(); // Returns controlled != null with minimal overhead.
	void select(Entity entity); // Add an entity to selection list; doesn't take effect in the server.
	EntitySet selected; // readonly
	Entity chase;
	Entity controlled; // Controlled Entity, assigning makes the Player to begin controlling.
	int chasecamera;
	float viewdist;
	CoordSys cs;
	int playerId; // readonly
}

::player <- Player();

/// Returns array of all players
Player[] ::players();

// Registers a function as a console command. You can manually add entries to global table 'console_commands' too.
void register_console_command(string name, function);

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

/// Sends a client message to the server. Only effective in the client.
/// The variable length arguments are interpreted depending clientMessageId.
/// For example, the following line shows the signature of DockCommand client message.
///   sendClientMessage("DockCommand", Entity e);
void sendClientMessage(string clientMessageId, ...);


// The application will try to call the following functions occasionary.
// Define them in this file in order to respond such events.


// Called each frame.
void frameproc(double frametime);

// Called when an Entity is about to be deleted.
void hook_delete_Entity(Entity e);

// Called everytime the system needs some string to be translated.
string translate(string source);

// The file name of the stellar file.
string stellar_file = "space.ssd";

*/

// load the module
if(1){
	local dllpath = isLinux() ? "Debug/gltestdll.so" : debugBuild() ?
		x64Build() ? "gltestdll.dll" : "..\\gltestplus\\Debug\\gltestdll.dll" :
		x64Build() ? "gltestdll.dll" : "gltestdll.dll";
//		x64Build() ? "x64\\Debug\\gltestdll.dll" : "..\\gltestplus\\Debug\\gltestdll.dll" :
//		x64Build() ? "x64\\Release\\gltestdll.dll" : "gltestdll.dll";
	local gltestdll = loadModule(dllpath);
	print("x64: " + x64Build() + ", \"" + dllpath + "\"l refc is " + gltestdll);
}


/// \brief Helper class that accumulates sequence numbers and calculate statistical values of them.
class CStatistician{
	avg = 0; // sum(value) == average
	m20 = 0; // sum(value^2) == 2nd moment around 0 == variance - average^2
	m30 = 0; // sum(value^3) == 3rd moment around 0
	cnt = 0; // count of samples so far
	function put(value){
		avg = (cnt * avg + value) / (cnt + 1);
		m20 = (cnt * m20 + value * value) / (cnt + 1);
		m30 = (cnt * m30 + value * value * value) / (cnt + 1);
		cnt++;
	}
	function getVar(){ return m20 - avg * avg; } // variance
	function getDev(){ return ::sqrt(getVar()); } // deviation
}


class Cvar{
	function _set(idx,val){
		::set_cvar(idx,val);
	}
	function _get(idx){
		return ::get_cvar(idx);
	}
}

cvar <- Cvar();

function printtree(cs){
	local child;
	for(child = cs.child(); child != null; child = child.next()){
		print(child.getpath());
		printtree(child);
	}
}

function foreachents(cs, proc){
	foreach(e in cs.entlist)
		proc(e);
}

function foreachbullets(cs, proc){
	foreach(e in cs.bulletlist)
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
	["Toggle Other Players Camera View"]="他プレイヤーカメラの表示",
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

register_console_command("pause", function(...){
	if(vargv.len() < 1){
		print("pause is " + universe.paused.tointeger());
		return;
	}
	if(vargv[0] == "t")
		universe.paused = !universe.paused;
	else
		universe.paused = vargv[0].tointeger() != 0;
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
	foreach(e in player.selected)
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

beginControl <- {};
endControl <- {};

/// Response functions that will be called when a client message with the given string is passed.
/// Running arbitrary code given from the client on the server can be the most dangerous security hole,
/// so we only permit certain operations defined in this table.
clientMessageResponses <- {
	load_eternalFight = @() loadmission("scripts/eternalFight.nut"),
	load_demo1 = @() loadmission("scripts/demo1.nut"),
	load_demo2 = @() loadmission("scripts/demo2.nut"),
	load_demo3 = @() loadmission("scripts/demo3.nut"),
	load_demo4 = @() loadmission("scripts/demo4.nut"),
	load_demo5 = @() loadmission("scripts/demo5.nut"),
	load_demo6 = @() loadmission("scripts/demo6.nut"),
	load_demo7 = @() loadmission("scripts/demo7.nut"),
	load_demo8 = @() loadmission("gundam/demo.nut"),
};


function earth(){
	return universe.findcspath("/sol/earth/Earth");
}

function sol(){
	return universe.findcspath("/sol");
}

function Polaris(){
	return universe.findcspath("/ns/Polaris");
}




function printPlayers(){
	local pls = ::players();
	local i;
	for(i = 0; i < pls.len(); i++){
		local pl = pls[i];
		print("Player[" + i + "] = {playerId = " + pl.playerId + ", chase = " + pl.chase + "}");

		local plsel = pl.selected;
		if(0 < plsel.len()){
			print(pl + " has " + plsel.len() + " selections");
			foreach(e in pl.selected)
				print(e);
		}
	}
}


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

