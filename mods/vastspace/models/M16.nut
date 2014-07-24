maxammo <- 20;
shootCooldown <- 0.1;
bulletSpeed <- 0.7;
bulletDamage <- 1.0;
bulletVariance <- 0.01;
aimFov <- 0.7;
shootRecoil <- PI / 128.;

local lthis = this;
local relpos = Vec3d(0.0002, -0.000, -0.0003);
local nh0 = Vec3d(0., 0., -1);

function shoot(weapon, shooter, dt){
	local shot = false;
	local w = weapon.cs;

	if(0 < weapon.ammo){
		local rot = shooter.getrot();
		local pb = shooter.cs.addent("Bullet", shooter.getpos() + rot.trans(relpos));
		if(pb == null){
			print("pb is null!");
			return;
		}
		pb.owner = shooter;
		pb.damage = lthis.bulletDamage;
		pb.life = 5.;
		pb.mass = 0.03 * lthis.bulletDamage; // 10 grams

		local nh = rot.trans(nh0);

		// Generate variance vector from random number generator.
		// It's not really isotropic distribution, I'm afraid.
		local vecvar = Vec3d(w.rs.nextGauss(), w.rs.nextGauss(), w.rs.nextGauss()) * lthis.bulletVariance * lthis.bulletSpeed;

		// Remove component parallel to heading direction.
		vecvar -= nh * nh.sp(vecvar);

		// Accumulate desired direction and variance vector to determine the bullet's direction.
		pb.velo = weapon.velo + (nh + vecvar) * lthis.bulletSpeed;
		// Advancing the bullet, but shooter.cooldown is already subtracted by shootCooldown, so compensate it
		pb.update(dt - shooter.cooldown + lthis.shootCooldown);
		weapon.ammo--;

		// Play the sound
		local file = format("mods/vastspace/sound/m16-%d.ogg", w.rs.next() % 3 + 1);
//		print(file);
		playSound3D(file, weapon.pos, 1, 0.05 * 0.05);
	}
}
