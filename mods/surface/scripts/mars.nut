// Mars surface simulation test

local targetcs = universe.findcspath("/marss/Mars/marss");
if(!targetcs)
	targetcs = universe.findcspath("/sol/marss/Mars/marss");

if(targetcs != null)
	player.cs = targetcs;

local birds = [];


local f15 = player.cs.addent("F15", Vec3d(200, 720, 0));
f15.setrot(Quatd(0,sqrt(2.)/2.,0,sqrt(2.)/2.));
f15.gear = true;
player.chase = f15;
f15.destArrived = true;
birds.append(f15);


local tank = player.cs.addent("Tank", Vec3d(0, 0, 0));
if(targetcs){
	local mars = targetcs.parent;
	if("getTerrainHeight" in mars){
		local marsPos = mars.transPosition(tank.pos, player.cs);
		marsPos.normin();
		marsPos *= mars.getTerrainHeight(marsPos) * mars.radius + 2.;
		tank.pos = player.cs.transPosition(marsPos, mars);
	}
}
//player.chase = tank;

universe.paused = false;
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 250.;
