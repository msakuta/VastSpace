
modelScale <- 1.e-1;

mass <- 4.e3;

maxhealth <- 200.; // Show some guts for demonstrating shooting effect in the client.

maxfuel <- 120.; // seconds for full thrust

maxAngleAccel <- PI * 0.8;
angleaccel <- PI * 0.4;
maxanglespeed <- PI;
maxspeed <- 500.;

analogDeadZone <- 0.1;
analogSensitivity <- 0.01;

local lateralAccel = 100.;
dir_accel <- [
	lateralAccel, lateralAccel, // Negative/Positive X-axis
	lateralAccel, lateralAccel, // Negative/Positive X-axis
	200., 150., // Forward/backward
]

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(5., 2., 3.)],
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
	local src = [Vec3d(0., 1., 2.) * 3, Vec3d(0., 8., 20.), Vec3d(0., 8, 20.)];
	local ret = {pos = e.pos, rot = e.rot};
	seatid = (seatid + 3) % 3;
	local ofs = Vec3d(0,0,0);

	// Currently, the equality operator (==) won't return true for the same CoordSys object.
	// It's caused by Squirrel's operator interpretation, so we cannot override its behavior
	// by providing a metamethod for comparison (_cmp).  The _cmp metamethod just affects
	// relation operators (<,>,<=,=>,<=>), not equality operators (==,!=).
	// For the time being, we use relation order operator (<=>) and compare its result to 0
	// to make it work.
	// Update: we've fixed this.
	if(seatid == 2 && e.enemy != null && e.enemy.alive && e.enemy.cs == e.cs){
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

if(isClient()){
	local classname = "Sceptor";

	local function controlProc(direction, state){
		local controlled = player.controlled;
		if(controlled)
			controlled.setControl(direction, state);
	}

	register_console_command("+turnleft", @() controlProc("left", true));
	register_console_command("-turnleft", @() controlProc("left", false));
	register_console_command("+turnright", @() controlProc("right", true));
	register_console_command("-turnright", @() controlProc("right", false));
	register_console_command("+turnup", @() controlProc("up", true));
	register_console_command("-turnup", @() controlProc("up", false));
	register_console_command("+turndown", @() controlProc("down", true));
	register_console_command("-turndown", @() controlProc("down", false));
	register_console_command("+turnCW", @() controlProc("rollCW", true));
	register_console_command("-turnCW", @() controlProc("rollCW", false));
	register_console_command("+turnCCW", @() controlProc("rollCCW", true));
	register_console_command("-turnCCW", @() controlProc("rollCCW", false));
	register_console_command("+throttleUp", @() controlProc("throttleUp", true));
	register_console_command("-throttleUp", @() controlProc("throttleUp", false));
	register_console_command("+throttleDown", @() controlProc("throttleDown", true));
	register_console_command("-throttleDown", @() controlProc("throttleDown", false));
	register_console_command("throttleReset", @() controlProc("throttleReset", true));
	register_console_command("toggleFlightAssist", function(){
		local controlled = player.controlled;
		if(controlled){
			controlled.flightAssist = !controlled.flightAssist;
			if(controlled.flightAssist)
				controlled.targetSpeed = controlled.rot.trans(Vec3d(0,0,-1)).sp(controlled.velo);
			else
				controlled.targetThrottle = 0.;
		}
	});

	beginControl[classname] <- function (){
		if("print" in this)
			print("Sceptor::beginControl");
		cmd("pushbind");
		cmd("bind a +turnleft");
		cmd("bind d +turnright");
		cmd("bind w +turnup");
		cmd("bind s +turndown");
		cmd("bind q +turnCW");
		cmd("bind e +turnCCW");
		cmd("bind r +throttleUp");
		cmd("bind f +throttleDown");
		cmd("bind x throttleReset");
		cmd("bind z toggleFlightAssist");
		cmd("r_windows 0");
	}

	endControl[classname] <- function (){
		if("print" in this)
			print("Sceptor::endControl");
		cmd("popbind");
		cmd("r_windows 1");
	}
}
