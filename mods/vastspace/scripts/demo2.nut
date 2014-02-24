
//initUI();

local earthlo = universe.findcspath("/sol/earth/lo");
if(!earthlo)
	earthlo = universe.findcspath("/earth/lo");

universe.paused = false;
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook

//local des = player.cs.addent("Destroyer", Vec3d(1.0, 0, 0.5));
//des.setrot(Quatd.rotation(PI/2., Vec3d(0,1,0)));

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

frameProcs.append(function(dt){
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

			// Force the players to chase a Soldier.
			if(pl.chase == null){
				foreachents(player.cs, function(e){
					if(e.classname == "Soldier" && e.race == i)
						pl.chase = e;
				});
			}

		}

		local racec = [countents(cs, 0, "Sceptor"), countents(cs, 1, "Sceptor")];

		print("time " + currenttime + ": " + racec[0] + ", " + racec[1]);

		while(true && countents(cs, 0, "Soldier") < 1){
			local soldier = player.cs.addent("Soldier", Vec3d((drand() - 0.5) * 0.3, gaussRand() * 0.005, -0.2 + (drand() - 0.5) * 0.10));
			soldier.setrot(Quatd(gaussRand(), gaussRand(), gaussRand(), gaussRand()).norm());
			soldier.race = 0;
			numsol0 = (numsol0 + 1) % 10;
		}
		while(true && countents(cs, 1, "Soldier") < 1){
			local soldier = player.cs.addent("Soldier", Vec3d((drand() - 0.5) * 0.3, gaussRand() * 0.005, 0.2 + (drand() - 0.5) * 0.10));
			soldier.setrot(Quatd(gaussRand(), gaussRand(), gaussRand(), gaussRand()).norm());
			soldier.race = 1;
			numsol = (numsol + 1) % 10;
		}


		foreach(key,value in deaths) foreach(key1,value1 in value)
			print("[team" + key + "][" + key1 + "] " + value1);
	}
});

function hook_delete_Entity(e){
	if(!(e.race in deaths))
		deaths[e.race] <- {};
	if(!(e.classname in deaths[e.race]))
		deaths[e.race][e.classname] <- 0;
	deaths[e.race][e.classname]++;
//	print(e + " pos:" + e.getpos() + " is deleted");
}


