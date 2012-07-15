local SIN15 = sin(PI/12);
local COS15 = cos(PI/12);

hitbox <- [
	[Vec3d(0., -0.0025, 0.050), Quatd(0,0,0,1), Vec3d(0.125, 0.060, 0.150)],
	[Vec3d(0.070, 0.015, -0.165), Quatd(0,0,0,1), Vec3d(0.030, 0.035, 0.065)],
	[Vec3d(-0.070, 0.015, -0.165), Quatd(0,0,0,1), Vec3d(0.030, 0.035, 0.065)],
]

local SQRT2P2 = sqrt(2)/2;
modelScale <- 0.001;

mass <- 2.e8;

accel <- 0.025;
maxspeed <-  0.1;
angleaccel <- 5000000. * 0.1;
maxanglespeed <- 0.2;
capacity <- 200000; // [MJ]
capacitorGen <- 300.; // [MW]

local angle2 = 0.12248933156343207708604124060564;

hardpoints <- [
	{pos = Vec3d(0.070, 0.0425, -0.150), rot = Quatd(0, 0, -sin(angle2), cos(angle2)), name = "Top Left Turret"}
	{pos = Vec3d(-0.070, 0.0425, -0.150), rot = Quatd(0, 0, sin(angle2), cos(angle2)), name = "Top Right Turret"},
	{pos = Vec3d(0.000, -0.060, -0.0075), rot = Quatd(0, 0, 1, 0), name = "Bottom Turret"},
]

navlights <- [
	{pos = Vec3d(0, 80, 120) * modelScale, radius = 0.005},
	{pos = Vec3d(-200, 15, 170) * modelScale, radius = 0.005},
	{pos = Vec3d(-200, 20, 90) * modelScale, radius = 0.005},
	{pos = Vec3d( 200, 15, 170) * modelScale, radius = 0.005},
	{pos = Vec3d( 200, 20, 90) * modelScale, radius = 0.005},
	{pos = Vec3d(-90, 10, -230) * modelScale, radius = 0.005},
	{pos = Vec3d(-60, -20, -230) * modelScale, radius = 0.005},
	{pos = Vec3d( 90, 10, -230) * modelScale, radius = 0.005},
	{pos = Vec3d( 60, -20, -230) * modelScale, radius = 0.005},
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
