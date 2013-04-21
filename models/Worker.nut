
modelScale <- 2.e-5;

mass <- 4.e3;

maxhealth <- 200.; // Show some guts for demonstrating shooting effect in the client.

maxfuel <- 120.; // seconds for full thrust

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(0.005, 0.005, 0.003)],
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
