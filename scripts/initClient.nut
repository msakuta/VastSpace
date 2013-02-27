
/* Pseudo-prototype declaration of pre-defined classes and global variables
  by the application.
   This file is loaded for initializing the client game.
   Most of codes are shared among the server and the client, which reside in
  common.nut.  Besure to check the file's notes to fully understand the specification.
   Note that type of returned value and arguments are not specifiable in Squirrel language,
  but shown here to clarify semantics.




/// Base class for all OpenGL-drawn GUI elements.
class GLelement{
	bool alive;
	string classname;
	int x;
	int y;
	int width;
	int height;
}

/// OpenGL Window
class GLwindow extends GLelement{
}

/// Menu list window
class GLWmenu extends GLwindow{
	void addItem(string title, string command);
	void close();
}

/// Menu list window with big fonts and buttons
class GLWbigMenu extends GLwindowMenu{
}

/// Button matrix window. The matrix size is needed to be specified in the constructor.
class GLWbuttonMatrix extends GLwindow{
	constructor(int xbuttons, int ybuttons, int buttonxsize, int buttonysize);
	void addButton(string command, string buttonimagefile, string tips);
	void addButton(GLWbutton button);
	void addCvarToggleButton(string cvarname, string buttonimagefile, string pushedimagefile, string tips);
	void addMoveOrderButton(string buttonimagefile, string pushedimagefile, string tips);
}

/// Element of GLWbuttonMatrix.
class GLWbutton extends GLelement{
}

/// 2-State button.
class GLWstateButton extends GLWbutton{
}

/// 2-State button with Squirrel function
class GLWsqStateButton extends GLWstateButton{
	/// \param statefunc The function that is called when the button needs to know whether the state is down or up.
	///                  The function must return a boolean value to indicate that. Can be null for non-stateful buttons.
	/// \param pressfunc The function that is called when the button is pressed.
	constructor(string buttonimagefile, string pushedimagefile, function statefunc, function pressfunc, string tips);
}

class GLWmessage extends GLwindow{
	constructor(string message, float timer, string onDestroy);
}

/// A chart window. You can add series of numbers to display.
class GLWchart extends GLwindow{
	/// \param seriesName
	///        Predefined series name. You can choose from "frametime", "framerate", "recvbytes", "frametimehistogram",
	///        "frameratehistogram", "recvbyteshistogram" or "sampled".
	/// \param ygroup
	///        The grouping id for sharing Y axis scaling. By default, all series have independent Y axis scale normalized
	///        by the dynamic range of the series, but sometimes you'd like to plot two or more series in the same scale.
	///        In that case, just assign the same id to those series.
	/// \param sampleName
	///        The predefined sample name. Meaningful only if seriesName is "sampled".
	void addSeries(string seriesName, int ygroup, string sampleName, float color[4]);
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

register_console_command("chart", function(...){
	chart <- GLWchart();
	chart.x = 0;
	chart.y = 200; // Avoid overlapping with GLWchat.
	chart.width = 300;
	chart.height = 150;
//	chart.addSeries("framerate");
	chart.addSeries("frametime", 0, "", [1,0.5,0.5,0.5]);
	chart.addSeries("recvbytes");
//	chart.addSeries("frametimehistogram");
	chart.addSeries("sampled", 0, "drawstartime", [1,0,1,1]);
	chart.addSeries("sampled", -1, "drawstarcount", [0.5,0.5,1,1]);
	chart.addSeries("sampled", -1, "exposure", [1,1,0,1]);
	chart.addSeries("sampled", -1, "diffuse", [0.5,1,1,1]);
	chart.addSeries("sampled", 0, "expsample", [1,0.5,0.5,1]);
	chart.addSeries("sampled", 0, "findtime", [1,0.5,1.0,1]);
//	chart.addSeries("sampled", -1, "ServerMissileMapSize", [0.75,0.75,0.75,1]);
//	chart.addSeries("sampled", -1, "ClientMissileMapSize", [0.5,0.5,0.5,1]);

	// Following charts are only available in debug build.
	if(debugBuild()){
		chart.addSeries("sampled", -1, "tellcount", [1,0.75,0.75,1]);
		chart.addSeries("sampled", -1, "teplcount", [0.75,1,0.75,1]);
		chart.addSeries("sampled", -1, "tevertcount", [0.75,0.75,1,1]);
	}
});

function control(...){
	if(player.isControlling())
		player.controlled = null;
	else if(player.selected.len() != 0)
		player.controlled = player.selected[0];
}

register_console_command("control", control);

mainmenu <- GLWbigMenu();

/// Sends a client message of a given name.
/// The name is interpreted in the server and corresponding handler will be called.
/// Actually, the name is key of clientMessageResponses table, which looks up a
/// function that will be run in the server.
function sendCM(name){
	// Easy checking if the given name exists in the message handlers.
	if(!(name in clientMessageResponses)){
		print(name + " does not seem to exist as a client message handler.");
		return 0;
	}
	local ret = timemeas(function(){return CMSQ(name);});
	print("send time " + ret.time);
	return ret.result;
}

/// Notify calling loadmission in client is invalid.
function loadmission(...){
	print("Invocation of loadmission() in the client machine is prohibited.");
}

//register_console_command("loadmission", loadmission);

//tutorial1 <- loadmission("scripts/tutorial1.nut");

function init_Universe(){
	function callTest(){
		print("Closing mainmenu" + mainmenu);
		mainmenu.close();
		mainmenu = GLWbigMenu();
		print("New mainmenu" + mainmenu);
		mainmenu.title = "Test Missions";
		mainmenu.addItem("Eternal Fight Demo", @() sendCM("load_eternalFight"));
		mainmenu.addItem("Interceptor vs Defender", @() sendCM("load_demo1"));
		mainmenu.addItem("Interceptor vs Frigate", @() sendCM("load_demo2"));
		mainmenu.addItem("Interceptor vs Destroyer", @() sendCM("load_demo3"));
		mainmenu.addItem("Defender vs Destroyer", @() sendCM("load_demo4"));
		mainmenu.addItem("Demo 5", @() sendCM("load_demo5"));
		mainmenu.addItem("Container Ship Demo", @() sendCM("load_demo6"));
		mainmenu.addItem("Surface Demo", @() sendCM("load_demo7"));
		mainmenu.addItem("Demo 8", @() sendCM("load_demo8"));

		// Adjust window position to center of screen, after all menu items are added.
		mainmenu.x = screenwidth() / 2 - mainmenu.width / 2;
		mainmenu.y = screenheight() / 2 - mainmenu.height / 2;
	}

	local scw = screenwidth();
	local sch = screenheight();

	mainmenu.title = "Select Mission";
	mainmenu.addItem("Tutorial 1 - Basic", @() sendCM("load_tutorial1"));
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

	// Whether show chart graph at the startup
	if(true){
		cmd("chart");
	}
}

controlStats <- CStatistician();

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
//	but.addControlButton("textures/control2.png", "textures/control.png", tlate("Control"));
	but.addButton(GLWsqStateButton("textures/control2.png", "textures/control.png",
		@() player.isControlling(), control, tlate("Control")));
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
	sysbut.addButton("sq GLwindowSolarMap()", "textures/solarmap.png", tlate("Solarmap"));
	sysbut.addButton("r_overlay t", "textures/overlay.png", tlate("Show Overlay"));
	sysbut.addButton("r_move_path t", "textures/movepath.png", tlate("Show Move Path"));
	sysbut.addButton("toggle g_drawastrofig", "textures/showorbit.png", tlate("Show Orbits"));
	sysbut.pinned = true;
}





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
			if(squirrelShare.tutorial == "true"){
				local tutorialbut = GLWbuttonMatrix(3, 1);
				tutorialbut.title = tlate("Tutorial");
				tutorialbut.addButton("sq \"sendCM(\\\"tutor_restart\\\")\"", "textures/tutor_restart.png", tlate("Restart"));
				tutorialbut.addButton("sq \"sendCM(\\\"tutor_proceed\\\")\"", "textures/tutor_proceed.png", tlate("Proceed"));
				tutorialbut.addButton("sq \"sendCM(\\\"tutor_end\\\")\"", "textures/tutor_end.png", tlate("End"));
				tutorialbut.x = screenwidth() - tutorialbut.width;
				tutorialbut.y = sysbut.y - tutorialbut.height;
				tutorialbut.pinned = true;
			}
		}
	}
}

//print("init.nut execution time: " + tm.lap() + " sec");
