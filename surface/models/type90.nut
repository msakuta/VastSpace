local m = 1.e-3;

modelScale <- 3.33 / 200 * m;

mass <- 50.e3;

maxhealth <- 800.;

topSpeed <- 70. / 3.6 * m;
backSpeed <- 35. / 3.6 * m;

mainGunCooldown <- 3.;
mainGunMuzzleSpeed <- 1.7;
mainGunDamage <- 500.;

turretYawSpeed <- 0.1 * PI;
barrelPitchSpeed <- 0.05 * PI;
barrelPitchMin <- -0.05 * PI;
barrelPitchMax <- 0.3 * PI;

sightCheckInterval <- 1.;


hitbox <- [
	[Vec3d(0., 0.000, -0.0015), Quatd(0,0,0,1), Vec3d(0.001665, 0.0007, 0.0015)],
	[Vec3d(0., 0.000,  0.0015), Quatd(0,0,0,1), Vec3d(0.001665, 0.0007, 0.0015)],
	[Vec3d(0., 100 * 3.4 / 200. * 1.e-3 - 0.0007, 0.0), Quatd(0,0,0,1), Vec3d(0.0015, 20 * 3.4 / 200. * 1.e-3, 0.0020)],
];

