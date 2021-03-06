
/* Pseudo-prototype declaration of pre-defined classes and global variables
  by the application.
   This file is loaded for initializing the client game.
   Most of codes are shared among the server and the client, which reside in
  common.nut.  Be sure to check the file's notes to fully understand the specification.
   Note that type of returned value and arguments are not specifiable in Squirrel language,
  but shown here to clarify semantics.



/// Primitive object representing a rectangle in a window system.
class GLWrect{
	/// Coordinates of left-top and right-bottom corner of the rectangle.
	/// They are not named like left, top, right or bottom like Windows GDI, because
	/// these directions may vary depending on coordinate systems.
	/// Those ending with '0' represents lower bounds and '1' represents upper bounds
	/// in coordinate values.
	int x0, y0, x1, y1;

	/// Read-only property
	int width, height;
}


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

	/// Returns a GLWrect representing client area of the window.
	/// Coordinates are screen-oriented, which means x0 and y0 are not necessarily zero.
	GLWrect clientRect();

	/// Post close message to this window.  The window will not immediately get destroyed,
	/// but the destruction is deferred to next frame rendering.
	void close();

	/// Draw event handler.  You can set your own handler to this property to customize display content.
	/// \param ws Window state table containing following fields:
	///           int mx, my; // mouse position
	///           int mousex, mousey; // mouse position in client coordinates of the window being drawn.
	onDraw = function(table ws){}

	/// Mouse event handler.  You can set your own handler to respond to mouse events.
	/// \param event Event data table containing following fields:
	///              string key; // Mouse key type string, "leftButton", "middleButton", "rightButton", "wheelUp", "wheelDown", "xbutton1" or "xbutton2".
	///              string state; // Mouse key status, "down", "up", "keepDown" or "keepUp".
	///              int x, y; // Mouse cursor position in client coordinates
	onMouse = function(table event){}

	/// Mouse status query event handler.  You can set your own handler to show specific mouse pointer.
	/// \param mousex Mouse cursor position in screen coordinates
	/// \param mousey Mouse cursor position in screen coordinates
	/// \return Index of mouse cursor types. 0 - normal, 1 - horizontal sizing, 2 - vertical sizing, 3 - right-down diagonal sizing, 4 - right-up diagonal sizing
	onMouseState = function(int mousex, int mousey){return 0;}
}

/// GLwindow that has resizable borders
class GLwindowSizeable extends GLwindow{
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
class GLWchart extends GLwindowSizeable{
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

/// Entity listing window
class GLWentlist extends GLwindowSizeable{
}

/// Chat window for multiplayer game
class GLWchat extends GLwindowSizeable{
}

/// Solar map window
class GLwindowSolarMap extends GLwindowSizeable{
}



int screenwidth();
int screenheight();

// Returns font sizes used by OpenGL window system in pixels.
// The font has fixed width for all glyphs, except for CJK letters.
int fontwidth();
int fontheight();


/// Sets current output position for strings
void glwpos2d(int x, int y);

/// Prints a string on a position designated by glwpos2d().
int glwprint(string);


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



function isWindow(name){
	return (name in this) && this[name] != null && this[name].alive;
}


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
	return cs.name;
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

// Ported from astrobj.cpp
local function hsv2rgb(hsv){
	local hue = hsv[0];
	local sat = hsv[1];
	local bri = hsv[2];
	local hi = floor(hue * 6.).tointeger();
	local f = hue * 6. - hi;
	local p = bri * (1. - sat);
	local q = bri * (1. - f * sat);
	local t = bri * (1. - (1. - f) * sat);
	local r,g,b;
	switch(hi){
		case 0: r = bri, g = t, b = q; break;
		case 1: r = q, g = bri, b = p; break;
		case 2: r = p, g = bri, b = t; break;
		case 3: r = p, g = q, b = bri; break;
		case 4: r = t, g = p, b = bri; break;
		case 5:
		case 6: r = bri, g = p, b = q; break;
		default: r = g = b = bri;
	}
	return [r, g, b];
}

// Returns rainbow color by given phase in range [0,1]
function rainbow(phase){
	return hsv2rgb([phase, 1, 1]);
}

register_console_command("chart", function(...){
	if(isWindow("chart")){
		chart.collapsed = !chart.collapsed;
		return;
	}
	chart <- GLWchart();
	chart.x = screenwidth() - 300;
	chart.y = 300; // Avoid overlapping with GLWentlist.
	chart.width = 300;
	chart.height = 150;
//	chart.addSeries("framerate");
	chart.addSeries("frametime", 0, "", [1,0.5,0.5,0.5]);
//	chart.addSeries("recvbytes");
//	chart.addSeries("frametimehistogram");
//	chart.addSeries("sampled", 0, "drawstartime", [1,0,1,1]);
//	chart.addSeries("sampled", -1, "drawstarcount", [0.5,0.5,1,1]);
//	chart.addSeries("sampled", -1, "exposure", [1,1,0,1]);
//	chart.addSeries("sampled", -1, "diffuse", [0.5,1,1,1]);
//	chart.addSeries("sampled", 0, "expsample", [1,0.5,0.5,1]);
//	chart.addSeries("sampled", 0, "findtime", [1,0.5,1.0,1]);
	chart.addSeries("sampled", 0, "wdtime", [0.5,1.0,0.0,1]);
//	chart.addSeries("sampled", 0, "tintime", [0.0,1.0,0.0,1]);
//	chart.addSeries("sampled", -1, "tinbinary", [0.0,0.75,0.5,1]);
//	chart.addSeries("sampled", -1, "tinfind", [0.0,1.0,0.5,1]);
//	chart.addSeries("sampled", 0, "tinfindtime", [0.5,0.75,0.0,1]);
//	chart.addSeries("sampled", 0, "f15time", [0.0,0.5,1.0,1]);
//	chart.addSeries("sampled", 0, "f15smtime", [1.0,0.5,0.0,1]);
//	chart.addSeries("sampled", 0, "posetime", [1.0,0.5,1.0,1]);
//	chart.addSeries("sampled", 0, "tratime", [1.0,0.5,0.0,1]);
//	chart.addSeries("sampled", -1, "ServerMissileMapSize", [0.75,0.75,0.75,1]);
//	chart.addSeries("sampled", -1, "ClientMissileMapSize", [0.5,0.5,0.5,1]);
//	chart.addSeries("sampled", -1, "tankvelo", [1.0,0.5,0.0,1]);
//	chart.addSeries("sampled", -1, "croll", [1,0.75,0.75,1], -2, 2);
//	chart.addSeries("sampled", -1, "omg", [0.75,1,0.75,1], -1, 1);
//	chart.addSeries("sampled", -1, "iaileron", [0.75,0.75,1,1], -2, 2);
//	chart.addSeries("sampled", -1, "yawDeflect", [1,1,0], -1, 1);
	chart.addSeries("sampled", 2, "dtstime", [1,0,1]);
	chart.addSeries("sampled", 2, "dtstime1", [0.5,0,1]);
	chart.addSeries("sampled", 2, "dtstime2", [1,0.5,1]);
	for(local i = 0; i < 7; i++)
		chart.addSeries("sampled", 3, "lodCount" + i, rainbow(i / 7.));
	chart.addSeries("sampled", 3, "lodCountAll", [1,0.75,1]);
	chart.addSeries("sampled", 3, "lodPatchWaits", [0.5,1,1]);

	// Following charts are only available in debug build.
/*	if(debugBuild()){
		chart.addSeries("sampled", -1, "tellcount", [1,0.75,0.75,1]);
		chart.addSeries("sampled", -1, "teplcount", [0.75,1,0.75,1]);
		chart.addSeries("sampled", -1, "tevertcount", [0.75,0.75,1,1]);
	}*/
});

