
initUI();

deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0, 0., -0.2), 0.1, 3, player.cs);
deltaFormation("Sceptor", 1, Quatd(0,1,0,0), Vec3d(0, 0., 1.2), 0.1, 3, player.cs);

framecount <- 0;

cmd("pause 0");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.));
player.setrot(lookrot);
player.setpos((Quatd(0., 1., 0., 0.) * Quatd(sin(PI/8.), 0, 0, cos(PI/8.))).trans(Vec3d(0., 0., -1.)));


function frameproc(dt){
	framecount++;

	if(showdt)
		print("DT = " + dt + ", FPS = " + (1. / dt) + ", FC = " + framecount);
}

