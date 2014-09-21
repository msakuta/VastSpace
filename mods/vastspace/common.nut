
// Prevent multiple includes
if("mods/vastspace/common.nut" in this)
	return;
this["mods/vastspace/common.nut"] <- true;

local dllpath = isLinux() ?
	debugBuild() ? "Debug/vastspace.so" : "Release/vastspace.so" :
	debugBuild() ?
	x64Build() ? "vastspace.dll" : "vastspace.dll" :
	x64Build() ? "vastspace.dll" : "vastspace.dll";
//	x64Build() ? "x64\\Debug\\vastspace.dll" : "..\\gltestplus\\Debug\\vastspace.dll" :
//	x64Build() ? "x64\\Release\\vastspace.dll" : "vastspace.dll";
local refc = loadModule(dllpath);
print("x64: " + x64Build() + ", \"" + dllpath + "\"l refc is " + refc);

clientMessageResponses["load_vastspace_demo1"] <- @() loadmission("mods/vastspace/scripts/demo1.nut");
clientMessageResponses["load_vastspace_demo2"] <- @() loadmission("mods/vastspace/scripts/demo2.nut");


registerFirearm("M16", "models/M16.nut");
registerFirearm("M40", "models/M40.nut");
