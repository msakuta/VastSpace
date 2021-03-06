
dofile("mods/surface/models/Aerial.nut");

modelScale <- 16.26 / 650.0;

hitRadius <- 10.;

mass <- 13.e3;

maxhealth <- 800.;

maxfuel <- 120.; // seconds for full thrust

hitbox <- [
	[Vec3d(0,20,0) * modelScale, Quatd(0,0,0,1), Vec3d(330, 50, 300) * modelScale],
	[Vec3d(0,-45,-75) * modelScale, Quatd(0,0,0,1), Vec3d(100, 15, 150) * modelScale],
];

navlights <- [
	{pos = Vec3d(0, -17, 4) * modelScale, radius = 1., period = 2, color = [1,1,1,1], pattern = "Step"}, // Pulse light
	{pos = Vec3d(-328, 12, -10) * modelScale, radius = 1., color = [1,0,0,1], pattern = "Constant"}, // Port
	{pos = Vec3d( 328, 12, -10) * modelScale, radius = 1., color = [0,1,0,1], pattern = "Constant"}, // Starboard
]

// Thrust power
thrust <- 10.;

// Aerodynamic tensors, referred to by wings.
local tensor1 = [
	-0.2, 0, 0,
	0, -10.0, 0,
	0, -1.0, -0.05
]

local tensor2 = [
	-0.1, 0, 0,
	0, -1.9, 0,
	0, -0.0, -0.015
]

local tensor3 = [
	-1.5, 0, 0,
	0, -0.05, 0,
	0, 0., -0.015
]

wings <- [
	{name = "MainRight", pos = Vec3d( 4., 1., 0.0), aero = tensor1,
		control = "aileron", sensitivity = -0.05 * PI},
	{name = "MainLeft", pos = Vec3d(-4., 1., 0.0), aero = tensor1,
		control = "aileron", sensitivity = 0.05 * PI},
	{name = "TailRight", pos = Vec3d( 3., 0.0, 8.), aero = tensor2,
		control = "elevator", sensitivity = -0.1 * PI},
	{name = "TailLeft", pos = Vec3d(-3., 0.0, 8.), aero = tensor2,
		control = "elevator", sensitivity = -0.1 * PI},
	{name = "VerticalLeft", pos = Vec3d( 2., 2., 7.), aero = tensor3,
		control = "rudder", sensitivity = -0.15 * PI, axis = Vec3d(0,1,0)},
	{name = "VerticalRight", pos = Vec3d(-2., 2., 7.), aero = tensor3,
		control = "rudder", sensitivity = -0.15 * PI, axis = Vec3d(0,1,0)},
]


wingTips <- [
	Vec3d(-328., 12., 10.) * modelScale,
	Vec3d( 328., 12., 10.) * modelScale,
]

gunPositions <- [
	Vec3d(5, -15, -320) * modelScale
]

/// Indicates direction of shot rounds from the guns. It must be normalized, the speed is determied by bulletSpeed,
/// not by the length of this parameter.
gunDirection <- Vec3d(0, 0, -1);

bulletSpeed <- 1000.0;

shootCooldown <- 60.0 / 2100;

cameraPositions <- [
	{pos = Vec3d(0., 50, -200) * modelScale, type = "cockpit"},
	{pos = Vec3d(0., 7.5, 25.)},
	{pos = Vec3d(20., 7., 50.)},
	{pos = Vec3d(4., -0.8, 10.)},
	{pos = Vec3d(0., 50, -200) * modelScale, type = "missiletrack"},
	{pos = Vec3d(0., 10., 30.), type = "viewtrack"},
]

hudPos <- cameraPositions[0].pos + Vec3d(0, 0, -20) * modelScale;

hudSize <- 10 * modelScale

// A trick to save "this" table after the evaluation ends
// (global variables in this file are not saved really globally)
local lthis = this;

local m = 1.e-3;

local ArmBase = {
	launcherType = "",
	projectileType = "Bullet",
	damage = 30.,
	cooldown = @(launchers) 1. / launchers.len(),
	ammoConsume = 1,
}

local HydraRocketLauncher = {
	launcherType = "HydraRocketLauncher",
	projectileType = "HydraRocket",
	damage = 300,
}
HydraRocketLauncher.setdelegate(ArmBase);

local HellfireLauncher = {
	launcherType = "HellfireLauncher",
	projectileType = "Hellfire",
	damage = 500,
	ammoConsume = 0,
	cooldown = @(...) 2.,
}
HellfireLauncher.setdelegate(ArmBase);

local SidewinderLauncher = {
	launcherType = "SidewinderLauncher",
	projectileType = "Sidewinder",
	damage = 800,
	cooldown = @(...) 1.,
}
SidewinderLauncher.setdelegate(ArmBase);

local weapons = [
	ArmBase,
	HydraRocketLauncher,
	HellfireLauncher,
	SidewinderLauncher,
];

