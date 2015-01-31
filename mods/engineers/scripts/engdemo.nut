
// Reset current position to the Earth's low orbit
local earthlo = universe.findcspath("/sol/mars/marsorbit");
if(!earthlo)
	earthlo = universe.findcspath("/mars/marsorbit");
if(earthlo){
	player.cs = earthlo;
	player.setpos(Vec3d(-1,0,0));
}

universe.paused = false;

local e = player.cs.addent("Engineer", Vec3d(-0.01, 0, 0));
player.chase = e;
player.select([e]);

local voxel = player.cs.addent("VoxelEntity", Vec3d(0.05, 0, 0));

voxel.cellWidth = 0.0025;
voxel.baseHeight = 0.03;
voxel.noiseHeight = 0.015;

cmd("bind g putvoxel");
cmd("bind h digvoxel");
cmd("bind 1 selvoxel 1");
cmd("bind 2 selvoxel 2");
cmd("bind 3 selvoxel 3");
cmd("bind 4 selvoxel 4");
cmd("bind 5 selvoxel 5");
cmd("bind 6 selvoxel 6");
cmd("bind 7 selvoxel 7");
cmd("bind pageup rotvoxel 0 1");
cmd("bind pagedown rotvoxel 0 -1");
cmd("bind home rotvoxel 1 1");
cmd("bind end rotvoxel 1 -1");
cmd("bind insert rotvoxel 2 1");
cmd("bind delete rotvoxel 2 -1");
cmd("bind i inventory");

local voxelType = 1;
local voxelRot = [0,0,0];

local function modifyVoxel(mode){
	local con = player.controlled;
	if(con == null || !(con instanceof Engineer))
		return;
	if(mode == "Put" || mode == "Preview"){
		local items = con.listItems();
		local item = null;
		local itemName = "";
		switch(voxelType){
			case 1: itemName = "RockOre"; break;
			case 2: itemName = "IronOre"; break;
			case 3:
			case 4:
			case 5:
			case 6: itemName = "SteelPlate"; break;
		}
		for(local i = 0; i < items.len(); i++){
			if(items[i].name == itemName && 1 < items[i].volume){
				item = items[i];
				break;
			}
		}
		if(item == null)
			return;
	}
	local src = con.pos;
	local dir = con.rot.trans(Vec3d(0, 0, -1));
	local rottype = voxelRot[0] | (voxelRot[1] << 2) | (voxelRot[2] << 4);
	local hit = false;
	foreach(e in player.cs.entlist){
		if("modifyVoxel" in e){
			if(e.modifyVoxel(src, dir, mode, voxelType, rottype, con)){
				hit = true;
				break;
			}
		}
	}
	if(!hit && mode == "Put"){
		local newve = player.cs.addent("VoxelEntity", src + dir * 0.01);
		newve.setVoxel(voxelType);
	}
}

register_console_command("putvoxel", @() modifyVoxel("Put"));
register_console_command("digvoxel", @() modifyVoxel("Remove"));

register_console_command("selvoxel", function(typestr){
	local type = typestr.tointeger();
	voxelType = type;
	switch(type){
		case 1: print("Rock is selected"); break;
		case 2: print("Iron ore is selected"); break;
		case 3: print("Armor is selected"); break;
		case 4: print("ArmorSlope is selected"); break;
		case 5: print("ArmorCorner is selected"); break;
		default: print("Unknown voxel type is given to selvoxel");
	}
});

register_console_command("rotvoxel", function(axisstr, deltastr){
	local axis = axisstr.tointeger();
	local delta = deltastr.tointeger();
	voxelRot[axis] = (voxelRot[axis] + delta + 4) % 4;
	print("Rotation is " + voxelRot[axis]);
});

frameProcs.append(function (dt){
	modifyVoxel("Preview");
});