register_console_command("buildmenu", function(){
	local sel = player.selected;
	foreach(e in sel){
		if(e.builder != null){
			lastBuilderWindow <- GLWbuild(sel[0]);
			return;
		}
	}
});

/// Shows a window to display all celestial bodies in a solar system.
/// Displays current system if no argument is given.
register_console_command("solarmenu", function(...){
	local a = vargv.len() != 0 ? player.cs.findcspath(vargv[0]) : player.cs;
	while(a && a.alive && !a.solarSystem)
		a = a.parent;
	if(!a || !a.solarSystem){
		print("Could not find a solar system");
		return;
	}
	local w = GLwindowSizeable(a.name + " System structure");
	w.closable = true;
	w.width = 320;
	w.height = 480;
	w.x = (screenwidth() - w.width) / 2 + 20;
	w.y = (screenheight() - w.height) / 2 + 20;

	const scrollBarWidth = 16;
	local scrollpos = 0;
	local count = 0;
	local indent = 0;

	// In Squirrel language, calling a local function recursively requires
	// defining the local variable first.
	local iter = null;

	// Iterate over coordinate systems to analyze structure of CoordSys tree.
	// Note that this function assumes no CoordSys object orbits around its
	// child nodes, otherwise it will perform infinite recursive call.
	iter = function(cs, proc, orbitSearch=false){
		// Process the children first (but do not process orbiters)
		local c = cs.child;
		while(c && c.alive){
			iter(c, proc, false);
			c = c.next;
		}

		// Process root system of mass, which in turn process child
		// and orbiter systems.
		if(orbitSearch || cs instanceof Orbit && !cs.orbitCenter){
			if(cs instanceof Astrobj){
				proc(cs);
				indent++;
			}
			foreach(v in cs.orbiters)
				iter(v, proc, true);
			if(cs instanceof Astrobj)
				indent--;
		}
	}

	w.onDraw = function(ws){
		local r = w.clientRect();

		local listHeight = count * fontheight();
		if(r.height < listHeight){ // Show scrollbar
			r.x1 -= scrollBarWidth;
			glwVScrollBarDraw(w, r.x1, r.y0, scrollBarWidth, r.height, listHeight - r.height, scrollpos);
			// Reset scroll pos
			if(listHeight - r.height <= scrollpos)
				scrollpos = listHeight - r.height - 1;
		}
		else
			scrollpos = 0; // Reset scroll pos

		if(!a || !a.alive || !a.solarSystem)
			return;

		glPushAttrib(GL_SCISSOR_BIT);
		glScissor(r.x0, screenheight() - r.y1, r.width, r.height);
		glEnable(GL_SCISSOR_TEST);

		local i = 1;
		local ind = 0 <= ws.mousex && ws.mousex < r.width && 0 <= ws.mousey ? ((ws.mousey + scrollpos) / fontheight()) : -1;

		glColor4fv([1,1,1,1]);
		glwpos2d(r.x0, r.y0 + fontheight() - scrollpos);
		glwprint("Solar System: " + a.getpath());

		indent = 0;
		iter(a, function(cs){
			local ypos = r.y0 + (1 + i) * fontheight() - scrollpos;
//			glColor4fv(isCoordSys ? [0,1,1,1] : [1,1,1,1]);
			glwpos2d(r.x0, ypos);
			local str = "";
			for(local c = 0; c < indent; c++)
				str += "  ";
			glwprint(str + cs.name + " (" + cs.classid + ")");
			i += 1;
		});

		count = i + 1;

		glPopAttrib();
	}

	w.onMouse = function(event){
		local function trackScrollBar(){
			local r = w.clientRect().moved(0,0);

			r.x1 -= scrollBarWidth;

			if(r.height < count * fontheight()){ // Show scrollbar
				local iy = glwVScrollBarMouse(w, event.x, event.y, r.x1, r.y0,
					scrollBarWidth, r.height, count * fontheight() - r.height, scrollpos);
				if(0 <= iy){
					scrollpos = iy;
					return true;
				}
			}
			return false;
		}

		local wheelSpeed = fontheight() * 2;

		if(event.key == "leftButton" && event.state == "down"){
			if(trackScrollBar())
				return true;
		}
		else if(event.key == "wheelUp"){
			if(0 <= scrollpos - wheelSpeed)
				scrollpos -= wheelSpeed;
			else
				scrollpos = 0;
		}
		else if(event.key == "wheelDown"){
			local scrollBarRange = count * fontheight() - (w.clientRect().height - 2 * fontheight());
			if(scrollpos + wheelSpeed < scrollBarRange)
				scrollpos += wheelSpeed;
			else
				scrollpos = scrollBarRange - 1;
		}
		else if(event.key == "leftButton" && event.state == "keepDown"){
			trackScrollBar();
		}
		return false;
	}
});

