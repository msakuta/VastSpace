// Defender vs Destroyer
//initUI();

deltaFormation("Defender", 0, Quatd(0,1,0,0), Vec3d(0, 0., -25.), 50., 5, player.cs, function(e){e.command("Deploy");});
deltaFormation("Destroyer", 1, Quatd(0,0,0,1), Vec3d(0, 0., 2000.), 500, 1, player.cs, null);

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 250.;

