
/* Pseudo-prototype declaration of pre-defined classes and global variables
  by the application.
   This file is loaded for initializing the client game.
   Most of codes are shared among the server and the client, which reside in
  common.nut.  Besure to check the file's notes to fully understand the specification.
   Note that type of returned value and arguments are not specifiable in Squirrel language,
  but shown here to clarify semantics.





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



// The application will try to call the following functions occasionary.
// Define them in this file in order to respond such events.
// Check also the corresponding section in common.nut.

// Called just after the Universe is initialized.
// Please note that when this script file itself is interpreted, the universe is not yet initialized,
// so you must defer operation on the universe using this callback.
void init_Universe();


*/

//local tm = TimeMeas();

dofile("scripts/common.nut");


print("Squirrel script for the client initialized!"); 




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

register_console_command("seat", function(...){
	if(vargv.len() == 0){
		print("seat is " + player.chasecamera);
		return;
	}
	player.chasecamera = vargv[0].tointeger();
});


register_console_command("viewdist", function(...){
	if(vargv.len() == 0){
		print("viewdist is " + player.viewdist);
		return;
	}
	player.viewdist = vargv[0].tofloat();
});

register_console_command("viewdist_zoom", function(...){
	if(vargv.len() == 0){
		print("viewdist is " + player.viewdist);
		return;
	}
	player.viewdist *= vargv[0].tofloat();
});

register_console_command("halt", function(...){
	foreach(e in player.selected)
		e.command("Halt");
});

mainmenu <- GLWbigMenu();

function loadmission(script){
	local ret = timemeas(function(){return CMSQ();});
	print("send time " + ret.time);
	return ret;
}

register_console_command("loadmission", loadmission);

//tutorial1 <- loadmission("scripts/tutorial1.nut");

function init_Universe(){
	function callTest(){
		print("Closing mainmenu" + mainmenu);
		mainmenu.close();
		mainmenu = GLWbigMenu();
		print("New mainmenu" + mainmenu);
		mainmenu.title = "Test Missions";
		mainmenu.addItem("Eternal Fight Demo", function(){loadmission("scripts/eternalFight.nut");});
		mainmenu.addItem("Interceptor vs Defender", function(){loadmission("scripts/demo1.nut");});
		mainmenu.addItem("Interceptor vs Frigate", function(){loadmission("scripts/demo2.nut");});
		mainmenu.addItem("Interceptor vs Destroyer", function(){loadmission("scripts/demo3.nut");});
		mainmenu.addItem("Defender vs Destroyer", function(){loadmission("scripts/demo4.nut");});
		mainmenu.addItem("Demo 5", function(){loadmission("scripts/demo5.nut");});
		mainmenu.addItem("Container Ship Demo", function(){loadmission("scripts/demo6.nut");});
		mainmenu.addItem("Surface Demo", function(){loadmission("scripts/demo7.nut");});
		mainmenu.addItem("Demo 8", function(){loadmission("gundam/demo.nut");});

		// Adjust window position to center of screen, after all menu items are added.
		mainmenu.x = screenwidth() / 2 - mainmenu.width / 2;
		mainmenu.y = screenheight() / 2 - mainmenu.height / 2;
	}

	local scw = screenwidth();
	local sch = screenheight();

	mainmenu.title = "Select Mission";
	mainmenu.addItem("Tutorial 1 - Basic", "loadmission \"scripts/tutorial1.dat\"");
	mainmenu.addItem("Tutorial 2 - Combat", "loadmission \"scripts/tutorial2.nut\"");
	mainmenu.addItem("Tutorial 3", "loadmission \"scripts/tutorial3.nut\"");
	mainmenu.addItem("test", callTest);
	mainmenu.addItem("Walkthrough", function(){mainmenu.close();});
	mainmenu.addItem("Exit", "exit");

	// Adjust window position to center of screen, after all menu items are added.
	mainmenu.x = scw / 2 - mainmenu.width / 2;
	mainmenu.y = sch / 2 - mainmenu.height / 2;

	cmd("r_overlay 0");
	cmd("r_move_path 1");
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
	cambut.addButton("toggle g_player_viewport", "textures/playercams.png", tlate("Toggle Other Players Camera View"));
//	cambut.addButton("bookmarks", "textures/eject.png", tlate("Teleport"));
	cambut.pinned = true;

	local chat = GLWchat();
	chat.x = 0;
	chat.width = 500;
	chat.y = 0;
	chat.height = 200;
	chat.closable = true;
	chat.pinnable = true;
	chat.pinned = true;

	// Function to return pause state
	local pause_state = @() universe.paused;

	// Function to toggle pause state
	local pause_press = @() universe.paused = !universe.paused;

	sysbut = GLWbuttonMatrix(3, 2, 32, 32);
	sysbut.title = tlate("System");
	sysbut.x = scw - sysbut.width;
	sysbut.y = sch - sysbut.height;
	sysbut.addButton("exit", "textures/exit.png", tlate("Exit game"));
	sysbut.addButton(GLWsqStateButton("textures/pause.png", "textures/unpause.png", pause_state, pause_press, tlate("Toggle Pause")));
	sysbut.addButton("togglesolarmap", "textures/solarmap.png", tlate("Solarmap"));
	sysbut.addButton("r_overlay t", "textures/overlay.png", tlate("Show Overlay"));
	sysbut.addButton("r_move_path t", "textures/movepath.png", tlate("Show Move Path"));
	sysbut.addButton("toggle g_drawastrofig", "textures/showorbit.png", tlate("Show Orbits"));
	sysbut.pinned = true;
}


showdt <- false;



missionLoaded <- false;

function frameproc(dt){
	if(!("squirrelShare" in this))
		return;
	if(!missionLoaded){
		local mission = squirrelShare.mission;
		if(mission != ""){
			missionLoaded = true;
			if(mainmenu.alive){
				print("Closing mainmenu" + mainmenu);
				mainmenu.close();
			}
			initUI();
		}
	}
}

//print("init.nut execution time: " + tm.lap() + " sec");
