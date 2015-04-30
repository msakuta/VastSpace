

modelScale <- 0.1;

hitRadius <- 10.;

mass <- 10000.;

maxhealth <- 500.;

function drawOverlay(){
	glBegin(GL_LINES);
	glVertex2d(-0.20, -0.50);
	glVertex2d(-0.20,  0.50);
	glVertex2d( 0.20, -0.50);
	glVertex2d( 0.20,  0.50);
	glVertex2d(-0.50, -0.20);
	glVertex2d( 0.50, -0.20);
	glVertex2d(-0.50,  0.20);
	glVertex2d( 0.50,  0.20);
	glEnd();
}

