
dofile("mods/gundam/common.nut");

local old_initUniverse = "init_Universe" in this ? init_Universe : null;;

function init_Universe(){
	if(old_initUniverse != null)
		old_initUniverse();

	local scw = screenwidth();
	local sch = screenheight();

	local function subMenu(){
		mainmenu.close();
		mainmenu = GLWbigMenu();
		mainmenu.title = "Gundam Demos";
		mainmenu.addItem("Demo 1", @() sendCM("load_gundamdemo"));

		// Adjust window position to center of screen, after all menu items are added.
		mainmenu.x = scw / 2 - mainmenu.width / 2;
		mainmenu.y = sch / 2 - mainmenu.height / 2;
	}

	mainmenu.addItem("Gundam Demos", subMenu);

	// Adjust window position to center of screen, after all menu items are added.
	mainmenu.x = scw / 2 - mainmenu.width / 2;
	mainmenu.y = sch / 2 - mainmenu.height / 2;
}

