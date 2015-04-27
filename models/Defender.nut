
modelScale <- 1.e-1;

mass <- 16.e3;

accel <- 100.;
maxspeed <- 100.;
angleaccel <- PI * 0.1;
maxanglespeed <- PI * 0.1;
capacity <- 1.;
capacitorGen <- 1.;


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

// MQO model object names for engine shafts
engineName0 <- "defender0_engine";
engineName1 <- "defender0_engine1";
engineName2 <- "defender0_engine2";
engineName3 <- "defender0_engine3";

