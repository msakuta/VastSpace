
modelScale <- 0.00001;

mass <- 5000.;

maxhealth <- 100.;

rotorAxisSpeed <- 0.1 * PI;

mainRotorLiftFactor <- 1.1;
tailRotorLiftFactor <- 0.1;

featherSpeed <- 1.0;

tailRotorSpeed <- 3.0;

chainGunCooldown <- 60. / 625.; ///< Shoot repeat cycle time
chainGunMuzzleSpeed <- 1.; ///< Speed of shot projectile at the gun muzzle, which may decrease as it travels through air.
chainGunDamage <- 30.0; ///< Bullet damage for each chain gun round.
chainGunVariance <- 0.015; ///< Chain gun variance (inverse accuracy) in cosine angles.
chainGunLife <- 5.; ///< Time before shot bullet disappear

hydraDamage <- 300.0;

cockpitOfs <- Vec3d(0., 0.0008, -0.0022);

hitbox <- [
	[Vec3d(0, 0, 0), Quatd(0,0,0,1), Vec3d(140, 230, 700) * modelScale],
];

if(isClient()){

	beginControl["Apache"] <- function (){
		if("print" in this)
			print("Apache::beginControl");
		cmd("r_windows 0");
	}

	endControl["Apache"] <- function (){
		if("print" in this)
			print("Apache::endControl");
		cmd("r_windows 1");
	}
}

function hydraFire(e,dt){
	while(e.cooldown < dt){
		local arms = e.arms;
		local launchers = [];

		// Ammunition count is saved in each launchers, so we must scan them
		// to find how many rockets we've got and which launcher is capable
		// of firing right now.
		local hydras = 0;
		foreach(it in arms)
			if(it.classname == "HydraRocketLauncher" && 0 < it.ammo)
				launchers.append(it), hydras += it.ammo;

		// If there's no rockets left in the launchers, exit
		if(launchers.len() == 0)
			return;

		print("Hello hydraFire " + hydras);
		local i = hydras % launchers.len();
		local arm = launchers[i];
		local pb = e.cs.addent("HydraRocket", arm.getpos());
		pb.owner = e;
		pb.life = 10;
		pb.damage = 300;
		pb.setrot(e.getrot());
		pb.setvelo(e.getvelo() + e.getrot().trans(Vec3d(0,0,-0.1)));
		e.cooldown += 1. / launchers.len();
		arm.ammo--;
	}
}

function hellfireFire(e,dt){
	if(e.cooldown < dt){
		local arms = e.arms;
		local launchers = [];

		// Ammunition count is saved in each launchers, so we must scan them
		// to find how many missiles we've got and which launcher is capable
		// of firing right now.
		local hellfires = 0;
		foreach(it in arms)
			if(it.classname == "HellfireLauncher" && 0 < it.ammo)
				launchers.append(it), hellfires += it.ammo;

		// If there's no missile left in the launchers, exit
		if(launchers.len() == 0)
			return;

		print("Hello hellfireFire " + hellfires);
		local i = hellfires % launchers.len();
		local arm = launchers[i];
		local pb = e.cs.addent("Hellfire", arm.getpos());
		pb.owner = e;
		pb.life = 10;
		pb.damage = 300;
		pb.target = e.enemy;
		pb.setrot(e.getrot());
		pb.setvelo(e.getvelo() + e.getrot().trans(Vec3d(0,0,-0.01)));
		e.cooldown += 1. / launchers.len();
		arm.ammo--;
	}
}

local rot0 = Quatd(0,0,0,1);

hardpoints <- [
	{pos = Vec3d(-200, -80, 0) * modelScale, rot = rot0, name = "Inner Wing L"},
	{pos = Vec3d( 200, -80, 0) * modelScale, rot = rot0, name = "Inner Wing R"},
	{pos = Vec3d(-300, -80, 0) * modelScale, rot = rot0, name = "Outer Wing L"},
	{pos = Vec3d( 300, -80, 0) * modelScale, rot = rot0, name = "Outer Wing R"},
	{pos = Vec3d(-380, -40, 0) * modelScale, rot = rot0, name = "Wingtip L"},
	{pos = Vec3d( 380, -40, 0) * modelScale, rot = rot0, name = "Wingtip R"},
	{pos = Vec3d(   0, 250, 0) * modelScale, rot = rot0, name = "Rotor Hub"},
];

/// Default equipment set for hardpoints
defaultArms <- [
	"HydraRocketLauncher", "HydraRocketLauncher", "HellfireLauncher", "HellfireLauncher",
];

function drawOverlay(){
	// Fuselage model
	local points = [
		[75.5,669],
		[75.5,764],
		[246.727,836.5],
		[602,836.5],
		[716.045,761.227],
		[906.955,705.75],
		[936.5,560.5],
		[905,560.5],
		[859.227,646.455],
		[567.182,698.5],
		[452.409,607.818],
		[227.409,606.682]
	];
	glBegin(GL_LINE_LOOP);
	foreach(v in points){
		glVertex2d(0.002 * v[0] - 1., 0.002 * v[1] - 1.);
//		print("vertex " + v[0] + " " + v[1]);
	}
	glEnd();

	// Rotor shape
	local points2 = [
		[51.699,591.075],
		[22.727,534.024],
		[255.625,415.751],
		[22.699,297.45],
		[51.699,240.376],
		[326.324,379.848],
		[600.898,240.399],
		[629.875,297.45],
		[396.977,415.723],
		[629.875,534.024],
		[600.875,591.075],
		[326.301,451.626]
	];
	glBegin(GL_LINE_LOOP);
	foreach(v in points2){
		glVertex2d(0.002 * v[0] - 1., 0.002 * v[1] - 1.);
//		print("vertex " + v[0] + " " + v[1]);
	}
	glEnd();
}
