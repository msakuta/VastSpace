// Just two capital ships nearby, calm

player.cs.addent("Attacker", Vec3d(1000,0,0));
player.cs.addent("Shipyard", Vec3d(-1000,0,0));

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

