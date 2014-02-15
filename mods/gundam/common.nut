// Prevent multiple includes
if("mods/gundam/common.nut" in this)
	return;
this["mods/gundam/common.nut"] <- true;


// Load the module
{
	local dllpath = "gundam.dll";
	local dll = loadModule(dllpath);
	print("loaded \"" + dllpath + "\" refc is " + dll);
}

clientMessageResponses["load_gundamdemo"] <- @() loadmission("gundam/demo.nut");
