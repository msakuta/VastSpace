
modelScale <- 0.001 / 30.0;

mass <- 12.e3;

maxhealth <- 500.;

maxfuel <- 120.; // seconds for full thrust

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(13.05, 5.63, 19.43) * 0.5e-3],
];

// Thrust power
thrust <- 0.01

// Aerodynamic tensors, referred to by wings.
local tensor1 = [
	-0.1, 0, 0,
	0, -6.5, 0,
	0, -0.3, -0.025
]

local tensor2 = [
	-0.1, 0, 0,
	0, -1.9, 0,
	0, -0.0, -0.015
]

local tensor3 = [
	-0.9, 0, 0,
	0, -0.05, 0,
	0, 0., -0.015
]

wings <- [
	{pos = Vec3d( 0.004, 0.001, 0.0), aero = tensor1},
	{pos = Vec3d(-0.004, 0.001, 0.0), aero = tensor1},
	{pos = Vec3d( 0.003, 0.002, 0.005), aero = tensor2},
	{pos = Vec3d(-0.003, 0.002, 0.005), aero = tensor2},
	{pos = Vec3d( 0.000, 0.0045, 0.006), aero = tensor3},
]




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