/// \brief Display a window to show all properties of a CoordSys object.
local function showInfo(a){
	local w = GLwindowSizeable(a.name + " Information");
	w.closable = true;
	w.width = 320;
	w.height = 480;
	w.x = (screenwidth() - w.width) / 2 + 20;
	w.y = (screenheight() - w.height) / 2 + 20;

	const scrollBarWidth = 16;
	local scrollpos = 0;
	local propCount = 0;

	w.onDraw = function(ws){
		local r = w.clientRect();

		local listHeight = propCount * fontheight();
		if(r.height < listHeight){ // Show scrollbar
			r.x1 -= scrollBarWidth;
			glwVScrollBarDraw(w, r.x1, r.y0, scrollBarWidth, r.height, listHeight - r.height, scrollpos);
			// Reset scroll pos
			if(listHeight - r.height <= scrollpos)
				scrollpos = listHeight - r.height - 1;
		}
		else
			scrollpos = 0; // Reset scroll pos

		if(!a || !a.alive)
			return;

		glPushAttrib(GL_SCISSOR_BIT);
		glScissor(r.x0, screenheight() - r.y1, r.width, r.height);
		glEnable(GL_SCISSOR_TEST);

		local i = 1;
		local ind = 0 <= ws.mousex && ws.mousex < r.width && 0 <= ws.mousey ? ((ws.mousey + scrollpos) / fontheight()) : -1;

		glColor4fv([1,1,1,1]);
		glwpos2d(r.x0, r.y0 + fontheight() - scrollpos);
		glwprint("class       : " + a.classid);

		foreach(key,value in a){
			local ypos = r.y0 + (1 + i) * fontheight() - scrollpos;

			local isCoordSys = value instanceof CoordSys;
			if(i == ind && isCoordSys){
				glColor4f(0,0,1,0.5);
				glBegin(GL_QUADS);
				glVertex2d(r.x0, ypos - fontheight());
				glVertex2d(r.x0, ypos);
				glVertex2d(r.x1, ypos);
				glVertex2d(r.x1, ypos - fontheight());
				glEnd();
			}

			glColor4fv(isCoordSys ? [0,1,1,1] : [1,1,1,1]);
			glwpos2d(r.x0, ypos);
			glwprint(format("%-12s: %s", key, "" + value));
			i += 1;
		}
		propCount = i + 1;
		glPopAttrib();
	}

	w.onMouse = function(event){
		local function trackScrollBar(){
			local r = w.clientRect().moved(0,0);

			r.x1 -= scrollBarWidth;

			if(r.height < propCount * fontheight()){ // Show scrollbar
				local iy = glwVScrollBarMouse(w, event.x, event.y, r.x1, r.y0,
					scrollBarWidth, r.height, propCount * fontheight() - r.height, scrollpos);
				if(0 <= iy){
					scrollpos = iy;
					return true;
				}
			}
			return false;
		}

		local wheelSpeed = fontheight() * 2;

		if(event.key == "leftButton" && event.state == "down"){
			if(trackScrollBar())
				return true;
			if(!a || !a.alive)
				return true;
			local ind = ((event.y + scrollpos) / fontheight());
			local i = 1;
			foreach(key,value in a){
				if(i == ind && value instanceof CoordSys){
					a = value;
					w.title = a.name + " Information";
					return true;
				}
				i++;
			}
		}
		else if(event.key == "wheelUp"){
			if(0 <= scrollpos - wheelSpeed)
				scrollpos -= wheelSpeed;
			else
				scrollpos = 0;
		}
		else if(event.key == "wheelDown"){
			local scrollBarRange = propCount * fontheight() - (w.clientRect().height - 2 * fontheight());
			if(scrollpos + wheelSpeed < scrollBarRange)
				scrollpos += wheelSpeed;
			else
				scrollpos = scrollBarRange - 1;
		}
		else if(event.key == "leftButton" && event.state == "keepDown"){
			trackScrollBar();
		}
		return false;
	}
}

