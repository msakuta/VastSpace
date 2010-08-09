
initUI();

deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, 0., 0.2), 0.05, 3, player.cs);
deltaFormation("Sceptor", 1, Quatd(0,0,0,1), Vec3d(0, 0., 1.2), 0.05, 3, player.cs);

framecount <- 0;

cmd("pause 0");
//cmd("mover tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.setpos(lookrot.trans(Vec3d(0., 0., 1.)));

foreachents(player.cs, function(e){e.command("Halt");});

function frameproc(dt){
	framecount++;

	if(showdt)
		print("DT = " + dt + ", FPS = " + (1. / dt) + ", FC = " + framecount);
}

