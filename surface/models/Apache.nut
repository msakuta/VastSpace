
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