/// \brief Shows the window to select a warp destination.
///
/// This window is the first example that almost everything is defined in Squirrel script.
/// It shows powerfulness and high productivity of scripting like JavaScript.
///
/// \param title Window title
///
/// \param locationGenerator Function to return a list of locations.
///        This function must return an array of tables with following members:
///            name: The name of item to display
///            dist: Distance of the location from current viewpoint
///            visited: A flag to filter out unvisited locations
///
/// \param selectEvent Event callback on clicking one of the items.
///        This callback will given two arguments:
///            first: The GLwindow that showing the list.
///            second: Item being selected
local function locationWindow(title,locationGenerator,selectEvent,rightClickEvent){
	local w = GLwindowSizeable(title);
	w.closable = true;
	w.width = 400;
	w.height = 480;
	w.x = (screenwidth() - w.width) / 2;
	w.y = (screenheight() - w.height) / 2;

	const LIGHT_SPEED = 299792.458; /* km/s */
	const AU_PER_KILOMETER = 149597870.691; /* ASTRONOMICAL UNIT */
	local LIGHTYEAR_PER_KILOMETER = LIGHT_SPEED*365*24*60*60; // Couldn't be const

	const scrollBarWidth = 16;

	const columnSizerWidth = 5;

	local sorter = 0; // Index of column used for sorting
	local order = true; // Ascending if true
	local filter = false;
	local sizing = 0;
	local scrollpos = 0;
	local cols = [ // Table of columns definitions
		{title = "Name", value = @(sc) sc.name, width = 16 * fontwidth(), cmp = @(a,b) a.name <=> b.name},
		{title = "Distance", value = @(sc) (sc.dist / LIGHTYEAR_PER_KILOMETER).tostring() + " LY", width = 16 * fontwidth(), cmp = @(a,b) a.dist <=> b.dist},
		{title = "Visited", value = @(sc) sc.visited, width = 10 * fontwidth(), cmp = @(a,b) a.visited <=> b.visited},
	];

	local orgList = locationGenerator(); // Call locationGenerator only once
	local filteredList = []; // Cached filtered list

	local function sortList(){
		filteredList.sort(order ? cols[sorter].cmp : @(a,b) -cols[sorter].cmp(a,b));
	}

	local function filterList(){
		// Always filter before sort because sorting speed greatly depends on item count (at best O(n*log(n)) ).
		if(filter)
			filteredList = orgList.filter(@(i,v) v.visited);
		else
			filteredList = orgList;
		sortList();
	}

	filterList();


	w.onDraw = function(ws){
		local white = [1,1,1,1];
		local selected = [0,1,1,1];
		local r = w.clientRect();
		local fh = fontheight();

		glColor4f(1,1,1,1);

		glwpos2d(r.x0, r.y0 + 1 * fh);
		glwprint("Filter: ");
		glColor4fv(filter ? white : selected);
		glwprint("  All  ");
		glColor4fv(!filter ? white : selected);
		glwprint("  Visited");

		local xpos = r.x0;
		for(local c = 0; c < cols.len(); c++){
			local xpos1 = xpos + cols[c].width;

			// Highlight background of column header under current mouse pointer
			if(fh <= ws.mousey && ws.mousey < 2 * fh && xpos + columnSizerWidth <= ws.mx && ws.mx < xpos1 - columnSizerWidth){
				glColor4f(0,0,1,0.5);
				glBegin(GL_QUADS);
				glVertex2d(xpos, r.y0 + fh);
				glVertex2d(xpos, r.y0 + 2 * fh);
				glVertex2d(xpos1, r.y0 + 2 * fh);
				glVertex2d(xpos1, r.y0 + fh);
				glEnd();
			}

			glColor4fv(sorter != c ? white : selected);
			glwpos2d(xpos, r.y0 + 2 * fh);
			glwprint(cols[c].title);

			// Draw sort order marker if this column is used for sorting
			if(sorter == c){
				glPushMatrix();
				glTranslated(xpos + cols[c].title.len() * fontwidth(), r.y0 + (order ? 1 : 2) * fh, 0);
				glScaled(fh, (order ? 1 : -1) * fh, 1.);
				glBegin(GL_TRIANGLES);
				glVertex2d(0.25, 0.25);
				glVertex2d(0.50, 0.75);
				glVertex2d(0.75, 0.25);
				glEnd();
				glPopMatrix();
			}

			glColor4f(0.5,0.5,0.5,1);
			glBegin(GL_LINES);
			glVertex2d(xpos, r.y0 + fh);
			glVertex2d(xpos1, r.y0 + fh);
			glVertex2d(xpos1, r.y0 + fh);
			glVertex2d(xpos1, r.y1);
			glEnd();
			xpos = xpos1;
		}

		r.y0 += 2 * fh;
		local listHeight = filteredList.len() * fh.tointeger();
		if(r.height < listHeight){ // Show scrollbar
			r.x1 -= scrollBarWidth;
			glwVScrollBarDraw(w, r.x1, r.y0, scrollBarWidth, r.height, filteredList.len() * fh - r.height, scrollpos);
			// Reset scroll pos
			if(listHeight - r.height <= scrollpos)
				scrollpos = listHeight - r.height - 1;
		}
		else
			scrollpos = 0; // Reset scroll pos

		glColor4f(1,0.5,1,1);
		glBegin(GL_LINES);
		glVertex2d(r.x0, r.y0);
		glVertex2d(r.x1, r.y0);
		glEnd();

		glPushAttrib(GL_SCISSOR_BIT);
		glScissor(r.x0, screenheight() - r.y1, r.width, r.height);
		glEnable(GL_SCISSOR_TEST);

		local function min(a,b){return a < b ? a : b;}

		local i = 0;
		local ind = 0 <= ws.mousex && ws.mousex < r.width && 0 <= ws.mousey ? ((ws.mousey + scrollpos) / fh) - 2 : -1;

		foreach(sd in filteredList){
			local ypos = r.y0 + (1 + i) * fh - scrollpos;
			if(r.y0 <= ypos && ypos - fh <= r.y1){

				if(i == ind){
					glColor4f(0,0,1,0.5);
					glBegin(GL_QUADS);
					glVertex2d(r.x0, ypos - fh);
					glVertex2d(r.x0, ypos);
					glVertex2d(r.x1, ypos);
					glVertex2d(r.x1, ypos - fh);
					glEnd();
				}

				glColor4f(1,1,1,1);
				local xpos = r.x0;
				for(local c = 0; c < cols.len(); c++){
					glwpos2d(xpos, ypos);
					local str = cols[c].value(sd).tostring();
					glwprint(str.slice(0, min(str.len(), cols[c].width / fontwidth())));
					xpos += cols[c].width;
				}
			}
			i += 1;
		}

		glPopAttrib();
	}

	w.onMouse = function(event){
		local function trackScrollBar(){
			local r = w.clientRect().moved(0,0);

			r.x1 -= scrollBarWidth;
			r.y0 += 2 * fontheight();

			if(r.height < filteredList.len() * fontheight()){ // Show scrollbar
				local iy = glwVScrollBarMouse(w, event.x, event.y, r.x1, r.y0,
					scrollBarWidth, r.height, filteredList.len() * fontheight() - r.height, scrollpos);
				if(0 <= iy){
					scrollpos = iy;
					return true;
				}
			}
			return false;
		}

		local wheelSpeed = fontheight() * 2;

		if(event.key == "leftButton" && event.state == "down" && event.y < fontheight()){
			filter = !filter;
			filterList();
		}
		else if(event.key == "leftButton" && event.state == "down"){
			if(event.y < 2 * fontheight()){
				local xpos = 0;
				for(local c = 0; c < cols.len(); c++){
					local width = cols[c].width;
					if(xpos + width - columnSizerWidth < event.x && event.x < xpos + width + columnSizerWidth){
						sizing = c+1;
						break;
					}
					else if(xpos < event.x && event.x < xpos + width){
						// Invert sort order if this column is already selected for sorting
						if(c == sorter)
							order = !order;
						else
							order = true;
						sorter = c;
						sortList();
						break;
					}
					xpos += width;
				}
			}
			else{
				if(trackScrollBar())
					return true;
				local ind = ((event.y + scrollpos) / fontheight()) - 2;
				local i = 0;
				foreach(sd in filteredList){
					if(i == ind){
						selectEvent(w,sd);
					}
					i++;
				}
			}
		}
		else if(event.key == "rightButton" && event.state == "down" && rightClickEvent != null){
			local ind = ((event.y + scrollpos) / fontheight()) - 2;
			local i = 0;
			foreach(sd in filteredList){
				if(i == ind){
					rightClickEvent(w,sd);
				}
				i++;
			}
		}
		else if(event.key == "wheelUp"){
			if(0 <= scrollpos - wheelSpeed)
				scrollpos -= wheelSpeed;
			else
				scrollpos = 0;
		}
		else if(event.key == "wheelDown"){
			local scrollBarRange = filteredList.len() * fontheight() - (w.clientRect().height - 2 * fontheight());
			if(scrollpos + wheelSpeed < scrollBarRange)
				scrollpos += wheelSpeed;
			else
				scrollpos = scrollBarRange - 1;
		}
		else if(event.key == "leftButton" && event.state == "keepDown"){
			if(trackScrollBar())
				return true;
			if(sizing){
				local xpos = 0;
				for(local c = 0; c <= sizing - 1; c++)
					xpos += cols[c].width;
				local width = cols[sizing - 1].width + (event.x - xpos);
				// Disallow negative widths
				if(0 < width)
					cols[sizing - 1].width = width;
				return true;
			}
		}
		else if(event.key == "leftButton" && event.state == "up")
			sizing = 0;
		return false;
	}


	w.onMouseState = function(x,y){
		if(sizing)
			return 1;
		else{
			local r = w.clientRect();
			if(r.y0 + fontheight() <= y && y < r.y0 + 2 * fontheight()){
				local xpos = r.x0;
				for(local c = 0; c < cols.len(); c++){
					local width = cols[c].width;
					xpos += width;
					if(xpos - columnSizerWidth < x && x < xpos + columnSizerWidth)
						return 1;
				}
			}
		}
		return 0;
	}
}

