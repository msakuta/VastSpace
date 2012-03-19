local SIN15 = sin(PI/12);
local COS15 = cos(PI/12);

hitbox <- [
	[Vec3d(0., 0., -0.005), Quatd(0,0,0,1), Vec3d(0.015, 0.015, 0.060)],
	[Vec3d(0.025, -0.015, 0.02), Quatd(0,0, -SIN15, COS15), Vec3d(0.0075, 0.002, 0.02)],
	[Vec3d(-0.025, -0.015, 0.02), Quatd(0,0, SIN15, COS15), Vec3d(0.0075, 0.002, 0.02)],
	[Vec3d(0.0, 0.03, 0.0325), Quatd(0,0,0,1), Vec3d(0.002, 0.008, 0.010)],
]

function drawOverlay(){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-0.40, -0.25);
	glVertex2d(-0.80, -0.10);
	glVertex2d(-0.50, -0.10);
	glVertex2d(-0.50, -0.05);
	glVertex2d(-1.00, -0.05);
	glVertex2d(-1.00,  0.05);
	glVertex2d(-0.50,  0.05);
	glVertex2d(-0.50,  0.1);
	glVertex2d(-0.80,  0.1);
	glVertex2d(-0.4 ,  0.3);
	glVertex2d( 0.1 ,  0.3);
	glVertex2d( 0.7 ,  0.6);
	glVertex2d( 0.7 ,  0.3);
	glVertex2d( 0.8 ,  0.3);
	glVertex2d( 0.8 , -0.2);
	glVertex2d( 0.7 , -0.2);
	glVertex2d( 0.7 , -0.6);
	glVertex2d( 0.4 , -0.4);
	glVertex2d( 0.3 , -0.25);
	glEnd();
}
