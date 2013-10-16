landingGSOffset <- -0.20;

local function rangein(v,min,max){
	return v < min ? min : max < v ? max : v;
}

local function fmod(v,f){
	return v - f * floor(v / f);
}

function aerialLandingRoll(e){
	local airport = e.landingAirport;
	if(airport == null)
		return;
	local landPerp = Quatd.rotation(airport.ILSHeading + PI / 2., Vec3d(0,1,0)).trans(Vec3d(0, 0, 1)); // Perpendicular to landing direction

	// Distance from correct path for landing
	local disco = (airport.ILSPos - e.getpos()).sp(landPerp);
	local front = -e.getrot().trans(Vec3d(0,0,1));
	local desiredAngle = airport.ILSHeading + disco;
	local realAngle = atan2(front[0], front[2]);
	local corrAngle = fmod((desiredAngle - realAngle) + 3. * PI, 2. * PI) - PI;
	return rangein(-1. * corrAngle, -PI / 2., PI / 2.);
}

function aerialLandingClimb(e){
//	print("aerialLandingClimb(" + e + ")");
//	return 0;
	local airport = e.landingAirport;

	local function localCoord(v){
		v = v / 128. - Vec3d(0.5,0,1);
		v[2] *= -1;
		return v;
	}

	// Landing site is 1.5 km (temporary) before ILS antennae.
	local landingSite = localCoord(Vec3d(24, 0, 0)) - Vec3d(0,0,1.5);

	local deltaPos = airport.getpos() + airport.getrot().trans(landingSite) - e.getpos(); // Delta position towards the destination
	if(deltaPos.len() == 0)
		return 0;
	local gs = deltaPos.norm()[1] + asin(3. / 180. * PI ) + (e.spoiler ? 0 : landingGSOffset); // Glide slope is 3 degrees
	local forward = -e.getrot().trans(Vec3d(0,0,1));
	local nvelo = e.getvelo().norm();
	local ret = (gs - forward[1]) * 3.;
	if(-0.08 < deltaPos[1])
		ret += (0.10 - deltaPos[1]) / 0.10 * -nvelo[1];
	else
		ret += 0.02 * -nvelo[1];
//	print("climb " + deltaPos[1] + ", gs = " + gs + ", climb = " + ret);
	return ret;
}

function aerialLandingThrottle(e){
	local airport = e.landingAirport;
	local deltaPos = airport.ILSPos - e.getpos(); // Delta position towards the destination
	local ret = rangein((deltaPos.len() + 1.) / 5. - 5. * e.getvelo().len(), 0, 0.5);
//	print("throttle " + ret);
	return ret;
}

function aerialLandingSpoiler(e){
	local airport = e.landingAirport;
	local deltaPos = airport.ILSPos - e.getpos(); // Delta position towards the destination
	local f = deltaPos.len() / 3.0 - e.getvelo().len() / 0.5;
	local ret = f < 1.;
//	print("spoiler " + f);
	return ret;
}

register_console_command("preset", function(){
	local z = 10.;
	local e = player.chase;
	if(e.alive){
		e.setpos(Vec3d(0.3, 1.0, z));
		e.setrot(Quatd(0,0,0,1));
		e.setvelo(Vec3d(0, 0, -0.2));
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



print("aerial.nut has been loaded");
