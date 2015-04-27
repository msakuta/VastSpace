local SIN15 = sin(PI/12);
local COS15 = cos(PI/12);

hitbox <- [
	[Vec3d(0, 0, -58.), Quatd(0,0,0,1), Vec3d(50., 32., 190.)],
	[Vec3d(0, 0, 165.), Quatd(0,0,0,1), Vec3d(50., 45., 35.)],
	[Vec3d(0, -60., 5.), Quatd(0,0,0,1), Vec3d(15., 30., 18.)],
	[Vec3d(0, 60., 5.), Quatd(0,0,0,1), Vec3d(15., 30., 18.)],
]

local SQRT2P2 = sqrt(2)/2;
modelScale <- 1.;

mass <- 1.e8;
maxhealth <- 50000;

accel <- 50.;
maxspeed <-  100.;
angleaccel <- 10000. * 0.1;
maxanglespeed <- 0.2;
capacity <- 150000; // [MJ]
capacitorGen <- 300.; // [MW]


hardpoints <- [
	{pos = Vec3d(0.000, 29 * modelScale, -175 * modelScale), rot = Quatd(0, 0, 0, 1), name = "Top Front Turret"},
	{pos = Vec3d(0.000, 32 * modelScale, -80 * modelScale), rot = Quatd(0, 0, 0, 1), name = "Top Back Turret"},
	{pos = Vec3d(0.000, -29 * modelScale, -175 * modelScale), rot = Quatd(0, 0, 1, 0), name = "Bottom Front Turret"},
	{pos = Vec3d(0.000, -32 * modelScale, -80 * modelScale), rot = Quatd(0, 0, 1, 0), name = "Bottom Back Turret"},
	{pos = Vec3d(52 * modelScale,  0.000, 80 * modelScale), rot = Quatd(0, 0, -SQRT2P2, SQRT2P2), name = "Right Turret"},
	{pos = Vec3d(-52 * modelScale,  0.000, 80 * modelScale), rot = Quatd(0, 0, SQRT2P2, SQRT2P2), name = "Left Turret"},
]

armCtors <- {
	Standard = ["LTurret", "LTurret", "LTurret", "LTurret", "LTurret", "LTurret"],
	Missile = ["LMissileTurret", "LTurret", "LMissileTurret", "LTurret", "LTurret", "LTurret"],
}

navlights <- [
	{pos = Vec3d(0, 0, -255 * modelScale), radius = 0.005, pattern = "Constant"},
	{pos = Vec3d(-30 * modelScale, -12 * modelScale, -250 * modelScale), radius = 3},
	{pos = Vec3d(-30 * modelScale,  12 * modelScale, -250 * modelScale), radius = 3},
	{pos = Vec3d( 30 * modelScale, -12 * modelScale, -250 * modelScale), radius = 3},
	{pos = Vec3d( 30 * modelScale,  12 * modelScale, -250 * modelScale), radius = 3},
	{pos = Vec3d( 0, 90 * modelScale, -23 * modelScale), radius = 3},
	{pos = Vec3d(-54.13 * modelScale, 79.67 * modelScale, 17.88 * modelScale), radius = 3},
	{pos = Vec3d( 54.13 * modelScale, 79.67 * modelScale, 17.88 * modelScale), radius = 3},
	{pos = Vec3d( 0, -90 * modelScale, -23 * modelScale), radius = 3},
	{pos = Vec3d(-54.13 * modelScale, -79.67 * modelScale, 17.88 * modelScale), radius = 3},
	{pos = Vec3d( 54.13 * modelScale, -79.67 * modelScale, 17.88 * modelScale), radius = 3},
]

function drawOverlay(){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-0.8, -0.5);
	glVertex2d(-0.8, -0.2);
	glVertex2d( 0.0,  0.4);
	glVertex2d( 0.8, -0.2);
	glVertex2d( 0.8, -0.5);
	glVertex2d( 0.0,  0.1);
	glEnd();
}
