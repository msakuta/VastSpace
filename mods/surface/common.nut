
// Prevent multiple includes
if("mods/surface/common.nut" in this)
	return;
this["mods/surface/common.nut"] <- true;

// Load the surface module.
// Note that loading here invokes loadModule() twice in self-hosted server.
local dllpath = debugBuild() ?
	x64Build() ? "surface.dll" : "surface.dll" :
	x64Build() ? "surface.dll" : "surface.dll";
//	x64Build() ? "x64\\Debug\\gltestdll.dll" : "..\\gltestplus\\Debug\\gltestdll.dll" :
//	x64Build() ? "x64\\Release\\gltestdll.dll" : "gltestdll.dll";
local refc = loadModule(dllpath);
print("\"" + dllpath + "\"l refc is " + refc);

clientMessageResponses["load_surface_demo"] <- @() loadmission("mods/surface/scripts/demo.nut");
