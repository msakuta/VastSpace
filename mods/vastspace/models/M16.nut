maxammo <- 20;
shootCooldown <- 0.1;
bulletSpeed <- 0.7;
bulletDamage <- 1.0;
bulletVariance <- 0.01;
aimFov <- 0.7;
shootRecoil <- PI / 128.;
modelName <- "models/m16.mqo";

local lthis = this;

dofile("mods/vastspace/models/Firearm.nut");

function shoot(weapon, shooter, dt){
	return lthis.customShoot(weapon, shooter, dt, lthis);
}
