// Earth surface simulation test

local f15 = player.cs.addent("F15", Vec3d(0, 4., 0.));
player.chase = f15;

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 0.25;

register_console_command("reset", function(){
	f15.setpos(Vec3d(0., 4., 0.));
	f15.setrot(Quatd(0,0,0,1));
	f15.setvelo(Vec3d(0,0,0));
});

function reset(){
	f15.setpos(Vec3d(0., 4., 0.));
	f15.setrot(Quatd(0,0,0,1));
	f15.setvelo(Vec3d(0,0,0));
};

