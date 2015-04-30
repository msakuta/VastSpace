// Sceptor vs Destroyer
local a = universe.findcspath("/earth/lo");
if(a)
	player.cs = a;

local target;
deltaFormation("DestroyerMissile", 1, Quatd(0,0,0,1), Vec3d(0, 0., 1000.), 500., 1, player.cs, @(e) target = e);
deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, 0., -1025.), 25., 10, player.cs, @(e) e.command("Attack", target));

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 250.;

