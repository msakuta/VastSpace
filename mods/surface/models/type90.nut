local m = 1;

modelScale <- 3.33 / 200 * m;

landOffset <- 0.7;

mass <- 50.e3;

maxhealth <- 800.;

topSpeed <- 70. / 3.6 * m;
backSpeed <- 35. / 3.6 * m;

mainGunCooldown <- 3.;
mainGunMuzzleSpeed <- 1700.;
mainGunDamage <- 500.;

turretYawSpeed <- 0.1 * PI;
barrelPitchSpeed <- 0.05 * PI;
barrelPitchMin <- -0.05 * PI;
barrelPitchMax <- 0.3 * PI;

sightCheckInterval <- 1.;


hitbox <- [
	[Vec3d(0., 0.1250, -1.5), Quatd(0,0,0,1), Vec3d(1.665, 0.4, 1.5)],
	[Vec3d(0., 0.1250,  1.5), Quatd(0,0,0,1), Vec3d(1.665, 0.4, 1.5)],
	[Vec3d(0., 100 * 3.4 / 200. - 0.7, 0.0), Quatd(0,0,0,1), Vec3d(1.5, 20 * 3.4 / 200., 2.)],
];

