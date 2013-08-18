
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
	"HydraRocketLauncher", "HydraRocketLauncher", "SidewinderLauncher", "SidewinderLauncher",
];


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


	beginControl["A10"] <- function (){
		if("print" in this)
			print("A10::beginControl");
		cmd("pushbind");
		cmd("bind g gear");
		cmd("r_windows 0");
	}

	endControl["A10"] <- function (){
		if("print" in this)
			print("A10::endControl");
		cmd("popbind");
		cmd("r_windows 1");
	}
}
