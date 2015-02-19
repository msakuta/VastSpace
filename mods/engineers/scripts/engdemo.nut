local THIS_FILE = "mods/engineers/scripts/engdemo.nut";

// Prevent multiple includes
if(THIS_FILE in this)
	return;
this[THIS_FILE] <- true;

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
player.controlled = e;
e.addItem("SteelPlate", 100);
e.addItem("Engine", 10);
e.addItem("ReactorComponent", 10);
e.addItem("CockpitComponent", 10);

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
cmd("bind 8 selvoxel 8");
cmd("bind 9 selvoxel 9");
cmd("bind pageup rotvoxel 0 1");
cmd("bind pagedown rotvoxel 0 -1");
cmd("bind home rotvoxel 1 1");
cmd("bind end rotvoxel 1 -1");
cmd("bind insert rotvoxel 2 1");
cmd("bind delete rotvoxel 2 -1");
cmd("bind i inventory");
cmd("bind t controlShip");


local function modifyVoxel(mode){
	local con = player.controlled;
	if(con == null || !(con instanceof Engineer))
		return;
	// If we are controlling a ship, do not try to build blocks
	if(con.controlledShip != null)
		return;
	local voxelType = con.currentCellType;
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
			case 7: itemName = "Engine"; break;
			case 8: itemName = "ReactorComponent"; break;
			case 9: itemName = "CockpitComponent"; break;
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
	local rottype = con.currentCellRotation;
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
	local con = player.controlled;
	if(con == null || !(con instanceof Engineer))
		return;
	local type = typestr.tointeger();
	switch(type){
		case 1: print("Rock is selected"); break;
		case 2: print("Iron ore is selected"); break;
		case 3: print("Armor is selected"); break;
		case 4: print("ArmorSlope is selected"); break;
		case 5: print("ArmorCorner is selected"); break;
		case 6: print("ArmorInvCorner is selected"); break;
		case 7: print("Engine is selected"); break;
		case 8: print("Reactor is selected"); break;
		case 9: print("Cockpit is selected"); break;
		default: print("Unknown voxel type is given to selvoxel");
	}
	con.currentCellType = type;
});

register_console_command("rotvoxel", function(axisstr, deltastr){
	local con = player.controlled;
	if(con == null || !(con instanceof Engineer))
		return;
	local axis = axisstr.tointeger();
	local delta = deltastr.tointeger();
	local rot = con.currentCellRotation;
	local axisRot = (rot >> (axis * 2)) & 3;
	axisRot = (axisRot + delta + 4) % 4;
	// There is no compound assignment operators for bitwise AND and OR operators (&= and |=)
	// in Squirrel, so we must write them as normal assignment.
	rot = rot
		& ~(3 << (axis * 2)) // Clear the rotation
		| (axisRot << (axis * 2)); // Set the new rotation in place of the axis in question
	con.currentCellRotation = rot;
	print("Rotation is " + rot);
});

register_console_command("controlShip", function(){
	local con = player.controlled;
	if(con == null)
		return;
	if(con instanceof Engineer && con.controlledShip != null){
		con.controlledShip.command("EnterCockpit", con);
		return;
	}
	foreach(e in player.cs.entlist){
		if(e != con){
			if(e.command("EnterCockpit", con))
				break;
		}
	}
});

frameProcs.append(function (dt){
	modifyVoxel("Preview");
});