function fire(e,dt){
	if(e.weapon == 0){
		local shot = false;
		while(e.cooldown < dt){
			local i = 0;
			local rot = e.getrot();
			foreach(it in lthis.gunPositions){
				local pb = e.cs.addent("ExplosiveBullet", e.getpos() + rot.trans(it));
				if(pb == null){
					print("pb is null!");
					return;
				}
				pb.owner = e;
				pb.damage = 50;
				pb.life = 3.;
				pb.splashRadius = 20 * m;
				pb.friendlySafe = false;
//				pb.mass = 0.010;
				local v0 = lthis.gunDirection * lthis.bulletSpeed;
				local newvelo = rot.trans(v0) + e.getvelo();
				local rs = e.cs.rs; // Obtain random sequence
				for(local j = 0; j < 3; j++)
					newvelo[j] += (rs.nextd() - 0.5) * 20.;
				pb.setvelo(newvelo);
				pb.update(dt - e.cooldown);
				e.gunFireEffect();
			};
			e.cooldown += lthis.shootCooldown;
			shot = true;
		}

		if(e.object == null)
			e.object = {};
		if(!("shootSid" in e.object))
			e.object.shootSid <- 0;

		if(shot){
			if(isEndSound3D(e.object.shootSid)){
				e.object.shootSid = playSound3D("surface/sound/A10-gunshot.ogg", e.pos);
				print("A10 shoot: " + e.object.shootSid);
			}
		}

		return;
	}

	if(weapons.len() <= e.weapon){
		print("A10.nut: invalid e.weapon in fire()");
		return;
	}
	local w = weapons[e.weapon];
	return fireLauncher(e, dt, w.launcherType, function(e,arm,fired){
		// Temporarily regenrates ammunition for testing missile homing algorithm.
		if(e.weapon == 2)
			arm.ammo++;
	});
}

function queryAmmo(e){
	if(weapons.len() <= e.weapon){
		print("A10.nut: invalid e.weapon in queryAmmo()");
		return 0;
	}
	local w = weapons[e.weapon];
	local count = 0;
	foreach(it in e.arms)
		if(it.classname == w.launcherType)
			count += it.ammo;
	return count;
}

local rot0 = Quatd(0,0,0,1);

// Always place front hardpoint before rear, because earlier entries are fired first.
hardpoints <- [
	{pos = Vec3d(- 60, -20, 0) * modelScale, rot = rot0, name = "Wing L 1"},
	{pos = Vec3d(  60, -20, 0) * modelScale, rot = rot0, name = "Wing R 1"},
	{pos = Vec3d(-160, -20, 0) * modelScale, rot = rot0, name = "Wing L 2"},
	{pos = Vec3d( 160, -20, 0) * modelScale, rot = rot0, name = "Wing R 2"},
	{pos = Vec3d(-200, -20, 0) * modelScale, rot = rot0, name = "Wing L 3"},
	{pos = Vec3d( 200, -20, 0) * modelScale, rot = rot0, name = "Wing R 3"},
	{pos = Vec3d(-240, -20, 0) * modelScale, rot = rot0, name = "Wing L 4"},
	{pos = Vec3d( 240, -20, 0) * modelScale, rot = rot0, name = "Wing R 4"},
	{pos = Vec3d(-330,  12, 0) * modelScale, rot = rot0, name = "Wingtip L"},
	{pos = Vec3d( 330,  12, 0) * modelScale, rot = rot0, name = "Wingtip R"},
];

/// Default equipment set for hardpoints
defaultArms <- [
	"HydraRocketLauncher", "HydraRocketLauncher",
	"HellfireLauncher", "HellfireLauncher",
	"SidewinderLauncher", "SidewinderLauncher",
];

/// Available weapon type names
weaponList <- [
	"GAU-8 Avenger",
	"Hydra-70 Rocket",
	"Hellfire Missile",
	"AIM-9 Sidewinder",
]

flyingSoundFile <- "sound/airrip.ogg";
flyingHiSoundFile <- "sound/airrip-h.ogg";

function drawOverlay(){
	local glPath = function(d,scale){
		local x0 = d[0] - scale, y0 = d[1] - scale;
		print("Adv: " + x0 + ", " + y0);
		local x = x0, y = y0;
		glBegin(GL_LINE_LOOP);
		for(local i = 1; i < d.len() / 2; i++)
			glVertex2d((x += d[i * 2]) / scale, (y += d[i * 2 + 1]) / scale);
		glVertex2d(x0 / scale, y0 / scale);
		glEnd();
	}

	local glRect = function(x0,y0,width,height){
		local x1 = x0 + width, y1 = y0 + height;
		glBegin(GL_LINE_LOOP);
		glVertex2d(x0, y0);
		glVertex2d(x0, y1);
		glVertex2d(x1, y1);
		glVertex2d(x1, y0);
		glEnd();
	}

	local data = [
		450,50,
		-50,370,
		-150,0,
		-150,50,
		0,60,
		150,50,
		150,0,
		50,370,
		150,0,
		0,-380,
		180,0,
		0,130,
		20,50,
		100,0,
		-20,-50,
		0,-400,
		20,-50,
		-100,0,
		-20,50,
		0,120,
		-180,0,
		0,-370,
	];

	glPath(data, 500.);
	glRect(100. / 500., -200. / 500., 140. / 500., 130. / 500.);
	glRect(100. / 500.,   60. / 500., 140. / 500., 130. / 500.);
}

if(isClient()){
	registerControls("A10");
}
