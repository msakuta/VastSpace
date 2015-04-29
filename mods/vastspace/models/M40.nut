maxammo <- 5;
shootCooldown <- 1.5;
bulletSpeed <- 1000.0;
bulletDamage <- 5.0;
bulletVariance <- 0.001;
aimFov <- 0.2;
shootRecoil <- PI / 32.;
modelName <- "models/m40.mqo";
overlayIconFile <- "models/sniperriflemodel.png";

local lthis = this;

dofile("mods/vastspace/models/Firearm.nut");

function shoot(weapon, shooter, dt){
	return lthis.customShoot(weapon, shooter, dt, lthis,
		@() format("mods/vastspace/sound/sniper-%d.ogg", weapon.cs.rs.next() % 2 + 1));
}
