// Attacker with Defenders
//initUI();

//deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, 0., -0.025), 0.025, 20, player.cs, null);
deltaFormation("Attacker", 1, Quatd(0,0,0,1), Vec3d(0, 0., 0.7), 0.15, 1, player.cs,
	function(e){
		local d = e.docker;
		if(d != null && d.alive){
			for(local i = 0; i < 5; i++){
				local e2 = d.addent("Defender");
				e2.race = e.race;
			}
		}
	}
);

cmd("pause 0");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
player.viewdist = 0.25;

