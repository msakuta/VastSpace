
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

// Define an event handler to process unhandled stellar file definition directives.
function stellarReadFile(cs, varlist, name, ...){

	// Define temporary function that take varlist as a free variable.
	local eval = function(s)/*:(varlist)*/{
		return compilestring("return (" + s + ")").call(varlist);
	};

	if(name == "bookmark" || name == "teleport"){
		if(::bookmarks == null)
			::bookmarks <- {};
		local item = BookmarkCoordSys(cs);
		item.pos = Vec3d(1 < vargv.len() ? eval(vargv[1]).tofloat() : 0, 2 < vargv.len() ? eval(vargv[2]).tofloat() : 0, 3 < vargv.len() ? eval(vargv[3]).tofloat() : 0);
		item.rot = 4 < vargv.len() ? eval(vargv[4]) : Quatd(0,0,0,1);
		item.warpable = name != "teleport";
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

		// Try to find the earth at the path in case of space_debug.ssd
		if(earths == null)
			earths = universe.findcspath("/earth/Earth");

		// Give up if it couldn't
		if(earths == null)
			throw "Earth could not be found";

		// Try to find Sol at the path in case of space_debug.ssd
		if(sol == null)
			sol = universe;

		// Give up if it couldn't
		if(sol == null)
			throw "sol could not be found";

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
	else if(name == "rstation"){
		local rs = cs.addent("RStation", Vec3d(
			0 < vargv.len() ? vargv[0].tofloat() : 0,
			1 < vargv.len() ? vargv[1].tofloat() : 0,
			2 < vargv.len() ? vargv[2].tofloat() : 0));
		return 1;
	}
	return 0;
};

/// \brief The callback function that is called when a randomly generated star materializes
/// \param name The randomly generated star's name
/// \param pos The position of the star being materialized
/// \param e The Entity which triggered materialization by entering the solar system
/// \param rs The random sequence reference to add characteristics to the solar system.  DO NOT save this object outside of this function.
/// \returns The newly created solar system to associate with StarCache
function materializeStar(name,pos,e,rs){
	local child = Star(name, universe);
	print("materializeStar: " + child);
	child.pos = pos;
	local radscale = exp(rs.nextGauss());
	child.rad = 6.960e5 * radscale; // Exponential distribution around Rsun
	child.mass = 1.9884e30 * radscale * radscale * radscale * exp(rs.nextGauss()); // Mass has variance too, but probably mean is proportional to cubic radius.
	child.spect = "G";
	// Create a temporary orbit to arrive.
	local orbit = Orbit("orbit", child);
	local orbitPos = child.transPosition(e.warpdst, e.warpdstcs);
	orbit.orbits(child, orbitPos.len(), 0., Quatd.direction(orbitPos).rotate(PI / 2., Vec3d(1,0,0)));
	orbit.showOrbit = true;
	e.warpdst = orbit.transPosition(e.warpdst, e.warpdstcs);
	e.warpdstcs = orbit;
	return child;
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



function loadmission(script){
	print("loading " + script);
	local ret = timemeas(function(){return loadfile(script);});
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

function clientMessage(name){
	if(name in clientMessageResponses){
		print("clientMessageResponses[" + name + "]");
		clientMessageResponses.rawget(name)();
	}
	else
		print(name + " does not exist in the server's clientMessageResponse");
}

//tutorial1 <- loadmission("scripts/tutorial1.nut");

// Reserve the previous function
server_old_init_Universe <- "init_Universe" in this ? init_Universe : null;

function init_Universe(){

	print("Server's init_Universe()");

	player.setpos(Vec3d(0.0, 0.2, 0.5));
	player.setrot(Quatd(0., 1., 0., 0.) * Quatd.rotation(-PI / 3., Vec3d(0, 1, 0)));

	cmd("exec dedsvr.cfg");

	// Earth low orbit in space_debug.ssd
	local paths = [
		universe.findcspath("Alpha Centauri/a/orbit"),
//		universe.findcspath("/earth/Earth/earths"),
		universe.findcspath("/earth/moon/moonlo"),
		universe.findcspath("/saturn/saturno1"),
//		universe.findcspath("/sunlo"),
		universe.findcspath("/earth/lo"),
	];
	local a = null;
	for(local i = 0; i < paths.len(); i++){
		if(paths[i]){
			a = paths[i];
			break;
		}
	}

	if(a)
		player.cs = a;
	else{
		local earths = universe.findcspath("/sol/earth/Earth/earths");
		if(!earths)
			earths = universe.findcspath("/earth/Earth/earths");
		if(earths){
			earths.setrot(Quatd.direction(earths.getpos()) * Quatd.rotation(PI / 2, Vec3d(1, 0, 0)));
		}
	}

	if(server_old_init_Universe != null)
		server_old_init_Universe();
}

register_console_command("sq1", @() loadmission("scripts/demo1.nut") );

//print("init.nut execution time: " + tm.lap() + " sec");
