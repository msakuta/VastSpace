// Ceres surface simulation test

local targetcs = universe.findcspath("/ceres/Ceres/ceress");
if(!targetcs)
	targetcs = universe.findcspath("/sol/ceres/Ceres/ceress");

if(targetcs != null)
	player.cs = targetcs;

local birds = [];

// Ceres is atmosphere-less, so adding an airplane wouldn't work.
// Although recent observations imply there's a thin atmosphere on Ceres,
// it wouldn't be enough to sustain airplanes.
/*local f15 = player.cs.addent("F15", Vec3d(200, 720, 5000. - 350.));
f15.setrot(Quatd(0,sqrt(2.)/2.,0,sqrt(2.)/2.));
f15.gear = true;
player.chase = f15;
f15.destArrived = true;
birds.append(f15);*/

local sc = player.cs.addent("Sceptor", Vec3d(200, 720, 5000. - 350.));
sc.setrot(Quatd(0,sqrt(2.)/2.,0,sqrt(2.)/2.));
player.chase = sc;
player.select([sc]);

local tank = player.cs.addent("Tank", Vec3d(0, 0, 0));
if(targetcs){
	local ceres = targetcs.parent;
	if("getTerrainHeight" in ceres){
		local ceresPos = ceres.transPosition(tank.pos, player.cs);
		ceresPos.normin();
		ceresPos *= ceres.getTerrainHeight(ceresPos) * ceres.radius + 2.;
		tank.pos = player.cs.transPosition(ceresPos, ceres);
	}
}
//player.chase = tank;

universe.paused = false;
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 250;
