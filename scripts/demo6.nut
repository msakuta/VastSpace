// Container Ship Demo
initUI();

//deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, 0., -0.025), 0.025, 20, player.cs, null);
deltaFormation("ContainerHead", 0, Quatd(0,0,0,1), Vec3d(0, 0., 0.), 0.15, 5, player.cs, null);
//deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0, 0.1, 0.), 0.15, 3, player.cs, null);
deltaFormation("Attacker", 0, Quatd(0,0,0,1), Vec3d(-1., 0.1, 0.), 0.5, 2, player.cs, null);

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 0.25;

