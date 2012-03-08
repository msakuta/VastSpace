
/* Pseudo-prototype declaration of pre-defined classes and global variables
  by the application.
   This file is loaded for initializing the server game.
   Most of codes are shared among the server and the client, which reside in
  common.nut.  Besure to check the file's notes to fully understand the specification.
   Note that type of returned value and arguments are not specifiable in Squirrel language,
  but shown here to clarify semantics.






// The application will try to call the following functions occasionary.
// Define them in this file in order to respond such events.
// Check also the corresponding section in common.nut.

// Called just after the Universe is initialized.
// Please note that when this script file itself is interpreted, the universe is not yet initialized,
// so you must defer operation on the universe using this callback.
void init_Universe();

// The file name of the stellar file.
string stellar_file = "space.ssd";

*/

//local tm = TimeMeas();

dofile("scripts/common.nut");

print("Squirrel script for the server initialized!"); 

cvar.pause = "1";

// load the module
if(0){
	local dllpath = debugBuild() ?
		x64Build() ? "gltestdll.dll" : "..\\gltestplus\\Debug\\gltestdll.dll" :
		x64Build() ? "gltestdll.dll" : "gltestdll.dll";
//		x64Build() ? "x64\\Debug\\gltestdll.dll" : "..\\gltestplus\\Debug\\gltestdll.dll" :
//		x64Build() ? "x64\\Release\\gltestdll.dll" : "gltestdll.dll";
	local gltestdll = loadModule(dllpath);
	print("x64: " + x64Build() + ", \"" + dllpath + "\"l refc is " + gltestdll);
}

// load the module
if(0){
	local dllpath = debugBuild() ?
		x64Build() ? "surface.dll" : "..\\gltestplus\\Debug\\surface.dll" :
		x64Build() ? "surface.dll" : "surface.dll";
//		x64Build() ? "x64\\Debug\\gltestdll.dll" : "..\\gltestplus\\Debug\\gltestdll.dll" :
//		x64Build() ? "x64\\Release\\gltestdll.dll" : "gltestdll.dll";
	local gltestdll = loadModule(dllpath);
	print("\"" + dllpath + "\"l refc is " + gltestdll);
}

// load the module
if(0){
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



function loadmission(script){
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
	squirrelShare.mission = script;
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

register_console_command("sq1", @() loadmission("scripts/demo1.nut") );

//print("init.nut execution time: " + tm.lap() + " sec");
