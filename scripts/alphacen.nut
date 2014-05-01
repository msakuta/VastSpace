// Start from Alpha Centauri planet orbit
local a = universe.findcspath("Alpha Centauri/a/orbit");
if(a)
	player.cs = a;

local unit = player.cs.addent("DestroyerMissile", Vec3d(0, 0, 0));

unit.capacitor = unit.capacity; // Full charge

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 0.25;

player.chase = unit;

