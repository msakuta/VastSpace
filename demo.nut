// ReZEL demo

ReZEL.set.bulletSpeed = 2;
ReZEL.set.walkSpeed = 0.05;
ReZEL.set.reloadTime = 2.;
ReZEL.set.rifleMagazineSize = 5;
ReZEL.set.vulcanReloadTime = 2.;
ReZEL.set.vulcanMagazineSize = 20;

function printeach(e){foreach(a,b in e) print(a + ": " + b);}

initUI();

player.cs = bookmarks["Side 7"].cs();

//deltaFormation("ReZEL", 0, Quatd(0,0,0,1), Vec3d(10, 0., 10. - 19.), 0.05, 1, player.cs, @(e) e.command("Move", Vec3d(10,0,0)));
//deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(10.01, 0., 10.01 - 20.), 0.05, 1, player.cs, null);
//deltaFormation("ReZEL", 0, Quatd(0,0,0,1), Vec3d(0, 3., 3.), 0.05, 2, player.cs, null);
deltaFormation("ReZEL", 1, Quatd(0,1,0,0), Vec3d(0, 0., 3.), 0.05, 1, player.cs, null);
//deltaFormation("ReZEL", 0, Quatd(0,0,0,1), Vec3d(0, -19., 0), 0.05, 1, player.cs, @(e) e.command("Move", Vec3d(0,10,0)));
//deltaFormation("ContainerHead", 0, Quatd(0,0,0,1), Vec3d(10, 0., 10. - 20.), 0.15, 1, player.cs, null);
//deltaFormation("Sceptor", 1, Quatd(0,1,0,0), Vec3d(0, 0., -3.), 0.05, 3, player.cs, null);

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setpos(Vec3d(10,0,10-16));
player.setrot(lookrot);
player.viewdist = 0.25;

