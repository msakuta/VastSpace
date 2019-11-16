local SIN15 = sin(PI/12);
local COS15 = cos(PI/12);

hitbox <- [
	[Vec3d(0., -2.5, 50.), Quatd(0,0,0,1), Vec3d(125., 60., 150.)],
	[Vec3d(70., 15., -165.), Quatd(0,0,0,1), Vec3d(30., 35., 65.)],
	[Vec3d(-70., 15., -165.), Quatd(0,0,0,1), Vec3d(30., 35., 65.)],
]

local SQRT2P2 = sqrt(2)/2;
modelScale <- 1.;

mass <- 2.e8;

accel <- 25.;
maxspeed <-  100.;
angleaccel <- 5000000. * 0.1;
maxanglespeed <- 0.2;
capacity <- 200000; // [MJ]
capacitorGen <- 300.; // [MW]

local angle2 = 0.12248933156343207708604124060564;

hardpoints <- [
	{pos = Vec3d(70., 42.5, -150.), rot = Quatd(0, 0, -sin(angle2), cos(angle2)), name = "Top Left Turret"}
	{pos = Vec3d(-70., 42.5, -150.), rot = Quatd(0, 0, sin(angle2), cos(angle2)), name = "Top Right Turret"},
	{pos = Vec3d(0., -60., -7.5), rot = Quatd(0, 0, 1, 0), name = "Bottom Turret"},
]

armCtor <- ["LTurret", "LTurret", "LMissileTurret"]

navlights <- [
	{pos = Vec3d(0, 80, 120) * modelScale, radius = 5.},
	{pos = Vec3d(-200, 15, 170) * modelScale, radius = 5.},
	{pos = Vec3d(-200, 20, 90) * modelScale, radius = 5.},
	{pos = Vec3d( 200, 15, 170) * modelScale, radius = 5.},
	{pos = Vec3d( 200, 20, 90) * modelScale, radius = 5.},
	{pos = Vec3d(-90, 10, -230) * modelScale, radius = 5.},
	{pos = Vec3d(-60, -20, -230) * modelScale, radius = 5.},
	{pos = Vec3d( 90, 10, -230) * modelScale, radius = 5.},
	{pos = Vec3d( 60, -20, -230) * modelScale, radius = 5.},
]

function drawOverlay(){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-0.15,  1.0);
	glVertex2d(-0.4,  1.0);
	glVertex2d(-0.5,  0.9);
	glVertex2d(-0.5,  0.2);
	glVertex2d(-0.9, -0.5);
	glVertex2d(-0.9, -0.7);
	glVertex2d(-0.5, -0.8);
	glVertex2d(-0.5, -0.9);
	glVertex2d(-0.2, -1.0);
	glVertex2d( 0.2, -1.0);
	glVertex2d( 0.5, -0.9);
	glVertex2d( 0.5, -0.8);
	glVertex2d( 0.9, -0.7);
	glVertex2d( 0.9, -0.5);
	glVertex2d( 0.5,  0.2);
	glVertex2d( 0.5,  0.9);
	glVertex2d( 0.4,  1.0);
	glVertex2d( 0.15,  1.0);
	glVertex2d( 0.15,  0.3);
	glVertex2d(-0.15,  0.3);
	glEnd();
}
