
modelScale <- 1.e-4;

mass <- 4.e3;

maxhealth <- 200.; // Show some guts for demonstrating shooting effect in the client.

maxfuel <- 120.; // seconds for full thrust

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(0.005, 0.002, 0.003)],
];
 
enginePos <- [
	{pos = Vec3d(0,0,30.) * modelScale, rot = Quatd(0,0,0,1)},
	{pos = Vec3d(34.5,0,40.) * modelScale, rot = Quatd(0,0,0,1)},
	{pos = Vec3d(-34.5,0,40.) * modelScale, rot = Quatd(0,0,0,1)},
];

local ofs = 32.5;
local upangle = Quatd(sin(PI*5./6./2.),0.,0.,cos(PI*5./6./2.));
local dnangle = Quatd(sin(-PI*5./6./2.),0.,0.,cos(-PI*5./6./2.));

enginePosRev <- [
	{pos = Vec3d( 34.5, 0, ofs) * modelScale, rot = upangle},
	{pos = Vec3d( 34.5, 0, ofs) * modelScale, rot = dnangle},
	{pos = Vec3d(-34.5, 0, ofs) * modelScale, rot = upangle},
	{pos = Vec3d(-34.5, 0, ofs) * modelScale, rot = dnangle},
];

function popupMenu(e){
	return [
		{title = translate("Dock"), cmd = "dock"},
		{title = translate("Military Parade Formation"), cmd = "parade_formation"},
		{separator = true},
		{title = translate("Cloak"), cmd = "cloak"},
		{title = translate("Delta Formation"), cmd = "delta_formation"},
	];
}

function cockpitView(e,seatid){
	local src = [Vec3d(0., 0.001, 0.002) * 3, Vec3d(0., 0.008, 0.020), Vec3d(0., 0.008, 0.020)];
	local ret = {pos = e.pos, rot = e.rot};
	seatid = (seatid + 3) % 3;
	local ofs = Vec3d(0,0,0);

	// Currently, the equality operator (==) won't return true for the same CoordSys object.
	// It's caused by Squirrel's operator interpretation, so we cannot override its behavior
	// by providing a metamethod for comparison (_cmp).  The _cmp metamethod just affects
	// relation operators (<,>,<=,=>,<=>), not equality operators (==,!=).
	// For the time being, we use relation order operator (<=>) and compare its result to 0
	// to make it work.
	if(seatid == 2 && e.enemy != null && e.enemy.alive && (e.enemy.cs <=> e.cs) == 0){
		ret.rot = e.rot * Quatd.direction(e.rot.cnj().trans(e.pos - e.enemy.pos));
		ofs = ret.rot.trans(Vec3d(src[seatid][0], src[seatid][1], src[seatid][2] / player.fov)); // Trackback if zoomed
	}
	else{
		ret.rot = e.rot;
		ofs = ret.rot.trans(src[seatid]);
	}
	ret.pos = e.pos + ofs;
	return ret;
}

function drawOverlay(){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-1.0, -1.0);
	glVertex2d(-0.5,  0.0);
	glVertex2d(-1.0,  1.0);
	glVertex2d( 0.0,  0.5);
	glVertex2d( 1.0,  1.0);
	glVertex2d( 0.5,  0.0);
	glVertex2d( 1.0, -1.0);
	glVertex2d( 0.0, -0.5);
	glEnd();
}
