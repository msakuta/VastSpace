
modelScale <- 0.001 / 30.0;

mass <- 12.e3;

maxhealth <- 500.;

maxfuel <- 120.; // seconds for full thrust

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(0.005, 0.002, 0.003)],
];


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
