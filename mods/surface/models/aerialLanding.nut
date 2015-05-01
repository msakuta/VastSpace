::landingGSOffset <- -0.2;

local function rangein(v,min,max){
	return v < min ? min : max < v ? max : v;
}

local function min(a,b){
	return a < b ? a : b;
}

local function fmod(v,f){
	return v - f * floor(v / f);
}

local function estimate_pos(pos, velo, srcpos, srcvelo, speed, cs){
	local grv = Vec3d(0,0,0);
	if(cs != null){
		local mid = (pos + srcpos) * 0.5;
		local evelo = (pos - srcpos).norm() * -speed;
		grv = cs.accel(mid, evelo);
	}
	local dist = (pos - srcpos).len();
	local posy = pos[1] + (velo[1] + grv[1]) * dist / speed;
	return pos + (velo - srcvelo + grv * dist / speed / 2.) * dist / speed;
}

local estimateSpeed = 500.; ///< Virtual approaching speed for estimating position
local turnRange = 3000.;
local turnBank = 2. / 5. * PI;
local rudderAim = 7.; ///< Aim strength factor for rudder

local function aerialCruise(e, dt, deltaPos, forward){
	local ret = {roll = 0, climb = 0, throttle = 1., rudder = 0., brake = false, spoiler = false, gear = false};
	local dist = deltaPos.len();
	local estPos = estimate_pos(deltaPos,
		e.enemy != null && e.enemy.alive ? e.enemy.velo - e.velo : -e.velo,
		Vec3d(0,0,0), Vec3d(0,0,0), estimateSpeed, null);
	local sp = estPos.sp(-forward);
	local turnAngle = 0;

	local planar = [deltaPos[0], deltaPos[2]];
	ret.climb = deltaPos.sp(-forward) < 0 ? rangein(deltaPos[1] / sqrt(planar[0] * planar[0] + planar[1] * planar[1]), -0.5, 0.5) : 0;

	ret.throttle = (500. - e.getvelo().len()) * 2.;

	if(500. < dist){
		if(turnRange < dist && 0. < sp){ // Going away
			// Turn around to get closer to target.
			turnAngle += forward.sp(estPos) < 0 ? turnBank : -turnBank;
//			turning = 1;
		}
		else{
			estPos.normin();
			local turning = estPos.vp(forward)[1] * (1. - fabs(estPos[1]));
			if(turnRange < dist || sp < 0){ // Approaching
				turnAngle += turnBank * turning;
				ret.rudder = -turning * rudderAim;
//				print("turn " + turning);
			}
			else // Receding
				turnAngle += -turnBank * turning;
//			turning = fabs(turning);
		}

		// Upside down; get upright as soon as possible or we'll crash!
		if(e.getrot().trans(Vec3d(0,1,0))[1] < 0){
//			turning = 1;
			turnAngle = e.getrot().trans(Vec3d(1,0,0))[1] < 0 ? -turnBank : turnBank;
//			print("turnAngle: " + turnAngle);
		}

		ret.roll = turnAngle;
//		ret.roll = rangein(-1. * e.getrot().trans(Vec3d(1,0,0))[1], -PI / 2., PI / 2.);
	}
	else
		e.destArrived = true;
//	print("estPos " + estPos + ", rudder " + ret.rudder);
	return ret;
}

