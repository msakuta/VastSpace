
hitbox <- [
	[Vec3d(0., 0., 0.), Quatd(0,0,0,1), Vec3d(0.0003, 0.0008, 0.0003)],
]

modelScale <- 2.e-6;

hitRadius <- 0.001;

mass <- 60;

accel <- 0.005;
maxspeed <- 0.005;
angleaccel <- PI * 0.1;
maxanglespeed <- PI * 0.1;
capacity <- 1.;
capacitorGen <- 1.;

hookSpeed <- 0.2;
hookRange <- 0.2;
hookPullAccel <- 0.05;
hookStopRange <- 0.025;

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


muzzleFlashRadius1 <- 0.0004; // Radius in kilometers
muzzleFlashOffset1 <- Vec3d(-0.00080, 0.00020, 0.0); // Offset from hand
muzzleFlashRadius2 <- 0.00025;
muzzleFlashOffset2 <- Vec3d(-0.00110, 0.00020, 0.0);

if(!isServer()){
register_console_command("reload", function(){
	if(player.controlled)
		player.controlled.command("ReloadWeapon");
});


beginControl["Soldier"] <- function (){
	if("print" in this)
		print("Soldier::beginControl");
	cmd("pushbind");
	cmd("bind r reload");
}

endControl["Soldier"] <- function (){
	if("print" in this)
		print("Soldier::endControl");
	cmd("popbind");
}
}
