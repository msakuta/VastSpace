
modelScale <- 2.e-6;

mass <- 60.;

accel <- 0.01;
maxspeed <- 0.01;
angleaccel <- PI * 0.1;
maxanglespeed <- PI * 0.1;
capacity <- 1.;
capacitorGen <- 1.;

function drawOverlay(){
	local cuts = 16;
	local headpos = -0.5;
	local headrad = 0.2;

	glBegin(GL_LINE_LOOP);
	for(local i = 0; i < cuts; i++)
		glVertex2d(headrad * cos(2 * PI * i / cuts), headpos + headrad * sin(2 * PI * i / cuts));
	glEnd();

	glBegin(GL_LINES);

	glVertex2d( 0.00,  headpos + headrad);
	glVertex2d( 0.00,  0.50);

	glVertex2d(-0.50, -0.00);
	glVertex2d( 0.50, -0.00);

	glVertex2d( 0.00,  0.50);
	glVertex2d(-0.30,  0.80);

	glVertex2d( 0.00,  0.50);
	glVertex2d( 0.30,  0.80);

	glEnd();
}
