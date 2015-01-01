
// Reset current position to the Earth's low orbit
local earthlo = universe.findcspath("/sol/mars/marsorbit");
if(!earthlo)
	earthlo = universe.findcspath("/mars/marsorbit");
if(earthlo){
	player.cs = earthlo;
	player.setpos(Vec3d(-1,0,0));
}

universe.paused = false;

local e = player.cs.addent("Soldier", Vec3d(-0.01, 0, 0));
player.chase = e;
player.select([e]);

local voxel = player.cs.addent("VoxelEntity", Vec3d(0.05, 0, 0));

cmd("bind g putvoxel");
cmd("bind h digvoxel");
cmd("bind 1 selvoxel 1");
cmd("bind 2 selvoxel 2");
cmd("bind 3 selvoxel 3");
cmd("bind 4 selvoxel 4");

local voxelType = 1;

local function modifyVoxel(put){
	print("putvoxel invoked");
	local src = player.getpos();
	local dir = player.getrot().cnj().trans(Vec3d(0, 0, -1));
	foreachents(player.cs, function(e){
		print("performing ModifyVoxel on " + e);
		e.command("ModifyVoxel", src, dir, put, voxelType);
	});
}

register_console_command("putvoxel", @() modifyVoxel(true));
register_console_command("digvoxel", @() modifyVoxel(false));

register_console_command("selvoxel", function(typestr){
	local type = typestr.tointeger();
	voxelType = type;
	switch(type){
		case 1: print("Rock is selected"); break;
		case 2: print("Iron ore is selected"); break;
		case 3: print("Armor is selected"); break;
		case 4: print("ArmorSlope is selected"); break;
		default: print("Unknown voxel type is given to selvoxel");
	}
});
