// Earth surface simulation test

local f15 = player.cs.addent("F15", Vec3d(0, 4., 0.));
player.chase = f15;

local f15_2 = player.cs.addent("F15", Vec3d(0, 4., -1));
f15_2.race = 1;

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 0.25;

function reset(){
	local z = 0.;
	foreach(e in [f15, f15_2]){
		e.setpos(Vec3d(0., 4., z));
		e.setrot(Quatd(0,0,0,1));
		e.setvelo(Vec3d(0, 0, -0.2));
		e.setomg(Vec3d(0, 0, 0));
		z -= 0.5;
	}
};

register_console_command("reset", reset);

reset();

yawscale <- 1.;
pitchscale <- -1;

roll <- @() dofile("scripts/roll.nut")

register_console_command("roll", roll)

roll()

