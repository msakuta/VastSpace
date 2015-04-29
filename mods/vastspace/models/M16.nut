maxammo <- 20;
shootCooldown <- 0.1;
bulletSpeed <- 700.;
bulletDamage <- 1.0;
bulletVariance <- 0.01;
aimFov <- 0.7;
shootRecoil <- PI / 128.;
modelName <- "models/m16.mqo";
overlayIconFile <- "models/gunmodel.png";

local lthis = this;

dofile("mods/vastspace/models/Firearm.nut");

function shoot(weapon, shooter, dt){
	return lthis.customShoot(weapon, shooter, dt, lthis,
		@() format("mods/vastspace/sound/m16-%d.ogg", weapon.cs.rs.next() % 3 + 1));
}
