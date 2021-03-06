
//initUI();

local earthlo = universe.findcspath("/sol/earth/lo");
if(!earthlo)
	earthlo = universe.findcspath("/earth/lo");
if(earthlo){
	player.cs = earthlo;
	player.setpos(Vec3d(-1,0,0));
}

if(1){
	redbase <- player.cs.addent("Shipyard", Vec3d(2.e3, 0, 0));
//	redbase.race = 1;
	local des = player.cs.addent("Destroyer", Vec3d(2.e3, 0, 500.));
//	des.race = 1;
	des.setrot(Quatd.rotation(PI/2., Vec3d(0,1,0)));
	local attacker = player.cs.addent("Attacker", Vec3d(2.e3, 0, 1.e3));
	attacker.setrot(Quatd.rotation(3.0*PI/2.0, Vec3d(0,0,1)));
/*	local assault = player.cs.addent("Assault", Vec3d(2.0, -0.30, 0.75));
	assault.setrot(Quatd.rotation(2.0*PI/3.0, Vec3d(0,1,0)));
	local beamer = player.cs.addent("Beamer", Vec3d(2.0, -0.30, 0.50));
	beamer.setrot(Quatd.rotation(2.0*PI/3.0, Vec3d(0,1,0)));*/
//	soldier <- player.cs.addent("Soldier", Vec3d(2.0, -0.2, 0.5));
//	soldier.race = 0;
//	local e2 = player.cs.addent("Soldier", Vec3d(-2.0, 0, 0.5));
//	e2.race = 1;
	local e = redbase;
	e.setrot(Quatd.rotation(PI/2., Vec3d(0,1,0)));
//	e.race = 1;

	for(local i = 0; i < 0; i++){
		local turret = player.cs.addent("GimbalTurret", Vec3d(0,0 + 50. * i,50.));
		turret.race = 1;
	}
	for(local i = 0; i < 0; i++){
		local mturret = player.cs.addent("MissileGimbalTurret", Vec3d(0,0 + i * 50.,-50.));
		mturret.race = 1;
	}
	for(local i = 0; i < 0; i++){
		local bturret = player.cs.addent("BeamGimbalTurret", Vec3d(50.,0 + 50. * i, -100.));
		bturret.race = 1;
	}
}
if(1){
	local e = player.cs.addent("Shipyard", Vec3d(-2.e3, 0, 0));
	bluebase <- e;
	e.setrot(Quatd.rotation(-PI/2., Vec3d(0,1,0)));
	e.race = 0;
}

//deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, -0.01, -0.025), 2.025, 2, player.cs, null);
//deltaFormation("Sceptor", 1, Quatd(0,0,0,1), Vec3d(0, -0.01, 0.025), -2.025, 2, player.cs, null);
//deltaFormation("Defender", 1, Quatd(0,0,0,1), Vec3d(0, 0.,  1.7), 0.05, 3, player.cs, function(e){e.command("Deploy");});

universe.paused = false;
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
//player.setmover("tactical");

/*
local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 0.25;
*/

function drand(){
	return rand().tofloat() / RAND_MAX;
}

function gaussRand(){
	return drand() + drand() - 1.0;
}

showdt <- false;
framecount <- 0;
checktime <- 0;
autochase <- true;
deaths <- {};
targetcs <- player.cs;
invokes <- 0;
obj1 <- null;
obj2 <- null;
assaults <- 0;

numsol0 <- 0;
numsol <- 0;

old_frameproc <- "frameproc" in this ? frameproc : null;

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
					obj1 = player.cs.addent("Sceptor", Vec3d(100., 0., 0.));
					print("obj1 is " + obj1);
					obj2 = player.cs.addent("Sceptor", Vec3d(-100., 0., 0.));
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

			// Force the players to control a Soldier.
