local THIS_FILE = "mods/engineers/common.nut";

// Prevent multiple includes
if(THIS_FILE in this)
	return;
this[THIS_FILE] <- true;

local dllpath = "engineers.dll";
local refc = loadModule(dllpath);
print("x64: " + x64Build() + ", \"" + dllpath + "\"l refc is " + refc);

clientMessageResponses["load_engdemo"] <- @() loadmission("mods/engineers/scripts/engdemo.nut");
