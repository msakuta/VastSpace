
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

cmd("bind g putvoxel");
cmd("bind h digvoxel");

local function modifyVoxel(put){
	print("putvoxel invoked");
	local src = player.getpos();
	local dir = player.getrot().cnj().trans(Vec3d(0, 0, -1));
	foreachents(player.cs, function(e){
		print("performing ModifyVoxel on " + e);
		e.command("ModifyVoxel", src, dir, put);
	});
}

register_console_command("putvoxel", @() modifyVoxel(true));
register_console_command("digvoxel", @() modifyVoxel(false));
