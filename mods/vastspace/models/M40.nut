maxammo <- 5;
shootCooldown <- 1.5;
bulletSpeed <- 1.0;
bulletDamage <- 5.0;
bulletVariance <- 0.001;
aimFov <- 0.2;
shootRecoil <- PI / 32.;
modelName <- "models/m40.mqo";

local lthis = this;

dofile("mods/vastspace/models/Firearm.nut");

function shoot(weapon, shooter, dt){
	return lthis.customShoot(weapon, shooter, dt, lthis);
}
