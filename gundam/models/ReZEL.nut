
modelScale <- 1./30000;

mass <- 4.e3;

maxhealth <- 100.;

bulletSpeed <- 2.;
walkSpeed <- 0.05;
reloadTime <- 5.;
rifleMagazineSize <- 5;
vulcanReloadTime <- 5.;
vulcanMagazineSize <- 20;

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(0.005, 0.01025, 0.005)],
];

function drawOverlay(){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-0.10, -0.10);
	glVertex2d(-0.05,  0.00);
	glVertex2d(-0.10,  0.10);
	glVertex2d( 0.00,  0.05);
	glVertex2d( 0.10,  0.10);
	glVertex2d( 0.05,  0.00);
	glVertex2d( 0.10, -0.10);
	glVertex2d( 0.00, -0.05);
	glEnd();
}
