local SIN15 = sin(PI/12);
local COS15 = cos(PI/12);

hitbox <- [
	[Vec3d(0., 0., -5.), Quatd(0,0,0,1), Vec3d(15., 14., 60.)],
	[Vec3d(0., 18., 32.5), Quatd(0,0,0,1), Vec3d(10., 4., 15.)],
	[Vec3d(25., -15., 20.), Quatd(0,0, -SIN15, COS15), Vec3d(7.5, 2., 20.)],
	[Vec3d(-25., -15., 20.), Quatd(0,0, SIN15, COS15), Vec3d(7.5, 2., 20.)],
	[Vec3d(0.0, 30., 32.5), Quatd(0,0,0,1), Vec3d(2., 8., 10.)],
]

local SQRT2P2 = sqrt(2)/2;
modelScale <- 0.2;

hitRadius <- 100.;

mass <- 1.e5;
maxhealth <- 3000;
maxshield <- 3000;

accel <- 25.;
maxspeed <- 100.;
angleaccel <- 100.;
maxanglespeed <- 0.4;
capacity <- 50000.;
capacitorGen <- 100.;

hardpoints <- [
	{pos = Vec3d(0.000, 50 * modelScale, -110 * modelScale), rot = Quatd(0, 0, 0, 1), name = "Top Turret"},
	{pos = Vec3d(0.000, -50 * modelScale, -110 * modelScale), rot = Quatd(0, 0, 1, 0), name = "Bottom Turret"},
	{pos = Vec3d(40 * modelScale, 0.000, -225 * modelScale), rot = Quatd(0, 0, -SQRT2P2, SQRT2P2), name = "Right Turret"},
	{pos = Vec3d(-40 * modelScale, 0.000, -225 * modelScale), rot = Quatd(0, 0, SQRT2P2, SQRT2P2), name = "Left Turret"},
]

armCtors <- {
	Standard = ["MTurret", "MTurret", "MTurret", "MTurret"],
	Gunner = ["MTurret", "GatlingTurret", "MTurret", "GatlingTurret"],
}

navlights <- [
	{pos = Vec3d(0, 210 * modelScale, 240 * modelScale), radius = 0.002, pattern = "Constant"},
	{pos = Vec3d(190 * modelScale, -120 * modelScale, 240 * modelScale), radius = 0.002, pattern = "Constant"},
	{pos = Vec3d(-190 * modelScale, -120 * modelScale, 240 * modelScale), radius = 0.002, pattern = "Constant"},
	{pos = Vec3d(0, 0.002 + 50 * modelScale, -60 * modelScale), radius = 0.002, pattern = "Constant"},
	{pos = Vec3d(0, 0.002 + 60 * modelScale, 40 * modelScale), radius = 0.002, pattern = "Constant"},
	{pos = Vec3d(0, -0.002 + -50 * modelScale, -60 * modelScale), radius = 0.002, pattern = "Constant"},
	{pos = Vec3d(0, -0.002 + -60 * modelScale, 40 * modelScale), radius = 0.002, pattern = "Constant"},
//	{pos = Vec3d(0, 210 * modelScale, 240 * modelScale), color = [1,1,1,1], radius = 0.003, pattern = "Step", period = 2.},
]

{
	local flashlights = [];
	foreach(e in navlights){
		local copy = clone e;
		copy.color <- [1,1,1,1];
		copy.radius = 3.;
		copy.pattern = "Step";
		copy.period <- 2.;
		copy.duty <- 0.1;
		flashlights.append(copy);
	}
	navlights.extend(flashlights);
}

function drawOverlay(){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-0.40, -0.25);
	glVertex2d(-0.80, -0.1);
	glVertex2d(-0.80,  0.05);
	glVertex2d(-0.4 ,  0.3);
	glVertex2d( 0.1 ,  0.3);
	glVertex2d( 0.7 ,  0.6);
	glVertex2d( 0.7 ,  0.3);
	glVertex2d( 0.8 ,  0.3);
	glVertex2d( 0.8 , -0.2);
	glVertex2d( 0.7 , -0.2);
	glVertex2d( 0.7 , -0.6);
	glVertex2d( 0.4 , -0.4);
	glVertex2d( 0.3 , -0.25);
	glEnd();
}
