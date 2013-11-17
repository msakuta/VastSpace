
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

if(isClient()){
	::reloadCommand <- function(){
		player.controlled.command("Reload");
	}

	::getCoverCommand <- function(){
		player.controlled.command("GetCover", 2);
	}

	::transformCommand <- function(){
		player.controlled.waverider = !player.controlled.waverider;
	}

	::stabilizerCommand <- function(){
		player.controlled.stabilizer = !player.controlled.stabilizer;
	}

	beginControl["ReZEL"] <- function (){
		if("print" in this)
			print("ReZEL::beginControl");
		cmd("pushbind");
		cmd("bind r sq \"reloadCommand()\"");
		cmd("bind t sq \"getCoverCommand()\"");
		cmd("bind b sq \"transformCommand()\"");
		cmd("bind g sq \"stabilizerCommand()\"");
		cmd("r_windows 0");
	}

	endControl["ReZEL"] <- function (){
		if("print" in this)
			print("ReZEL::endControl");
		cmd("popbind");
		cmd("r_windows 1");
	}
}
