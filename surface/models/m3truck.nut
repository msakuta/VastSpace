local m = 1.e-3;

modelScale <- 3.4 / 200.0 * m;

mass <- 9000;

maxhealth <- 150.;

topSpeed <- 100. / 3.6 * m;
backSpeed <- 40. / 3.6 * m;

turretCooldown <- 0.5;
turretMuzzleSpeed <- 0.7;
turretDamage <- 20.;

turretYawSpeed <- 0.1 * PI;
barrelPitchSpeed <- 0.05 * PI;
barrelPitchMin <- -0.05 * PI;
barrelPitchMax <- 0.3 * PI;


hitbox <- [
	[Vec3d(0., 0.000, -0.0015), Quatd(0,0,0,1), Vec3d(0.0012, 0.0007, 0.0015)],
	[Vec3d(0., 0.000,  0.0015), Quatd(0,0,0,1), Vec3d(0.0012, 0.0007, 0.0015)],
];

