
initUI();

deltaFormation("Assault", 0, Quatd(0,1,0,0), Vec3d(0, 0., -0.025), 0.5, 3, player.cs, null);
deltaFormation("Defender", 1, Quatd(0,0,0,1), Vec3d(0, 0.,  1.7), 0.05, 15, player.cs, function(e){e.command("Deploy");});

framecount <- 0;

cmd("pause 0");
//cmd("mover tactical");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
//player.setpos(lookrot.trans(Vec3d(0., 0., 0.25)));
player.viewdist = 0.25;

function frameproc(dt){
	framecount++;

	if(showdt)
		print("DT = " + dt + ", FPS = " + (1. / dt) + ", FC = " + framecount);
}
