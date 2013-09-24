
modelScale <- 0.001 / 30.0;

hitRadius <- 0.012;

mass <- 12.e3;

maxhealth <- 500.;

maxfuel <- 120.; // seconds for full thrust

hitbox <- [
	[Vec3d(0,1,0) * 0.5e-3, Quatd(0,0,0,1), Vec3d(13.05, 3., 19.43) * 0.5e-3],
	[Vec3d(0,-30,-50) * modelScale, Quatd(0,0,0,1), Vec3d(60., 25., 100.) * modelScale],
];

// Thrust power
thrust <- 0.015;

// Aerodynamic tensors, referred to by wings.
local tensor1 = [
	-0.1, 0, 0,
	0, -6.5, 0,
	0, -0.6, -0.025
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
	Vec3d(-215., 9., 125.) * modelScale,
	Vec3d( 215., 9., 125.) * modelScale,
]

gunPositions <- [
	Vec3d(20, 12, -200) * modelScale
]

local deg_per_rad = 180. / PI;

/// Indicates direction of shot rounds from the guns. It must be normalized, the speed is determied by bulletSpeed,
/// not by the length of this parameter.
gunDirection <- Vec3d(0, sin(2.0 / deg_per_rad), -cos(2.0 / deg_per_rad));

bulletSpeed <- 0.78;

shootCooldown <- 0.07;

cameraPositions <- [
	{pos = Vec3d(0., 30 * modelScale, -150 * modelScale), type = "cockpit"},
	{pos = Vec3d(0., 0.0075, 0.025)},
	{pos = Vec3d(0.020, 0.007, 0.050)},
	{pos = Vec3d(0.004, -0.0015, 0.010)},
	{pos = Vec3d(0., 30 * modelScale, -150 * modelScale), type = "missiletrack"},
	{pos = Vec3d(0., 0.010, 0.030), type = "viewtrack"},
]

hudPos <- Vec3d(0., 30.0, -170.0) * modelScale

hudSize <- 10 * modelScale

function sidewinderFire(e,dt){
	if(e.cooldown < dt){
		local arms = e.arms;
		local launchers = [];

		// Ammunition count is saved in each launchers, so we must scan them
		// to find how many missiles we've got and which launcher is capable
		// of firing right now.
		local sidewinders = 0;
		foreach(it in arms)
			if(it.classname == "SidewinderLauncher" && 0 < it.ammo)
				launchers.append(it), sidewinders += it.ammo;

		// If there's no missile left in the launchers, exit
		if(launchers.len() == 0)
			return;

		print("Hello sidewinderFire " + sidewinders);
		local i = sidewinders % launchers.len();
		local arm = launchers[i];
		local pb = e.cs.addent("Sidewinder", arm.getpos());
		pb.owner = e;
		pb.life = 10;
		pb.damage = 300;
		pb.target = e.enemy;
		pb.setrot(e.getrot());
		pb.setvelo(e.getvelo() + e.getrot().trans(Vec3d(0,0,-0.01)));
		e.cooldown += 1.;
		e.lastMissile = pb;
		arm.ammo--;
	}
}

local rot0 = Quatd(0,0,0,1);

// Always place front hardpoint before rear, because earlier entries are fired first.
hardpoints <- [
	{pos = Vec3d( 60, -20, -10) * modelScale, rot = rot0, name = "Fuselage FR"},
	{pos = Vec3d(-60, -20, -10) * modelScale, rot = rot0, name = "Fuselage FL"},
	{pos = Vec3d( 60, -20, 110) * modelScale, rot = rot0, name = "Fuselage RR"},
	{pos = Vec3d(-60, -20, 110) * modelScale, rot = rot0, name = "Fuselage RL"},
	{pos = Vec3d( 100, -20, 0) * modelScale, rot = rot0, name = "Inner Wing R"},
	{pos = Vec3d(-100, -20, 0) * modelScale, rot = rot0, name = "Inner Wing L"},
	{pos = Vec3d( 100, -20, 0) * modelScale, rot = rot0, name = "Inner Wing R"},
	{pos = Vec3d(-150, -20, 0) * modelScale, rot = rot0, name = "Outer Wing L"},
	{pos = Vec3d( 150, -20, 0) * modelScale, rot = rot0, name = "Outer Wing R"},
	{pos = Vec3d(-220,  10, 0) * modelScale, rot = rot0, name = "Wingtip L"},
	{pos = Vec3d( 220,  10, 0) * modelScale, rot = rot0, name = "Wingtip R"},
];

engineNozzles <- [
	Vec3d(20, 1, 240) * modelScale,
	Vec3d(-20, 1, 240) * modelScale,
];

/// Default equipment set for hardpoints
defaultArms <- [
	"SidewinderLauncher", "SidewinderLauncher", "SidewinderLauncher", "SidewinderLauncher",
];

/// Available weapon type names
weaponList <- [
	"M61A1 Vulcan",
//	"AIM-7 Sparrow",
	"AIM-9 Sidewinder",
//	"AIM-120 AMRAAM",
]


function drawOverlay(){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-1.0,  0.0);
	glVertex2d(-0.5,  0.2);
	glVertex2d( 0.0,  0.2);
	glVertex2d( 0.5,  0.7);
	glVertex2d( 0.7,  0.5);
	glVertex2d( 0.5,  0.2);
	glVertex2d( 0.7,  0.2);
	glVertex2d( 0.8,  0.4);
	glVertex2d( 1.0,  0.4);
	glVertex2d( 0.9,  0.2);
	glVertex2d( 0.9, -0.2);
	glVertex2d( 1.0, -0.4);
	glVertex2d( 0.8, -0.4);
	glVertex2d( 0.7, -0.2);
	glVertex2d( 0.5, -0.2);
	glVertex2d( 0.7, -0.5);
	glVertex2d( 0.5, -0.7);
	glVertex2d( 0.0, -0.2);
	glVertex2d(-0.5, -0.2);
	glEnd();
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

	beginControl["F15"] <- function (){
		if("print" in this)
			print("F15::beginControl");
		cmd("pushbind");
		cmd("bind g gear");
		cmd("bind b spoiler");
		cmd("r_windows 0");
		register_console_command("afterburner", function(){
			foreach(e in player.selected)
				e.afterburner = !e.afterburner;
		});
		cmd("bind e afterburner");
	}

	endControl["F15"] <- function (){
		if("print" in this)
			print("F15::endControl");
		cmd("popbind");
		cmd("r_windows 1");
	}
}
