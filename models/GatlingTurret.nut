
modelScale <- 0.05;

hitRadius <- 5.;

turretVariance <- 0.001 * PI;
turretIntolerance <- PI / 20.;
rotateSpeed <- 0.4 * PI;
manualRotateSpeed <- rotateSpeed * 0.5;

bulletDamage <- 10;
bulletLife <- 3.;
shootInterval <- 0.1;
bulletSpeed <- 4000.;
magazineSize <- 50;
reloadTime <- 5.;
barrelRotSpeed <- 2. * PI / shootInterval / 3.;
shootOffset <- Vec3d(0., 30, 0.) * modelScale;

modelFile <- "models/turretg1.mqo";
turretObjName <- "turretg1";
barrelObjName <- "barrelg1";
barrelRotObjName <- "barrelsg1";
muzzleObjName <- "muzzleg1";
