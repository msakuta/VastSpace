
// Reset current position to the Earth's low orbit
local earthlo = universe.findcspath("/sol/mars/marsorbit");
if(!earthlo)
	earthlo = universe.findcspath("/mars/marsorbit");
if(earthlo){
	player.cs = earthlo;
	player.setpos(Vec3d(-1,0,0));
}

universe.paused = false;

local e = player.cs.addent("Soldier", Vec3d(-0.05, 0, 0));
player.chase = e;
player.select([e]);

local voxel = player.cs.addent("VoxelEntity", Vec3d(0.05, 0, 0));
