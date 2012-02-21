
//initUI();

local earthlo = universe.findcspath("/sol/earth/lo");
if(!earthlo)
	earthlo = universe.findcspath("/earth/lo");
if(earthlo){
	player.cs = earthlo;
	player.setpos(Vec3d(0,0,0));
}


//deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, -0.01, -0.025), 2.025, 2, player.cs, null);
//deltaFormation("Sceptor", 1, Quatd(0,0,0,1), Vec3d(0, -0.01, 0.025), -2.025, 2, player.cs, null);
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
targetcs <- player.cs;
invokes <- 0;
obj1 <- null;
obj2 <- null;

function frameproc(dt){
	framecount++;
	local global_time = universe.global_time;

	if(showdt){
		local counte = 0, countb = 0;
		local entl = [];
		foreachents(targetcs, @(e) (/*print(e + ": " + e.health),*/ counte++) );
		foreachbullets(targetcs, @(e) (/*print(e + ": " + e.health),*/ countb++) );
		print("DT = " + dt + ", FPS = " + (1. / dt) + ", FC = " + framecount + " counte = " + counte + ", countb = " + countb);
	}

	local currenttime = universe.global_time + 9.;

	if(false){
		if(checktime + 2. < currenttime){
			checktime = currenttime;
			switch(invokes){
				case 0:
					obj1 = player.cs.addent("Sceptor", Vec3d(0.1, 0., 0.));
					print("obj1 is " + obj1);
					obj2 = player.cs.addent("Sceptor", Vec3d(-0.1, 0., 0.));
					print("obj2 is " + obj2);
					obj1.enemy = obj2;
					print("obj1.enemy is " + obj1.enemy);
					break;
				case 1:
					obj1.kill();
					print("obj1 is deleted " + obj1);
					break;
				case 2:
					print("obj1 is " + obj1);
					if(obj1)
						print("obj1 is " + (obj1.alive ? "" : "not ") + "alive");
					obj2.kill();
					print("obj2 is deleted " + obj2);
					break;
			}
			invokes++;
		}
	}
	else if(true && checktime + 10. < currenttime){
		local cs = targetcs;
		checktime = currenttime;

		// Force the players to chase someone
		local pls = players();
		print("Players = " + pls.len());
		local i = 0;
		for(; i < pls.len(); i++){
			local pl = pls[i];
			pl.cs = targetcs;

			if(pl.chase == null){
				local e;
				for(e = cs.entlist; e != null; e = e.next){
					if(e.race == i){
						print("Set players[" + i + "].chase to " + e);
						pl.chase = e;
						pl.chasecamera = 1;
						pl.setrot(Quatd(0,0,0,1));
						break;
					}
				}
			}
		}

		local racec = [countents(cs, 0, "Sceptor"), countents(cs, 1, "Sceptor")];

		print("time " + currenttime + ": " + racec[0] + ", " + racec[1]);

		if(racec[0] < 2){
			deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, -0.01, -2.025), 0.025, 2, player.cs, null);
		}
		if(racec[1] < 2){
			deltaFormation("Sceptor", 1, Quatd(0,0,0,1), Vec3d(0, -0.01, 2.025), 0.025, 2, player.cs, null);
		}


		foreach(key,value in deaths) foreach(key1,value1 in value)
			print("[team" + key + "][" + key1 + "] " + value1);
	}
}