register_console_command("warpmenu", @() locationWindow("Warp Destination Window", function(){
	local sclist = [];
	local i = 0;
	local plpos = universe.transPosition(player.getpos(), player.cs);
	local se = StarEnum(plpos, 1, true);
	local params = {};
	while(se.next(params)){
		local sepos = params.pos;
		local sesystem = params.system;
		sclist.append({
			pos = function(plpos){
				if(sesystem)
					return Vec3d(0,0,0)
				else
					return sepos + (plpos - sepos).norm() * AU_PER_KILOMETER;
			},
			dist = (params.pos - plpos).len(),
			name = params.name,
			visited = sesystem != null,
			system = @() sesystem ? sesystem.findcs("orbit") : universe, // Defer execution of findcs() until actual use
		});
	}
	foreach(k,v in bookmarks){
		// Skip non-warpable bookmarks
		if(!v.warpable)
			continue;

		local cs = v.cs();
		if(!cs) // A symbolic bookmark could be broken
			continue;
		sclist.append({
			// Items for locationWindow
			dist = (universe.transPosition(v.pos, cs) - plpos).len(),
			name = k,
			visited = true,

			pos = @(plpos) v.pos,
			system = @() cs,
		});
	}
	return sclist;
},
function(w,sd){
	if(player && player.selected.len()){
		local pe = player.selected[0];
		local plpos = universe.transPosition(pe.pos, pe.cs);
		local destcs = sd.system();
		local destpos = sd.pos(plpos);
		foreach(e in player.selected)
			e.command("Warp", destcs, destpos);
		w.close();
	}
},
function(w,sd){
	if("system" in sd){
		local system = sd.system();
		if(system != null)
			showInfo(sd.system());
	}
}));

