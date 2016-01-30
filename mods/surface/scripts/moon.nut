// Moon surface simulation test

local targetcs = universe.findcspath("/sol/earth/moon/Moon/moons");
if(!targetcs)
	targetcs = universe.findcspath("/earth/moon/Moon/moons");

if(targetcs != null)
	player.cs = targetcs;

local birds = [];


local sc = player.cs.addent("Sceptor", Vec3d(200, 10720, 5000. - 350.));
sc.setrot(Quatd(0,sqrt(2.)/2.,0,sqrt(2.)/2.));
player.chase = sc;
player.select([sc]);

universe.paused = false;
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 250;
