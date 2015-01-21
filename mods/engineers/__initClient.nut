
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