register_console_command("teleportmenu", @() locationWindow("Teleport Destination Window", function(){
	local sclist = [];
	local i = 0;
	local plpos = universe.transPosition(player.getpos(), player.cs);
	foreach(k,v in bookmarks){
		local cs = v.cs();
		if(!cs) // A symbolic bookmark could be broken
			continue;
		sclist.append({
			// Items for locationWindow
			dist = (universe.transPosition(v.pos, cs) - plpos).len(),
			name = k,
			visited = true,

			pos = v.pos,
			cs = cs,
		});
	}
	return sclist;
},
function(w,sd){
	player.chase = null;
	player.cs = sd.cs;
	player.setpos(sd.pos);
	player.setvelo(Vec3d(0,0,0));
	w.close();
}, null));

function control(...){
	if(player.isControlling())
		player.controlled = null;
	else if(player.selected.len() != 0)
		player.controlled = player.selected[0];
}

register_console_command("control", control);

register_console_command("inventory", function(){
	local sels = player.selected;
	if(sels.len() == 0)
		return;
	local sel = sels[0];
	local w = GLWinventory(sel);
	w.closable = true;
	w.pinnable = true;
	w.width = 250;
	w.height = 150;

	// Test codes for initial goods
	sel.addItem("hydrogen", 12.);
	sel.addItem("oxygen", 3.);
	sel.addItem("water", 5.);
});

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

