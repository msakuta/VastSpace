
dofile("mods/vastspace/common.nut");

local old_initUniverse = "init_Universe" in this ? init_Universe : null;;

function init_Universe(){
	if(old_initUniverse != null)
		old_initUniverse();

	local scw = screenwidth();
	local sch = screenheight();

	local function subMenu(){
		mainmenu.close();
		mainmenu = GLWbigMenu();
		mainmenu.title = "VastSpace Demos";
		mainmenu.addItem("Container Ship Demo", @() sendCM("load_vastspace_demo1"));
		mainmenu.addItem("VastSpace Demo 2", @() sendCM("load_vastspace_demo2"));
		mainmenu.addItem("Attacker Demo", @() sendCM("load_vastspace_demo_attacker"));

		// Adjust window position to center of screen, after all menu items are added.
		mainmenu.x = scw / 2 - mainmenu.width / 2;
		mainmenu.y = sch / 2 - mainmenu.height / 2;
	}

	mainmenu.addItem("VastSpace Demos", subMenu);

	// Adjust window position to center of screen, after all menu items are added.
	mainmenu.x = scw / 2 - mainmenu.width / 2;
	mainmenu.y = sch / 2 - mainmenu.height / 2;
}

