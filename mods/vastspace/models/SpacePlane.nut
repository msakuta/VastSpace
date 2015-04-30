modelScale <- 0.4;

hitRadius <- 80.;

mass <- 2.e7;

engines <- [
	Vec3d(0, 0, 155) * modelScale,
	Vec3d(75, 0, 135) * modelScale,
	Vec3d(-75, 0, 135) * modelScale,
];

maxhealth <- 15000.;


function drawOverlay(){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-0.10,  0.00);
	glVertex2d(-0.05, -0.03);
	glVertex2d( 0.00, -0.03);
	glVertex2d( 0.08, -0.07);
	glVertex2d( 0.10, -0.03);
	glVertex2d( 0.10,  0.03);
	glVertex2d( 0.08,  0.07);
	glVertex2d( 0.00,  0.03);
	glVertex2d(-0.05,  0.03);
	glEnd();
}

