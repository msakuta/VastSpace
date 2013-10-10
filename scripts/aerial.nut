landingGSOffset <- -0.12;

function aerialLandingClimb(e){
//	print("aerialLandingClimb(" + e + ")");
//	return 0;
	local deltaPos = e.destPos - e.getpos(); // Delta position towards the destination
	local gs = deltaPos[1] + landingGSOffset; // Glide slope is 3 degrees
	local nvelo = e.getvelo().norm();
	local ret = gs - nvelo[1] * 0.;
	print("delta = " + deltaPos + ", gs = " + gs + ", ret = " + ret);
	return ret;
}

print("aerial.nut has been loaded");
