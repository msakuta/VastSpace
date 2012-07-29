
modelScale <- 1.e-4;

mass <- 16.e3;

gunPos <- Vec3d(0., -16.95 * modelScale, -200. * modelScale);

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(30 * modelScale, 30 * modelScale, 125 * modelScale)],
];
 

function drawOverlay(){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-1.0,  0.2);
	glVertex2d(-1.0, -0.2);
	glVertex2d(-0.3, -0.2);
	glVertex2d(-0.6, -0.6);
	glVertex2d(-0.3, -0.9);
	glVertex2d( 0.0, -0.4);
	glVertex2d( 0.3, -0.9);
	glVertex2d( 0.6, -0.6);
	glVertex2d( 0.3, -0.2);
	glVertex2d( 1.0, -0.2);
	glVertex2d( 1.0,  0.2);
	glVertex2d( 0.3,  0.2);
	glVertex2d( 0.6,  0.6);
	glVertex2d( 0.3,  0.9);
	glVertex2d( 0.0,  0.4);
	glVertex2d(-0.3,  0.9);
	glVertex2d(-0.6,  0.6);
	glVertex2d(-0.3,  0.2);
	glEnd();
}
