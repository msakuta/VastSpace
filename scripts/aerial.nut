landingGSOffset <- -0.20;

local function rangein(v,min,max){
	return v < min ? min : max < v ? max : v;
}

local function min(a,b){
	return a < b ? a : b;
}

local function fmod(v,f){
	return v - f * floor(v / f);
}

function aerialLanding(e){
	local airport = e.landingAirport;
	if(airport == null)
		return null;

	local deltaPos = airport.ILSPos - e.getpos(); // Delta position towards the destination
	local forward = -e.getrot().trans(Vec3d(0,0,1));
	local ret = {};

	ret.roll <- function(){
		local landPerp = Quatd.rotation(airport.ILSHeading + PI / 2., Vec3d(0,1,0)).trans(Vec3d(0, 0, 1)); // Perpendicular to landing direction

		// Distance from correct path for landing
		local disco = (airport.ILSPos - e.getpos()).sp(landPerp);
		local desiredAngle = airport.ILSHeading + disco;
		local realAngle = atan2(forward[0], forward[2]);
		local corrAngle = fmod((desiredAngle - realAngle) + 3. * PI, 2. * PI) - PI;
		return rangein(-1. * corrAngle, -PI / 2., PI / 2.);
	}();

	ret.climb <- function(){
		if(deltaPos.len() == 0)
			return 0;
		local gs = deltaPos.norm()[1] + asin(3. / 180. * PI ) + (e.spoiler ? 0 : landingGSOffset); // Glide slope is 3 degrees
		local nvelo = e.getvelo().norm();
		local ret = (gs - forward[1]) * 3.;
		if(-0.08 < deltaPos[1])
			ret += (0.10 - deltaPos[1]) / 0.10 * -nvelo[1];
		else
			ret += 0.02 * -nvelo[1];
		ret += -e.getrot().cnj().trans(e.getomg())[0];
	//	print("climb " + deltaPos[1] + ", gs = " + gs + ", climb = " + ret);
		return ret;
	}();

	ret.throttle <- rangein(min(deltaPos.len() + 1., 5.) / 5. - 5. * e.getvelo().len(), 0, 0.5);
//	print("throttle " + ret.throttle);

	local f = deltaPos.len() / 3.0 - e.getvelo().len() / 0.5;
	ret.spoiler <- f < 1.;
//	print("spoiler " + ret.spoiler);

	ret.gear <- f < 1.;

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
