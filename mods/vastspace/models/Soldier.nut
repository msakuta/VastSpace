
hitbox <- [
	[Vec3d(0., 0., 0.), Quatd(0,0,0,1), Vec3d(0.3, 0.8, 0.3)],
]

modelScale <- 2.e-3;

hitRadius <- 1.;

mass <- 60;

maxhealth <- 10.;

accel <- 5.;
maxspeed <- 5.;
angleaccel <- PI * 0.1;
maxanglespeed <- PI * 0.1;
capacity <- 1.;
capacitorGen <- 1.;

hookSpeed <- 200.;
hookRange <- 200.;
hookPullAccel <- 50.;
hookStopRange <- 25.;

hardpoints <- [
	{pos = Vec3d(0,0,0), rot = Quatd(0, 0, 0, 1), name = "Armed weapon"},
	{pos = Vec3d(0,0,0), rot = Quatd(0, 1, 0, 0), name = "Stocked weapon"},
]

function drawOverlay(){
	local cuts = 16;
	local headpos = -0.5;
	local headrad = 0.2;

	glBegin(GL_LINE_LOOP);
	for(local i = 0; i < cuts; i++)
		glVertex2d(headrad * cos(2 * PI * i / cuts), headpos + headrad * sin(2 * PI * i / cuts));
	glEnd();

	glBegin(GL_LINES);

	glVertex2d( 0.00,  headpos + headrad);
	glVertex2d( 0.00,  0.50);

	glVertex2d(-0.50, -0.00);
	glVertex2d( 0.50, -0.00);

	glVertex2d( 0.00,  0.50);
	glVertex2d(-0.30,  0.80);

	glVertex2d( 0.00,  0.50);
	glVertex2d( 0.30,  0.80);

	glEnd();
}


muzzleFlashRadius1 <- 0.4; // Radius in meters
muzzleFlashOffset1 <- Vec3d(-0.80, 0.20, 0.0); // Offset from hand
muzzleFlashRadius2 <- 0.25;
muzzleFlashOffset2 <- Vec3d(-1.10, 0.20, 0.0);

if(isClient()){
register_console_command("reload", function(){
	if(player.controlled)
		player.controlled.command("ReloadWeapon");
});

register_console_command("jump", function(){
	if(player.controlled)
		player.controlled.command("Jump");
});


beginControl["Soldier"] <- function (){
	if("print" in this)
		print("Soldier::beginControl");
	cmd("pushbind");
	cmd("bind r reload");
	cmd("bind space jump");
	cmd("r_windows 0");
}

endControl["Soldier"] <- function (){
	if("print" in this)
		print("Soldier::endControl");
	cmd("popbind");
	cmd("r_windows 1");
}
}
