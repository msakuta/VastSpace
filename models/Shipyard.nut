hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(0.3, 0.2, 0.500)]
]

function drawOverlay(){
	local cuts = 12;

	glBegin(GL_LINE_LOOP);
	for(local i = 0; i < cuts; i++)
		glVertex2d( 0.00 + 0.20 * cos(i * 2 * PI / 12),  -0.80 + 0.20 * sin(i * 2 * PI / 12));
	glEnd();

	glBegin(GL_LINES);
	glVertex2d( 0.00,  0.75);
	glVertex2d( 0.00, -0.60);
	glVertex2d(-0.30, -0.45);
	glVertex2d( 0.30, -0.45);
	glEnd();

	glBegin(GL_LINE_STRIP);
	glVertex2d(-0.80,  0.20);
	glVertex2d(-0.60,  0.45);
	glVertex2d(-0.40,  0.61);
	glVertex2d(-0.20,  0.75);
	glVertex2d(-0.00,  0.85);
	glVertex2d( 0.20,  0.75);
	glVertex2d( 0.40,  0.61);
	glVertex2d( 0.60,  0.45);
	glVertex2d( 0.80,  0.20);
	glEnd();
}
