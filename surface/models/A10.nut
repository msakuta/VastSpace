
modelScale <- 0.01626 / 650.0;

hitRadius <- 0.010;

mass <- 13.e3;

maxhealth <- 800.;

maxfuel <- 120.; // seconds for full thrust

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(13.05, 5., 19.43) * 0.5e-3],
];

// Thrust power
thrust <- 0.01

// Aerodynamic tensors, referred to by wings.
local tensor1 = [
	-0.2, 0, 0,
	0, -10.0, 0,
	0, -0.5, -0.05
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
	{name = "MainRight", pos = Vec3d( 0.004, 0.001, 0.0), aero = tensor1,
		control = "aileron", sensitivity = -0.05 * PI},
	{name = "MainLeft", pos = Vec3d(-0.004, 0.001, 0.0), aero = tensor1,
		control = "aileron", sensitivity = 0.05 * PI},
	{name = "TailRight", pos = Vec3d( 0.003, 0.0, 0.008), aero = tensor2,
		control = "elevator", sensitivity = -0.1 * PI},
	{name = "TailLeft", pos = Vec3d(-0.003, 0.0, 0.008), aero = tensor2,
		control = "elevator", sensitivity = -0.1 * PI},
	{name = "VerticalLeft", pos = Vec3d( 0.0020, 0.002, 0.007), aero = tensor3,
		control = "rudder", sensitivity = -0.1 * PI, axis = Vec3d(0,1,0)},
	{name = "VerticalRight", pos = Vec3d(-0.0020, 0.002, 0.007), aero = tensor3,
		control = "rudder", sensitivity = -0.1 * PI, axis = Vec3d(0,1,0)},
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

bulletSpeed <- 1.0;

shootCooldown <- 60.0 / 2100;

cameraPositions <- [
	Vec3d(0., 50, -200) * modelScale,
	Vec3d(0., 0.0075, 0.025),
	Vec3d(0.020, 0.007, 0.050),
	Vec3d(0.004, -0.0015, 0.010),
]

hudPos <- cameraPositions[0] + Vec3d(0, 0, -20) * modelScale;

hudSize <- 10 * modelScale

// A trick to save "this" table after the evaluation ends
// (global variables in this file are not saved really globally)
local lthis = this;

local m = 1.e-3;

function fire(e,dt){
	if(e.weapon == 0){
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
					newvelo[j] += (rs.nextd() - 0.5) * 0.02;
				pb.setvelo(newvelo);
				pb.update(dt - e.cooldown);
				e.gunFireEffect();
			};
			e.cooldown += lthis.shootCooldown;
		}
		return;
	}

	if(e.cooldown < dt){
		local launcherType;
		local projectileType;
		local damage;
		local cooldown = @(launchers) 1. / launchers.len();
		switch(e.weapon){
		case 1:
			launcherType = "HydraRocketLauncher";
			projectileType = "HydraRocket";
			damage = 300;
			break;
		case 2:
			launcherType = "HellfireLauncher";
			projectileType = "Hellfire";
			damage = 500;
			break;
		case 3:
			launcherType = "SidewinderLauncher";
			projectileType = "Sidewinder";
			damage = 800;
			cooldown = @(...) 1.;
			break;
		default:
			print("A10.nut: invalid e.weapon in fire()");
			return;
		}
		local arms = e.arms;
		local launchers = [];

		// Ammunition count is saved in each launchers, so we must scan them
		// to find how many missiles we've got and which launcher is capable
		// of firing right now.
		local count = 0;
		foreach(it in arms)
			if(it.classname == launcherType && 0 < it.ammo)
				launchers.append(it), count += it.ammo;

		// If there's no missile left in the launchers, exit
		if(launchers.len() == 0)
			return;

		print("Hello fire launcherType = " + launcherType + ", launchers.len() = " + launchers.len() + " count = " + count);
		local i = count % launchers.len();
		local arm = launchers[i];
		local pb = e.cs.addent(projectileType, arm.getpos());
		pb.owner = e;
		pb.life = 10;
		pb.damage = damage;
		if(e.weapon != 1) // Rockets cannot guide to target
			pb.target = e.enemy;
		pb.setrot(e.getrot());
		pb.setvelo(e.getvelo() + e.getrot().trans(Vec3d(0,0,-0.01)));
		local deltaCooldown = cooldown(launchers);
		e.cooldown += deltaCooldown;
		e.lastMissile = pb;
		arm.ammo--;
	}
}

local rot0 = Quatd(0,0,0,1);

// Always place front hardpoint before rear, because earlier entries are fired first.
hardpoints <- [
	{pos = Vec3d(- 60, -20, 0) * modelScale, rot = rot0, name = "Wing L 1"},
	{pos = Vec3d(  60, -20, 0) * modelScale, rot = rot0, name = "Wing R 1"},
	{pos = Vec3d(-120, -20, 0) * modelScale, rot = rot0, name = "Wing L 2"},
	{pos = Vec3d( 120, -20, 0) * modelScale, rot = rot0, name = "Wing R 2"},
	{pos = Vec3d(-180, -20, 0) * modelScale, rot = rot0, name = "Wing L 3"},
	{pos = Vec3d( 180, -20, 0) * modelScale, rot = rot0, name = "Wing R 3"},
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
	register_console_command("gear", function(){
		if(player.controlled)
			player.controlled.gear = !player.controlled.gear;
	});

	register_console_command("spoiler", function(){
		if(player.controlled)
			player.controlled.spoiler = !player.controlled.spoiler;
	});

	beginControl["A10"] <- function (){
		if("print" in this)
			print("A10::beginControl");
		cmd("pushbind");
		cmd("bind g gear");
		cmd("bind b spoiler");
		cmd("r_windows 0");
	}

	endControl["A10"] <- function (){
		if("print" in this)
			print("A10::endControl");
		cmd("popbind");
		cmd("r_windows 1");
	}
}
