local m = 1;

modelScale <- 2.22 / 200.0 * m;

mass <- 9000;

maxhealth <- 150.;

topSpeed <- 100. / 3.6 * m;
backSpeed <- 40. / 3.6 * m;

turretCooldown <- 0.2;
turretMuzzleSpeed <- 700.;
turretDamage <- 10.;

turretYawSpeed <- 0.3 * PI;
barrelPitchSpeed <- 0.2 * PI;
barrelPitchMin <- -0.05 * PI;
barrelPitchMax <- 0.3 * PI;

sightCheckInterval <- 1.;

landOffset <- 0.7;


hitbox <- [
	[Vec3d(0., landOffset, -0.0025), Quatd(0,0,0,1), Vec3d(0.002, 0.002, 0.0025)],
	[Vec3d(0., landOffset,  0.0025), Quatd(0,0,0,1), Vec3d(0.002, 0.002, 0.0025)],
];

cameraPositions <- [
	Vec3d(-50, 170, -70) * modelScale - Vec3d(0, landOffset, 0),
	Vec3d(0., 7.5, 25),
]
