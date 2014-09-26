
dofile("mods/surface/common.nut");

local old_initUniverse = "init_Universe" in this ? init_Universe : null;;

function init_Universe(){
	if(old_initUniverse != null)
		old_initUniverse();

	mainmenu.addItem("Surface Demo", function(){
		print("Closing mainmenu" + mainmenu);
		mainmenu.close();
		mainmenu = GLWbigMenu();
		print("New mainmenu" + mainmenu);
		mainmenu.title = "Test Missions";
		mainmenu.addItem("Earth Surface Demo", @() sendCM("load_surface_demo"));
		mainmenu.addItem("Ceres Surface Demo", @() sendCM("load_surface_ceres"));

		// Adjust window position to center of screen, after all menu items are added.
		mainmenu.x = screenwidth() / 2 - mainmenu.width / 2;
		mainmenu.y = screenheight() / 2 - mainmenu.height / 2;
	});
}

