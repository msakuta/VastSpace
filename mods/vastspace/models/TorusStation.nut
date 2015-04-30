modelScale <- 0.1;

hitRadius <- 130.;

mass <- 1.e8;

maxhealth <- 1.e6;


function drawOverlay(){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-0.70, -0.70);
	glVertex2d(-0.70,  0.70);
	glVertex2d(-0.50,  0.70);
	glVertex2d(-0.50,  0.10);
	glVertex2d( 0.50,  0.10);
	glVertex2d( 0.50,  0.70);
	glVertex2d( 0.70,  0.70);
	glVertex2d( 0.70, -0.70);
	glVertex2d( 0.50, -0.70);
	glVertex2d( 0.50, -0.10);
	glVertex2d(-0.50, -0.10);
	glVertex2d(-0.50, -0.70);
	glEnd();
}

