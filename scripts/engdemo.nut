
// Reset current position to the Earth's low orbit
local earthlo = universe.findcspath("/sol/mars/marsorbit");
if(!earthlo)
	earthlo = universe.findcspath("/mars/marsorbit");
if(earthlo){
	player.cs = earthlo;
	player.setpos(Vec3d(-1,0,0));
}

universe.paused = false;

local voxel = player.cs.addent("VoxelEntity", Vec3d(0, 0, -0.5));
