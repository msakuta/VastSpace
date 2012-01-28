
//initUI();

local earthlo = universe.findcspath("/sol/earth/lo");
if(!earthlo)
	earthlo = universe.findcspath("/earth/lo");
if(earthlo){
	player.cs = earthlo;
	player.setpos(Vec3d(0,0,0));
}


deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, -0.01, -0.025), 2.025, 2, player.cs, null);
deltaFormation("Sceptor", 1, Quatd(0,0,0,1), Vec3d(0, -0.01, 0.025), -2.025, 2, player.cs, null);
//deltaFormation("Defender", 1, Quatd(0,0,0,1), Vec3d(0, 0.,  1.7), 0.05, 1, player.cs, function(e){e.command("Deploy");});

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
//player.setmover("tactical");

/*
local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 0.25;
*/

showdt <- false;
framecount <- 0;
checktime <- 0;
autochase <- true;
deaths <- {};

function frameproc(dt){
	framecount++;
	local global_time = universe.global_time;

	if(showdt)
		print("DT = " + dt + ", FPS = " + (1. / dt) + ", FC = " + framecount);

	local currenttime = universe.global_time + 9.;

	if(true && checktime + 10. < currenttime){
		local cs = player.cs;
		checktime = currenttime;

		local racec = [countents(cs, 0, "Sceptor"), countents(cs, 1, "Sceptor")];

		print("time " + currenttime + ": " + racec[0] + ", " + racec[1]);

		if(racec[0] < 2){
			deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, -0.01, -0.025), 2.025, 2, player.cs, null);
		}
		if(racec[1] < 2){
			deltaFormation("Sceptor", 1, Quatd(0,0,0,1), Vec3d(0, -0.01, 0.025), -2.025, 2, player.cs, null);
		}


		foreach(key,value in deaths) foreach(key1,value1 in value)
			print("[team" + key + "][" + key1 + "] " + value1);
	}
}