if(!isServer()){
	/// Notify calling loadmission in client is invalid, but only if it's not standalone.
	function loadmission(...){
		print("Invocation of loadmission() in the client machine is prohibited.");
	}
}

//register_console_command("loadmission", loadmission);

//tutorial1 <- loadmission("scripts/tutorial1.nut");

// Reserve the previous function
client_old_init_Universe <- "init_Universe" in this ? init_Universe : null;

function init_Universe(){
	cmd("scripter");

	// The default star name sector radius settings
	cmd("r_star_name_sectors " + 1);

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

		// Adjust window position to center of screen, after all menu items are added.
		mainmenu.x = screenwidth() / 2 - mainmenu.width / 2;
		mainmenu.y = screenheight() / 2 - mainmenu.height / 2;
	}

	local scw = screenwidth();
	local sch = screenheight();

	mainmenu.title = "Select Mission";

	// Show tutorial missions only in standalone games.
	if(isServer()){
		// Not using sendCM() because tutorials are only valid in standalone game, which means
		// no client messages over network is required.
		mainmenu.addItem("Tutorial 1 - Basic", @() loadmission("scripts/tutorial1.nut"));
		mainmenu.addItem("Tutorial 2 - Combat", @() loadmission("scripts/tutorial2.nut"));
		mainmenu.addItem("Tutorial 3 - Construction", @() loadmission("scripts/tutorial3.nut"));
	}

	mainmenu.addItem("test", callTest);
	mainmenu.addItem("Start from Alpha Centauri", @() loadmission("scripts/alphacen.nut"));
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

	if(client_old_init_Universe != null)
		client_old_init_Universe();
}

controlStats <- CStatistician();

taskbar <- null;
sysbut <- null;

