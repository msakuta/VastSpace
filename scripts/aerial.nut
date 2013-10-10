landingGSOffset <- 0.12;

local function rangein(v,min,max){
	return v < min ? min : max < v ? max : v;
}

local function fmod(v,f){
	return v - f * floor(v / f);
}

function aerialLandingRoll(e){
	local landPerp = Quatd.rotation(PI * 3. / 2., Vec3d(0,1,0)).trans(Vec3d(0, 0, 1)); // Perpendicular to landing direction

	// Distance from correct path for landing
	local disco = (e.destPos - e.getpos()).sp(landPerp);
	local front = -e.getrot().trans(Vec3d(0,0,1));
	local desiredAngle = PI + disco;
	local realAngle = atan2(front[0], front[2]);
	local corrAngle = fmod((desiredAngle - realAngle) + 3. * PI, 2. * PI) - PI;
	rangein(-1. * corrAngle, -PI / 2., PI / 2.);
}

function aerialLandingClimb(e){
//	print("aerialLandingClimb(" + e + ")");
//	return 0;
	local deltaPos = e.destPos - e.getpos(); // Delta position towards the destination
	local gs = deltaPos.norm()[1] + landingGSOffset; // Glide slope is 3 degrees
	local nvelo = e.getvelo().norm();
	local ret = gs - nvelo[1] * 0.2;
	print("delta = " + deltaPos + ", gs = " + gs + ", ret = " + ret);
	return ret;
}

function aerialLandingThrottle(e){
	local deltaPos = e.destPos - e.getpos(); // Delta position towards the destination
	local ret = rangein((deltaPos.len() - 3.) / 15. - e.getvelo().len(), 0, 0.5);
	return ret;
}

print("aerial.nut has been loaded");
