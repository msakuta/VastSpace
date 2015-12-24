
dofile("mods/surface/models/LandVehicle.nut");

local m = 1;

modelScale <- 3.33 / 200 * m;

landOffset <- 0.7;

mass <- 50.e3;

maxhealth <- 800.;

topSpeed <- 70. / 3.6 * m;
backSpeed <- 35. / 3.6 * m;

mainGunCooldown <- 3.;
mainGunMuzzleSpeed <- 1700.;
mainGunDamage <- 500.;

turretYawSpeed <- 0.1 * PI;
barrelPitchSpeed <- 0.05 * PI;
barrelPitchMin <- -0.05 * PI;
barrelPitchMax <- 0.3 * PI;

sightCheckInterval <- 1.;


hitbox <- [
	[Vec3d(0., 0.1250, -1.5), Quatd(0,0,0,1), Vec3d(1.665, 0.4, 1.5)],
	[Vec3d(0., 0.1250,  1.5), Quatd(0,0,0,1), Vec3d(1.665, 0.4, 1.5)],
	[Vec3d(0., 100 * 3.4 / 200. - 0.7, 0.0), Quatd(0,0,0,1), Vec3d(1.5, 20 * 3.4 / 200., 2.)],
];


if(isClient()){
	registerClass("Tank");
}



/// =========== Vehicle configuration ==========================================
// These suspension parameters should be proportional to the vehicle mass
// if we want similar behaviors regardless of mass, but they looks like
// capped by maxSuspensionForce anyway.
suspensionStiffness <- 20.;
suspensionDamping <- 2.3;
suspensionCompression <- 4.4;
wheelFriction <- 1000;
rollInfluence <- 0.1;
// The default parameter (6000) is optimized for a vehicle weighing 800 kg.
maxSuspensionForce <- 6000 * mass / 800.;
maxEngineForce <- 0.7 * mass;//this should be engine/velocity dependent
maxBrakingForce <- 0.1 * mass;

local halfTread = 1.5;
local wheelBase = 2.5; //Assume wheels are evenly placed before and after center of gravity
local connectionHeight = 0.25;
local wheelDirection = Vec3d(0,-1,0);
local wheelAxle = Vec3d(-1,0,0);
local wheelRadius = 0.5;
local suspensionRestLength = 0.6;

// Wheel index placement, note that -Z is the front
//        front
//        1 - 0
// left   |   | right
//        2 - 3
//        back

wheels <- [
	{
		connectionPoint = Vec3d(halfTread, connectionHeight, -wheelBase),
		wheelDirection = wheelDirection,
		wheelAxle = wheelAxle,
		suspensionRestLength = suspensionRestLength,
		wheelRadius = wheelRadius,
		isFrontWheel = true,
		isLeftWheel = false,
	},
	{
		connectionPoint = Vec3d(-halfTread, connectionHeight, -wheelBase),
		wheelDirection = wheelDirection,
		wheelAxle = wheelAxle,
		suspensionRestLength = suspensionRestLength,
		wheelRadius = wheelRadius,
		isFrontWheel = true,
		isLeftWheel = true,
	},
	{
		connectionPoint = Vec3d(-halfTread, connectionHeight, wheelBase),
		wheelDirection = wheelDirection,
		wheelAxle = wheelAxle,
		suspensionRestLength = suspensionRestLength,
		wheelRadius = wheelRadius,
		isFrontWheel = false,
		isLeftWheel = true,
	},
	{
		connectionPoint = Vec3d(halfTread, connectionHeight, wheelBase),
		wheelDirection = wheelDirection,
		wheelAxle = wheelAxle,
		suspensionRestLength = suspensionRestLength,
		wheelRadius = wheelRadius,
		isFrontWheel = false,
		isLeftWheel = false,
	},
]
