
modelScale <- 0.001 / 30.0;

hitRadius <- 0.012;

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
	-1.5, 0, 0,
	0, -0.05, 0,
	0, 0., -0.015
]

wings <- [
	{name = "MainRight", pos = Vec3d( 0.004, 0.001, 0.0), aero = tensor1,
		control = "aileron", sensitivity = -0.05 * PI},
	{name = "MainLeft", pos = Vec3d(-0.004, 0.001, 0.0), aero = tensor1,
		control = "aileron", sensitivity = 0.05 * PI},
	{name = "TailRight", pos = Vec3d( 0.003, 0.0, 0.008), aero = tensor2,
		control = "elevator", sensitivity = -0.1 * PI},
	{name = "TailLeft", pos = Vec3d(-0.003, 0.0, 0.008), aero = tensor2,
		control = "elevator", sensitivity = -0.1 * PI},
	{name = "VerticalLeft", pos = Vec3d( 0.0020, 0.002, 0.007), aero = tensor3,
		control = "rudder", sensitivity = -0.1 * PI, axis = Vec3d(0,1,0)},
	{name = "VerticalRight", pos = Vec3d(-0.0020, 0.002, 0.007), aero = tensor3,
		control = "rudder", sensitivity = -0.1 * PI, axis = Vec3d(0,1,0)},
]


wingTips <- [
	Vec3d(-215., 9., 125.) * modelScale,
	Vec3d( 215., 9., 125.) * modelScale,
]

gunPositions <- [
	Vec3d(20, 12, -200) * modelScale
]

bulletSpeed <- 0.78;

shootCooldown <- 0.07;

cameraPositions <- [
	Vec3d(0., 30 * modelScale, -150 * modelScale),
	Vec3d(0., 0.0075, 0.025),
	Vec3d(0.020, 0.007, 0.050),
	Vec3d(0.010, 0.007, -0.010),
	Vec3d(0.010, 0.007, -0.010),
	Vec3d(0.004, 0.0, 0.0),
	Vec3d(-0.004, 0.0, 0.0),
]

hudPos <- Vec3d(0., 30.0, -170.0) * modelScale

hudSize <- 10 * modelScale


function drawOverlay(){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-1.0,  0.0);
	glVertex2d(-0.5,  0.2);
	glVertex2d( 0.0,  0.2);
	glVertex2d( 0.5,  0.7);
	glVertex2d( 0.7,  0.5);
	glVertex2d( 0.5,  0.2);
	glVertex2d( 0.7,  0.2);
	glVertex2d( 0.8,  0.4);
	glVertex2d( 1.0,  0.4);
	glVertex2d( 0.9,  0.2);
	glVertex2d( 0.9, -0.2);
	glVertex2d( 1.0, -0.4);
	glVertex2d( 0.8, -0.4);
	glVertex2d( 0.7, -0.2);
	glVertex2d( 0.5, -0.2);
	glVertex2d( 0.7, -0.5);
	glVertex2d( 0.5, -0.7);
	glVertex2d( 0.0, -0.2);
	glVertex2d(-0.5, -0.2);
	glEnd();
}
