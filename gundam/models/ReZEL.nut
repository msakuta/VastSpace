
dofile("gundam/models/MobileSuit.nut");

modelScale <- 1./30000;

mass <- 4.e3;

maxhealth <- 100.;

bulletSpeed <- 2.;
walkSpeed <- 0.05;
reloadTime <- 5.;
rifleMagazineSize <- 5;
vulcanReloadTime <- 5.;
vulcanMagazineSize <- 20;

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(0.005, 0.01025, 0.005)],
];

function drawOverlay(){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-0.10, -0.10);
	glVertex2d(-0.05,  0.00);
	glVertex2d(-0.10,  0.10);
	glVertex2d( 0.00,  0.05);
	glVertex2d( 0.10,  0.10);
	glVertex2d( 0.05,  0.00);
	glVertex2d( 0.10, -0.10);
	glVertex2d( 0.00, -0.05);
	glEnd();
}

function popupMenu(e){
	local function selector(com,i){
		foreach(e in player.selected)
			e.command(com, i);
	}
	local selectTransform = @(i) selector("Transform", i);
	local selectWeapon = @(i) selector("Weapon", i);
	local selectStabilizer = @(i) selector("Stabilizer", i);

	return [
		{title = "Transform to Waverider", proc = @() selectTransform(1)},
		{title = "Transform to Fighter", proc = @() selectTransform(0)},
		{separator = true},
		{title = "Arm Beam Rifle", proc = @() selectWeapon(0)},
		{title = "Arm Shield Beam", proc = @() selectWeapon(1)},
		{title = "Arm Vulcan", proc = @() selectWeapon(2)},
		{title = "Arm Beam Sabre", proc = @() selectWeapon(3)},
		{separator = true},
		{title = "Turn on Stabilizer", proc = @() selectStabilizer(1)},
		{title = "Turn off Stabilizer", proc = @() selectStabilizer(0)},
		{separator = true},
		{title = "Get Cover", proc = @() selector("GetCover", 1)},
		{title = "Exit Cover", proc = @() selector("GetCover", 0)},
		{separator = true},
		{title = "Reload", proc = function(){ foreach(e in player.selected) e.command("Reload")}},
	];
}

function cockpitView(e,seatid){
	local function min(a,b){ return a < b ? a : b }
	local function aimRot(e){
		return Quatd.rotation(e.aimdir[1], Vec3d(0, -1, 0)) * Quatd.rotation(e.aimdir[0], Vec3d(-1, 0, 0));
	}
	local function coverFactor(e){
		return min(e.coverRight, 1.);
	}
	local src = [
		Vec3d(0., 0.001, -0.002),
		Vec3d(0., 0.018,  0.035),
		Vec3d(0., 0.018,  0.035),
		Vec3d(0.008, 0.007,  0.013),
	];
	local ret = {pos = e.pos, rot = e.rot};
	seatid = (seatid + 4) % 4;
	local ofs = Vec3d(0,0,0);
	if(seatid == 2 && e.enemy != null && e.enemy.alive && e.enemy.cs == e.cs){
		ret.rot = e.rot * Quatd.direction(e.rot.cnj().trans(e.pos - e.enemy.pos));
		ofs = ret.rot.trans(Vec3d(src[seatid][0], src[seatid][1], src[seatid][2] / player.fov)); // Trackback if zoomed
	}
	else{
		ret.rot = e.rot * aimRot(e);
		ofs = ret.rot.trans(src[seatid] * (1. - coverFactor(e)) + Vec3d(0.009, 0.007, 0.015) * coverFactor(e));
	}
	ret.pos = e.pos + ofs;
	return ret;
}

if(isClient())
	registerControls("ReZEL");
