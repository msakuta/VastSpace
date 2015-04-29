local SIN15 = sin(PI/12);
local COS15 = cos(PI/12);

hitbox <- [
	[Vec3d(0, 0, -58.), Quatd(0,0,0,1), Vec3d(220., 55., 300.)],
	[Vec3d(0, -60., 5.), Quatd(0,0,0,1), Vec3d(15., 30., 18.)],
	[Vec3d(0, 60., 5.), Quatd(0,0,0,1), Vec3d(15., 30., 18.)],
]

local SQRT2P2 = sqrt(2)/2;
modelScale <- 1.;

hardpoints <- [
	{pos = Vec3d(100 * modelScale, 55 * modelScale,  175 * modelScale), rot = Quatd(0, 0, 0, 1), name = "Top Left 1 Turret"},
	{pos = Vec3d(100 * modelScale, 55 * modelScale,  75 * modelScale), rot = Quatd(0, 0, 0, 1), name = "Top Left 2 Turret"},
	{pos = Vec3d(100 * modelScale, 55 * modelScale, -25 * modelScale), rot = Quatd(0, 0, 0, 1), name = "Top Left 3 Turret"},
	{pos = Vec3d(180 * modelScale, 50 * modelScale,  175 * modelScale), rot = Quatd(0, 0, sin(atan(1.0 / 8.0) / 2), cos(atan(1.0 / 8.0) / 2)), name = "Top Left 4 Turret"},
	{pos = Vec3d(180 * modelScale, 50 * modelScale,  75 * modelScale), rot = Quatd(0, 0, sin(atan(1.0 / 8.0) / 2), cos(atan(1.0 / 8.0) / 2)), name = "Top Left 5 Turret"},
	{pos = Vec3d(100 * modelScale, -55 * modelScale,  175 * modelScale), rot = Quatd(0, 0, 1, 0), name = "Bottom Left 1 Turret"},
	{pos = Vec3d(100 * modelScale, -55 * modelScale,  75 * modelScale), rot = Quatd(0, 0, 1, 0), name = "Bottom Left 2 Turret"},
	{pos = Vec3d(100 * modelScale, -55 * modelScale, -25 * modelScale), rot = Quatd(0, 0, 1, 0), name = "Bottom Left 3 Turret"},
	{pos = Vec3d(180 * modelScale, -50 * modelScale,  175 * modelScale), rot = Quatd(0, 0, sin((PI - atan(1.0 / 8.0)) / 2), cos((PI - atan(1.0 / 8.0)) / 2)), name = "Bottom Left 4 Turret"},
	{pos = Vec3d(180 * modelScale, -50 * modelScale,  75 * modelScale), rot = Quatd(0, 0, sin((PI - atan(1.0 / 8.0)) / 2), cos((PI - atan(1.0 / 8.0)) / 2)), name = "Bottom Left 5 Turret"},

	{pos = Vec3d(-100 * modelScale, 55 * modelScale,  175 * modelScale), rot = Quatd(0, 0, 0, 1), name = "Top Right 1 Turret"},
	{pos = Vec3d(-100 * modelScale, 55 * modelScale,  75 * modelScale), rot = Quatd(0, 0, 0, 1), name = "Top Right 2 Turret"},
	{pos = Vec3d(-100 * modelScale, 55 * modelScale, -25 * modelScale), rot = Quatd(0, 0, 0, 1), name = "Top Right 3 Turret"},
	{pos = Vec3d(-180 * modelScale, 50 * modelScale,  175 * modelScale), rot = Quatd(0, 0, sin(-atan(1.0 / 8.0) / 2), cos(-atan(1.0 / 8.0) / 2)), name = "Top Right 4 Turret"},
	{pos = Vec3d(-180 * modelScale, 50 * modelScale,  75 * modelScale), rot = Quatd(0, 0, sin(-atan(1.0 / 8.0) / 2), cos(-atan(1.0 / 8.0) / 2)), name = "Top Right 5 Turret"},
	{pos = Vec3d(-100 * modelScale, -55 * modelScale,  175 * modelScale), rot = Quatd(0, 0, 1, 0), name = "Bottom Right 1 Turret"},
	{pos = Vec3d(-100 * modelScale, -55 * modelScale,  75 * modelScale), rot = Quatd(0, 0, 1, 0), name = "Bottom Right 2 Turret"},
	{pos = Vec3d(-100 * modelScale, -55 * modelScale, -25 * modelScale), rot = Quatd(0, 0, 1, 0), name = "Bottom Right 3 Turret"},
	{pos = Vec3d(-180 * modelScale, -50 * modelScale,  175 * modelScale), rot = Quatd(0, 0, sin((PI + atan(1.0 / 8.0)) / 2), cos((PI + atan(1.0 / 8.0)) / 2)), name = "Bottom Right 4 Turret"},
	{pos = Vec3d(-180 * modelScale, -50 * modelScale,  75 * modelScale), rot = Quatd(0, 0, sin((PI + atan(1.0 / 8.0)) / 2), cos((PI + atan(1.0 / 8.0)) / 2)), name = "Bottom Right 5 Turret"},

	{pos = Vec3d(220 * modelScale,  0.000, 50 * modelScale), rot = Quatd(0, 0, -SQRT2P2, SQRT2P2), name = "Right Turret"},
	{pos = Vec3d(-220 * modelScale,  0.000, 50 * modelScale), rot = Quatd(0, 0, SQRT2P2, SQRT2P2), name = "Left Turret"},
]

navlights <- [
	{pos = Vec3d(0, 0, -255 * modelScale), radius = 0.005, pattern = "Constant"},
	{pos = Vec3d(-30 * modelScale, -12 * modelScale, -250 * modelScale), radius = 0.003},
	{pos = Vec3d(-30 * modelScale,  12 * modelScale, -250 * modelScale), radius = 0.003},
	{pos = Vec3d( 30 * modelScale, -12 * modelScale, -250 * modelScale), radius = 0.003},
	{pos = Vec3d( 30 * modelScale,  12 * modelScale, -250 * modelScale), radius = 0.003},
	{pos = Vec3d( 0, 90 * modelScale, -23 * modelScale), radius = 0.003},
	{pos = Vec3d(-54.13 * modelScale, 79.67 * modelScale, 17.88 * modelScale), radius = 0.003},
	{pos = Vec3d( 54.13 * modelScale, 79.67 * modelScale, 17.88 * modelScale), radius = 0.003},
	{pos = Vec3d( 0, -90 * modelScale, -23 * modelScale), radius = 0.003},
	{pos = Vec3d(-54.13 * modelScale, -79.67 * modelScale, 17.88 * modelScale), radius = 0.003},
	{pos = Vec3d( 54.13 * modelScale, -79.67 * modelScale, 17.88 * modelScale), radius = 0.003},
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
