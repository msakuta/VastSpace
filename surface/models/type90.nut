local m = 1.e-3;

modelScale <- 3.33 / 200 * m;

mass <- 50.e3;

topSpeed <- 70. / 3.6 * m;
backSpeed <- 35. / 3.6 * m;

mainGunCooldown <- 3.;
mainGunMuzzleSpeed <- 1.7;
mainGunDamage <- 500.;

hitbox <- [
	[Vec3d(0., 0.000, -0.0015), Quatd(0,0,0,1), Vec3d(0.001665, 0.0007, 0.0015)],
	[Vec3d(0., 0.000,  0.0015), Quatd(0,0,0,1), Vec3d(0.001665, 0.0007, 0.0015)],
	[Vec3d(0., 100 * 3.4 / 200. * 1.e-3 - 0.0007, 0.0), Quatd(0,0,0,1), Vec3d(0.0015, 20 * 3.4 / 200. * 1.e-3, 0.0020)],
];

