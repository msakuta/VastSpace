modelScale <- 0.0002;

baseHitRadius <- 300 * modelScale;

mass <- 2.e7;
containerMass <- 1.e7;

maxhealth <- 15000.;

maxContainerCount <- 6;

containerSize <- 300 * modelScale;

// The real engine position is offset by containerCount * containerSize / 2.
engines <- [
	Vec3d(0, 80, 250) * modelScale,
	Vec3d(75, -25, 250) * modelScale,
	Vec3d(-75, -25, 250) * modelScale,
];

function drawOverlay(){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-0.10,  0.00);
	glVertex2d(-0.05, -0.05 * sqrt(3.));
	glVertex2d( 0.05, -0.05 * sqrt(3.));
	glVertex2d( 0.10,  0.00);
	glVertex2d( 0.05,  0.05 * sqrt(3.));
	glVertex2d(-0.05,  0.05 * sqrt(3.));
	glEnd();
}

