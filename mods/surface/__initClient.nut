
dofile("mods/surface/common.nut");

local old_initUniverse = "init_Universe" in this ? init_Universe : null;;

function init_Universe(){
	if(old_initUniverse != null)
		old_initUniverse();

	mainmenu.addItem("Surface Demo", @() sendCM("load_surface_demo"));
}

