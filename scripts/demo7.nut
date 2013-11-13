// Earth surface simulation test

local birds = [];

/*
local f15 = player.cs.addent("F15", Vec3d(0 + 0.2, 0.72, 5. - 0.35));
f15.setrot(Quatd(0,sqrt(2.)/2.,0,sqrt(2.)/2.));
f15.gear = true;
//player.chase = f15;
f15.destArrived = true;
birds.append(f15);
*/

//local f15_2 = player.cs.addent("F15", Vec3d(0, 4., -1));
//f15_2.race = 1;
//birds.append(f15_2);

//local f15_3 = player.cs.addent("F15", Vec3d(1, 4., 0));
//f15_3.race = 1;

local a10 = player.cs.addent("A10", Vec3d(0 + 0.2, 0.72, 5.0));
a10.setrot(Quatd(0,sqrt(2.)/2.,0,sqrt(2.)/2.));
player.chase = a10;
birds.append(a10);

//local a10_2 = player.cs.addent("A10", Vec3d(0 + 0.2, 0.72, 11.0));
//birds.append(a10_2);

local tank = player.cs.addent("Tank", Vec3d(0, 0, 0));
//player.chase = tank;

//f15.enemy = tank;
a10.enemy = tank;

//local tank2 = player.cs.addent("M3Truck", Vec3d(0.1, 0, 0.2));
//tank2.race = 1;

local bldg = player.cs.addent("SurfaceBuilding", Vec3d(0.3, 0, 0.3));
bldg.modelFile = "surface/models/bigsight.mqo";
bldg.modelScale = 0.001;
bldg.hitRadius = 0.15;

local airport = player.cs.addent("Airport", Vec3d(0, 0, 5.0));
airport.setrot(Quatd(0,1,0,0));

local function localCoord(v){
	v = v / 128. - Vec3d(0.5,0,1);
	v[2] *= -1;
	return v;
}

local function globalCoord(v){
	return airport.getrot().trans(localCoord(v)) + airport.getpos();
}



/*local apache = player.cs.addent("Apache", Vec3d(0, 2.71, 5. - 0.2));
player.chase = apache;
apache.enemy = tank;*/

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 0.25;

yawscale <- 1.;
pitchscale <- -1;

roll <- @() dofile("scripts/roll.nut")

register_console_command("roll", roll)

//roll()

function drand(){
	return rand().tofloat() / RAND_MAX;
}

function drand0(){
	return drand() - 0.5;
}

framecount <- 0;
checktime <- 0;
targetcs <- player.cs;

old_frameproc <- "frameproc" in this ? frameproc : null;

local path = [
//	globalCoord(Vec3d(24,0,24)),
//	globalCoord(Vec3d(42,0,24)),
	globalCoord(Vec3d(42,0,140)),
	globalCoord(Vec3d(69,0,154)),
	globalCoord(Vec3d(69,0,172)),
	globalCoord(Vec3d(52,0,196)),
	globalCoord(Vec3d(24,0,196)),
	globalCoord(Vec3d(24,0,28)),
];

local function flyProc(e){
	print("starting flyProc for " + e);

	while(true){
		e.landingAirport = airport;

		// Wait until land
		while(!e.onFeet)
			yield;

		local i = 0;
		e.destPos = path[i++];
		e.destArrived = false;
		while(i < path.len()){
			if(e.destArrived){
				print("Node next [" + i + "/" + path.len() + "]: " + path[i]);
				e.destPos = path[i];
				e.destArrived = false;
				if(path.len() - 1 <= i)
					e.takingOff = true;
				i++;
			}
			yield;
		}

		// Wait until took off
		while(!e.takingOff)
			yield;

		print("Waypoint next ");
		e.destPos = airport.getpos() + Vec3d(0., 0.0, 15.);
		e.destArrived = false;

		while(!e.destArrived)
			yield;

		print("Waypoint approach ");
		e.destPos = airport.getpos() + Vec3d(0., 0.0, 10.);
		e.destArrived = false;

		while(!e.destArrived)
			yield;

		print("Airport next ");
		e.landingAirport = airport;

		yield;
	}

	print("ending flyProc for " + e);

};

local ipaths = {};

function reset(){
	local z = 15.;
	foreach(e in birds) if(e.alive){
		ipaths[e] <- flyProc(e);
		e.landingAirport = airport;
		e.setpos(Vec3d(-0.1, 2.0, z));
		e.setrot(Quatd(0,0,0,1));
		e.setvelo(Vec3d(0, 0, -0.2));
		e.setomg(Vec3d(0, 0, 0));
		e.health = e.maxhealth;
		z -= 0.5;
	}
};

register_console_command("reset", reset);

reset();

function frameproc(dt){
	framecount++;
	local currenttime = universe.global_time + 9.;

	foreach(e in birds){
		if(e.alive){
			if(e in ipaths){
				if(ipaths[e].getstatus() == "suspended")
					resume ipaths[e];
			}
		}
	}

	if(checktime + 10. < currenttime){
		local cs = targetcs;
		checktime = currenttime;

		local racec = [countents(cs, 0, "Tank"), countents(cs, 1, "Tank")];

		if(false && racec[0] < 0){
			local a = player.cs.addent("Tank", Vec3d(drand0() * 10.5, 0.0, drand0() * 10.5));
			a.race = 0;
		}
		if(false && racec[1] < 0){
			local a = player.cs.addent("Tank", Vec3d(drand0() * 10.5, 0.0, drand0() * 10.5));
			a.race = 1;
		}

		local truck0 = countents(cs, 0, "M3Truck");
		local truck1 = countents(cs, 1, "M3Truck");
		if(false && truck0 < 0){
			local a = player.cs.addent("M3Truck", Vec3d(drand0() * 10.5, 0.0, drand0() * 10.5));
			a.race = 0;
		}
		if(false && truck1 < 0){
			local a = player.cs.addent("M3Truck", Vec3d(drand0() * 10.5, 0.0, drand0() * 10.5));
			a.race = 1;
		}
	}

	if(old_frameproc != null)
		old_frameproc(dt);
}