::aerialLanding <- function(e,dt){
	local airport = e.landingAirport;
	local landing = false;
	local deltaPos = (!e.onFeet && e.enemy ? e.enemy.getpos() : e.destPos) - e.getpos();
	local forward = -e.getrot().trans(Vec3d(0,0,1));

	if(airport != null && airport.alive && airport instanceof Airport){
		local tmpDeltaPos = airport.ILSPos - e.getpos(); // Delta position towards the destination
		if(0 < tmpDeltaPos.sp(forward)){
			deltaPos = tmpDeltaPos;
			landing = true;
			if(!e.showILS)
				e.showILS = true; // Turn on ILS display on the HUD.
		}
//		print("airport: " + landing + " tmpDeltaPos: " + tmpDeltaPos + " forward: " + forward);
	}

	if(!landing)
		return aerialCruise(e, dt, deltaPos, forward);

	local airportPos = Quatd.rotation(airport.ILSHeading, Vec3d(0,1,0)).trans(Vec3d(0,0,-1));
	deltaPos += airportPos;
	local ret = {rudder = 0};

	ret.roll <- function(){
		local landPerp = Quatd.rotation(airport.ILSHeading + PI / 2., Vec3d(0,1,0)).trans(Vec3d(0, 0, 1)); // Perpendicular to landing direction

		// Distance from correct path for landing
		local disco = (airport.ILSPos - e.getpos()).sp(landPerp);
		local desiredAngle = rangein(airport.ILSHeading + disco * 1e-3, -PI * 0.5, PI * 0.5);
		local realAngle = atan2(forward[0], forward[2]);
		local corrAngle = fmod((desiredAngle - realAngle) + 3. * PI, 2. * PI) - PI;
//		print(" desiredAngle " + desiredAngle + " realAngle " + realAngle + " corrAngle " +corrAngle );
//		return rangein(-1. * e.getrot().trans(Vec3d(1,0,0))[1], -PI / 2., PI / 2.);
		return rangein(-1. * corrAngle, -PI / 2., PI / 2.);
	}();

	ret.climb <- function(){
		if(deltaPos.len() == 0)
			return 0;
		local gs = deltaPos.norm()[1] + asin(3. / 180. * PI ) + (e.spoiler ? 0 : landingGSOffset); // Glide slope is 3 degrees
		local nvelo = e.getvelo().norm();
		local ret = (gs - forward[1]) * 10.;
		if(-80. < deltaPos[1])
			ret += (100. + deltaPos[1]) / 100. /** -nvelo[1]*/;
		else
			ret += 0.05e-3 * -nvelo[1];
		local localOmg = e.getrot().cnj().trans(e.getomg())[0];
		ret += -10. * localOmg;

		// Retrieve saved parameters from the global table if one exist.
		local params = e.object;

		// Otherwise assign the default value.
		if(params == null || typeof params != "table"){
			// Default parameter set
			params = {iclimb = 0};
			e.object = params;
			print("set default object for e: " + params);
		}

		ret += -1.0 * params.iclimb;

		// Integrate climb value (difference from ideal pitch)
		params.iclimb = rangein(params.iclimb + dt * ret, -1, 1);

		print("s[" + e + "]: ret = " + ret + ", iclimb " + params.iclimb + ", " + localOmg);

		return ret;
	}();

	local desiredSpeed = min(deltaPos.len() / 5. + 50. + 4 * deltaPos[1], 150.);
//	print("s[" + e + "]: " + e.getvelo().len() + ", "  + desiredSpeed);
	ret.throttle <- rangein((desiredSpeed - e.getvelo().len()) * 10., 0, 0.5);
//	print("throttle " + ret.throttle);

	local f = deltaPos.len() / 2000.0 - e.getvelo().len() / 250.;
	ret.brake <- f < 0.;
	ret.spoiler <- f < 1.;
//	print("spoiler " + ret.spoiler);

	ret.gear <- f < 1.;

	return ret;
}

register_console_command("preset", function(){
	local z = 7.;
	local e = player.chase;
	if(e.alive){
		e.setpos(Vec3d(300., 900., z));
		e.setrot(Quatd(0,0,0,1));
		e.setvelo(Vec3d(0, 0, -130.));
		e.setomg(Vec3d(0, 0, 0));
		e.health = e.maxhealth;
		foreach(a in player.cs.entlist){
			if(a.classname == "Airport"){
				e.landingAirport = a;
				break;
			}
		}
		z -= 0.5;
	}
});

// Reload this file
register_console_command("aerial", function(){
	print("aerialLanding.nut being loaded");
	dofile("mods/surface/models/aerialLanding.nut");
});

print("aerialLanding.nut has been loaded");
