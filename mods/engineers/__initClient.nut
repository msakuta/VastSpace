
dofile("mods/engineers/common.nut");

local old_callTest = "callTest" in this ? callTest : null;

function callTest(){
	print("Overridden callTest()");
	if(old_callTest != null)
		old_callTest();

	mainmenu.addItem("Engineers Demo", @() sendCM("load_engdemo"));

	// Adjust window position to center of screen, after all menu items are added.
	mainmenu.x = screenwidth() / 2 - mainmenu.width / 2;
	mainmenu.y = screenheight() / 2 - mainmenu.height / 2;
}

/// \brief Display a window to show all items an Engineer has.
function showInventory(a){
	local w = GLwindowSizeable(a.classname + " Inventory");
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
//		glwprint("class       : " + a.classid);

		if("listItems" in a){
			foreach(key,value in a.listItems()){
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
				glwprint(format("%-12s: %g %g", value.name, value.mass, value.volume));
				i += 1;
			}
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

register_console_command("inventory", function(){
	local sels = player.selected;
	showInventory(sels[0]);
});
