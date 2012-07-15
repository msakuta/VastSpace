local SIN15 = sin(PI/12);
local COS15 = cos(PI/12);

hitbox <- [
	[Vec3d(0., 0., -0.005), Quatd(0,0,0,1), Vec3d(0.015, 0.015, 0.060)],
	[Vec3d(0.025, -0.015, 0.02), Quatd(0,0, -SIN15, COS15), Vec3d(0.0075, 0.002, 0.02)],
	[Vec3d(-0.025, -0.015, 0.02), Quatd(0,0, SIN15, COS15), Vec3d(0.0075, 0.002, 0.02)],
	[Vec3d(0.0, 0.03, 0.0325), Quatd(0,0,0,1), Vec3d(0.002, 0.008, 0.010)],
]

modelScale <- 0.0002;

mass <- 1.e5;

accel <- 0.025;
maxspeed <- 0.1;
angleaccel <- 100.;
maxanglespeed <- 0.4;
capacity <- 50000.;
capacitorGen <- 100.;

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
		copy.radius = 0.003;
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
	glVertex2d(-0.80, -0.10);
	glVertex2d(-0.50, -0.10);
	glVertex2d(-0.50, -0.05);
	glVertex2d(-1.00, -0.05);
	glVertex2d(-1.00,  0.05);
	glVertex2d(-0.50,  0.05);
	glVertex2d(-0.50,  0.1);
	glVertex2d(-0.80,  0.1);
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