/*			if(pl.controlled == null){
				local soldier = player.cs.addent("Soldier", Vec3d(1.5 + gaussRand() * 0.003, -0.2 + gaussRand() * 0.003, 0.5 + gaussRand() * 0.003 + i * 0.2));
				soldier.setrot(Quatd.rotation(-PI / 2., Vec3d(0,1,0)));
				soldier.race = i;
				pl.controlled = soldier;
			}*/

			// Force the players to chase a Soldier.
			if(pl.chase == null){
				foreachents(player.cs, function(e){
					if(e.classname == "Soldier" && e.race == i)
						pl.chase = e;
				});
			}

//			if(pl.chase == null){
//				foreach(e in cs.entlist){
//					if(e.classname == "Sceptor" && e.race == i){
//						print("Set players[" + i + "].chase to " + e);
//						pl.chase = e;
//						pl.chasecamera = 1;
//						pl.setrot(Quatd(0,0,0,1));
//						break;
//					}
//				}
//			}
		}

		if(bluebase.alive)
			bluebase.ru += 75;
		if(redbase.alive)
			redbase.ru += 75;

		local racec = [countents(cs, 0, "Sceptor"), countents(cs, 1, "Sceptor")];

		print("time " + currenttime + ": " + racec[0] + ", " + racec[1]);

		if(true && racec[0] < 2){
			if(bluebase.alive)
				bluebase.command("Build", "Interceptor");
//			local d = bluebase.docker;
//			if(d != null){
//				local e = d.addent("Sceptor");
//				e.race = 0;
//			}
		}
		if(true && racec[1] < 2){
			if(redbase.alive){
				redbase.command("Build", "Interceptor");
				local d = redbase.docker;
				if(d != null){
					local e = d.addent("Sceptor");
					e.race = 1;
				}
			}
		}
		if(false && assaults < 1 && countents(cs, 0, "Assault") < 1){
			local docker = bluebase.docker;
			if(docker != null){
				local be = docker.addent("Assault");
				be.race = 0;
			}
			assaults++;
		}
		if(false && countents(cs, 0, "Defender") < 2){
			local docker = bluebase.docker;
			if(docker != null){
				local be = docker.addent("Defender");
				be.race = 0;
			}
		}
		if(false && racec[1] < 20){
			local d = redbase.docker;
			if(d != null){
				local e = d.addent("Sceptor");
				e.race = 1;
			}
		}
		if(false && countents(cs, 1, "Beamer") < 1){
			local docker = redbase.docker;
			if(docker != null){
				local be = docker.addent("Beamer");
				be.race = 1;
			}
		}
		while(true && countents(cs, 0, "Soldier") < 1){
			local soldier = player.cs.addent("Soldier", Vec3d(2.0 + (drand() - 0.5) * 300., -200. + gaussRand() * 5., 400. + (drand() - 0.5) * 100.));
			soldier.setrot(Quatd(gaussRand(), gaussRand(), gaussRand(), gaussRand()).norm());
			soldier.race = 0;
			numsol0 = (numsol0 + 1) % 10;
			if(!player.chase)
				player.chase = soldier;
		}
		while(false && countents(cs, 1, "Soldier") < 1){
			local soldier = player.cs.addent("Soldier", Vec3d(2.0 + (drand() - 0.5) * 300., -200. + gaussRand() * 5., 650. + (drand() - 0.5) * 100.));
			soldier.setrot(Quatd(gaussRand(), gaussRand(), gaussRand(), gaussRand()).norm());
			soldier.race = 1;
			numsol = (numsol + 1) % 10;
		}


		foreach(key,value in deaths) foreach(key1,value1 in value)
			print("[team" + key + "][" + key1 + "] " + value1);
	}

	if(old_frameproc != null)
		old_frameproc(dt);
}

function hook_delete_Entity(e){
	if(!(e.race in deaths))
		deaths[e.race] <- {};
	if(!(e.classname in deaths[e.race]))
		deaths[e.race][e.classname] <- 0;
	deaths[e.race][e.classname]++;
//	print(e + " pos:" + e.getpos() + " is deleted");
}


