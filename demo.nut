// Container Ship Demo
initUI();

deltaFormation("ReZEL", 0, Quatd(0,0,0,1), Vec3d(0, 0., 0.), 0.05, 3, player.cs, null);
deltaFormation("ReZEL", 1, Quatd(0,1,0,0), Vec3d(0, 0., -3.), 0.05, 3, player.cs, null);

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 0.25;

