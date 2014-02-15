// load the module

local dllpath = isLinux() ?
	debugBuild() ? "Debug/gltestdll.so" : "Release/gltestdll.so" :
	debugBuild() ?
	x64Build() ? "gltestdll.dll" : "gltestdll.dll" :
	x64Build() ? "gltestdll.dll" : "gltestdll.dll";
//	x64Build() ? "x64\\Debug\\gltestdll.dll" : "..\\gltestplus\\Debug\\gltestdll.dll" :
//	x64Build() ? "x64\\Release\\gltestdll.dll" : "gltestdll.dll";
local refc = loadModule(dllpath);
print("x64: " + x64Build() + ", \"" + dllpath + "\"l refc is " + refc);