function initUI(){
	local scw = screenwidth();
	local sch = screenheight();

	local function chatFunc(){
		// If we're in a standalone game, there's no point showing chat window.
		if(isClient() && isServer())
			return;
		if(!isWindow("chatWindow")){
			local chat = GLWchat();
			chat.x = taskbar.x + taskbar.width;
			chat.width = 500;
			chat.y = 0;
			chat.height = 200;
			chat.closable = true;
			chat.pinnable = true;
			chat.pinned = true;
			chatWindow <- chat;
		}
		else
			chatWindow.collapsed = !chatWindow.collapsed;
	}

	local function entlistFunc(){
		if(!isWindow("entlistWindow")){
			local entlist = GLWentlist();
			entlist.title = tlate("Entity List");
			entlist.x = scw - 300;
			entlist.width = 300;
			entlist.y = 100;
			entlist.height = 200;
			entlist.closable = true;
			entlist.pinnable = true;
			entlist.pinned = true;
			entlistWindow <- entlist;
		}
		else
			entlistWindow.collapsed = !entlistWindow.collapsed;
	}

	local function commandButtonFunc(){
		if(!isWindow("commandButtons")){
			local but = GLWbuttonMatrix(3, 3);
			but.title = tlate("command");
			but.x = taskbar.width;
			but.y = sch - but.height;
			moveOrderButton <- but.addMoveOrderButton("textures/move2.png", "textures/move.png", tlate("Move order"));
			attackOrderButton <- but.addToggleButton("attackorder", "textures/attack2.png", "textures/attack.png", tlate("Attack order"));
			but.addToggleButton("forceattackorder", "textures/forceattack2.png", "textures/forceattack.png", tlate("Force Attack order"));
			but.addButton("halt", "textures/halt.png", tlate("Halt"));
			but.addButton("dock", "textures/dock.png", tlate("Dock"));
			but.addButton("undock", "textures/undock.png", tlate("Undock"));
		//	but.addControlButton("textures/control2.png", "textures/control.png", tlate("Control"));
			but.addButton(GLWsqStateButton("textures/control2.png", "textures/control.png",
				@() player.isControlling(), control, tlate("Control")));
			but.pinned = true;
			commandButtons <- but;
		}
		else
			commandButtons.collapsed = !commandButtons.collapsed;
	}

	local function cameraButtonFunc(){
		if(!isWindow("cameraButtons")){
			local cambut = GLWbuttonMatrix(5, 1);
			cambut.title = tlate("camera");
			if(!isWindow("commandButtons"))
				cambut.x = taskbar.width;
			else
				cambut.x = commandButtons.x + commandButtons.width;
			cambut.y = sch - cambut.height;
			cambut.addButton("chasecamera", "textures/focus.png", tlate("Follow Camera"));
			cambut.addButton("mover cycle", "textures/cammode.png", tlate("Switch Camera Mode"));
			cambut.addButton("originrotation", "textures/resetrot.png", tlate("Reset Camera Rotation"));
			cambut.addButton("eject", "textures/eject.png", tlate("Eject Camera"));
			cambut.addButton("toggle g_player_viewport", "textures/playercams.png", tlate("Toggle Other Players Camera View"));
		//	cambut.addButton("bookmarks", "textures/eject.png", tlate("Teleport"));
			cambut.pinned = true;
			cameraButtons <- cambut;
		}
		else
			cameraButtons.collapsed = !cameraButtons.collapsed;
	}

	taskbar = GLWtaskBar();
	taskbar.addButton(GLWsqStateButton("textures/chat.png", null, @() true, chatFunc, tlate("Chat")));
	taskbar.addButton(GLWsqStateButton("textures/entlist.png", "textures/entlist.png",
		@() true, entlistFunc, tlate("Entity List")));
	taskbar.addButton(GLWsqStateButton("textures/commands.png", "textures/commands.png",
		@() true, commandButtonFunc, tlate("Commands")));
	taskbar.addButton(GLWsqStateButton("textures/cameras.png", "textures/cameras.png",
		@() true, cameraButtonFunc, tlate("Cameras")));
	buildManButton <- taskbar.addButton(GLWsqStateButton("textures/buildman.png", null,
		function(){ // True if at least one Entity in selection is a builder.
			foreach(e in player.selected)
				if(e.builder != null)
					return true;
			return false;
		},
		@() cmd("buildmenu"), tlate("Build Manager")));
	taskbar.addButton(GLWsqStateButton("textures/dockman.png", null,
		function(){ // True if at least one Entity in selection is a docker.
			foreach(e in player.selected)
				if(e.docker != null)
					return true;
			return false;
		},
		@() cmd("dockmenu"), tlate("Dock Manager")));
	taskbar.addButton(GLWsqStateButton("textures/chart.png", null,
		@() true, @() cmd("chart"), tlate("Charts")));

	chatFunc();
	entlistFunc();
	commandButtonFunc();
	cameraButtonFunc();

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

frameProcs.append(function (dt){
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
})

//print("init.nut execution time: " + tm.lap() + " sec");
