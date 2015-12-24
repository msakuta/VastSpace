local m = 1;

modelScale <- 2.22 / 200.0 * m;

mass <- 9000;

maxhealth <- 150.;

topSpeed <- 100. / 3.6 * m;
backSpeed <- 40. / 3.6 * m;

turretCooldown <- 0.2;
turretMuzzleSpeed <- 700.;
turretDamage <- 10.;

turretYawSpeed <- 0.3 * PI;
barrelPitchSpeed <- 0.2 * PI;
barrelPitchMin <- -0.05 * PI;
barrelPitchMax <- 0.3 * PI;

sightCheckInterval <- 1.;

landOffset <- 0.7;


hitbox <- [
	[Vec3d(0., landOffset - 0.05, -1.5), Quatd(0,0,0,1), Vec3d(1.11, 0.9, 1.5)],
	[Vec3d(0., landOffset - 0.05,  1.5), Quatd(0,0,0,1), Vec3d(1.11, 0.9, 1.5)],
];

cameraPositions <- [
	Vec3d(-50, 170, -70) * modelScale - Vec3d(0, landOffset, 0),
	Vec3d(0., 7.5, 25),
]


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
maxEngineForce <- 1.5 * mass;//this should be engine/velocity dependent
maxBrakingForce <- 0.1 * mass;
steeringSpeed <- PI / 12.;
maxSteeringAngle <- PI / 12.;

local halfTread = 1.;
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
