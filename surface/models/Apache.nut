
modelScale <- 0.00001;

mass <- 5000.;

maxhealth <- 100.;

rotorAxisSpeed <- 0.1 * PI;

cockpitOfs <- Vec3d(0., 0.0008, -0.0022);

hitbox <- [
	[Vec3d(0, 30, -220) * modelScale, Quatd(0,0,0,1), Vec3d(140, 230, 700) * modelScale],
];
