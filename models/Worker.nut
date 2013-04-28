
modelScale <- 2.e-5;

mass <- 2.e3;

maxhealth <- 100.; // We are not designed for combat

maxfuel <- 120.; // seconds for full thrust

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(0.003, 0.003, 0.004)],
];

enginePos <- [];

for(local ix = -1; ix <= 1; ix += 2) for(local iy = -1; iy <= 1; iy += 2)
	enginePos.append(Vec3d(ix * 73.28, iy * 127.59, 63.10) * modelScale);

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
