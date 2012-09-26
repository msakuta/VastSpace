
//initUI();

showdt <- false;
framecount <- 0;
checktime <- 0;
autochase <- true;
deaths <- {};

rotation <- Quatd(0,0,0,1);

cmd("pause 0");

function frameproc(dt){
	framecount++;
	local global_time = universe.global_time;

	if(showdt)
		print("DT = " + dt + ", FPS = " + (1. / dt) + ", FC = " + framecount);

	if(autochase && player.chase == null){
//		rotation = (rotation * Quatd(sin(dt/2.), 0., 0., cos(dt/2.))).normin();
//		player.setpos(rotation.trans(Vec3d(0,0,2)));
//		player.setrot(rotation);
//		local phase = (global_time - PI/2.);
//		player.rot = Quatd(0., sin(phase/2.), 0., cos(phase/2.));
//		foreachents(player.cs, function(e):(player){ player.chase = e; });
	}

	local currenttime = universe.global_time + 9.;

	if(true && checktime + 10. < currenttime){
		local cs = universe.findcspath("/sol/saturn/saturno2");
		local farcs = universe.findcspath("/sol/saturn/saturno1");
		checktime = currenttime;

		if(cs == null || farcs == null)
			return;

		local racec = [countents(cs, 0, "Attacker"),
			countents(cs, 1, "Attacker") + countents(farcs, 1, "Attacker")];

		print("time " + currenttime + ": " + racec[0] + ", " + racec[1]);
/*
		if(racec[0] < 5)
			deltaFormation("Sceptor", 0, Quatd(0, 0, 0, 1)
				, Vec3d(0, 0.1,  0.5), 0.1, 15, null);
		if(racec[1] < 3)
			deltaFormation("Assault", 1, Quatd(0, 1, 0, 0)
				, Vec3d(0, 0.1, -0.5), 0.1, 3, null);
*/
		if(racec[0] < 2){
			local zrot = (2. * PI * rand()) / RAND_MAX;
			deltaFormation("Attacker", 0, Quatd(0, 0, 0, 1) * Quatd(0,0,sin(zrot),cos(zrot))
				, Vec3d(0, 0.1,  3.), 0.4, 3, cs, function(e){
				local d = e.docker;
				if(d != null && d.alive){
					for(local i = 0; i < 2; i++){
						local e2 = Entity.create("Defender");
						e2.race = e.race;
						d.addent(e2);
					}
					e.command("SetAggressive");
					e.command("Move", e.getpos() + e.getrot().trans(Vec3d(0,0,-2)));
				}
			});
		}
		if(racec[1] < 2){
			local zrot = (2. * PI * rand()) / RAND_MAX;
/*			deltaFormation("Attacker", 1, Quatd(0, 1, 0, 0) * Quatd(0,0,sin(zrot),cos(zrot))
				, Vec3d(0, 0.1, -3.), 0.4, 3, cs, null);*/
			deltaFormation("Attacker", 1, Quatd(0, 1, 0, 0) * Quatd(0,0,sin(zrot),cos(zrot))
				, Vec3d(0, 0.1, -3.), 0.4, 3, farcs != null ? farcs : cs, function(e):(cs){
					local d = e.docker;
					if(d != null && d.alive){
						for(local i = 0; i < 8; i++){
							local e2 = Entity.create("Sceptor");
							e2.race = e.race;
							d.addent(e2);
						}
					}
					e.command("RemainDocked", true);
					e.command("Warp", cs, Vec3d(0,0,2));
				});
		}

		foreachents(cs, function(e){
			if(e.race == 1 && e.classname == "Attacker")
				e.command("RemainDocked", false);
		});

		foreach(key,value in deaths) foreach(key1,value1 in value)
			print("[team" + key + "][" + key1 + "] " + value1);
	}
}

function hook_delete_Entity(e){
	if(!(e.race in deaths))
		deaths[e.race] <- {};
	if(!(e.classname in deaths[e.race]))
		deaths[e.race][e.classname] <- 0;
	deaths[e.race][e.classname]++;
/*	if(!(e.classname in deaths))
		deaths[e.classname] <- 0;
	deaths[e.classname]++;*/
}

