
local function rangein(v,min,max){
	return v < min ? min : max < v ? max : v;
}

local function min(a,b){
	return a < b ? a : b;
}

local function max(a,b){
	return a < b ? b : a;
}

local function fmod(v,f){
	return v - f * floor(v / f);
}

local SSM_ACCEL = 0.20;

function HellfireProc(e,dt){
	local forward = -e.rot.trans(Vec3d(0,0,1));
	print("" + e + ", forward: " + forward);

	local obj = e.object;
	if(obj == null || typeof obj != "table"){
		obj = {def = [0,0], integral = 0., centered = false};
		e.object = obj;
		print("set default object for e: " + obj);
	}
	local def = obj.def;

	local dv = Vec3d(0,0,0);
	local dv2 = Vec3d(0,0,0);

	local function run(){
		local samspeed = 0.8;
		local delta = e.target.pos - e.pos;
		if(delta.slen() == 0)
			return;

		local zh = delta.norm();
		local accel = e.cs.accel(e.pos, e.velo);

		local qc = e.rot.cnj();
		dv = zh
			+ e.rot.trans(Vec3d(1,0,0)) * def[0]
			+ e.rot.trans(Vec3d(0,1,0)) * def[1];
		if(dv.len() != 0)
			dv.normin();
		local dif = forward.sp(dv);
		local localdef = qc.trans(dv);
		local deflection = zh - forward * dif;
		if(0.96 < dif){
			local k = (7.e2 * dt, 7.e2 * dt, 0.5e2 * dt);
			obj.integral += 1.e5 * deflection.slen() * dt;
			def[0] += k * localdef[0];
			def[1] += k * localdef[1];
		}
		else if(0. < dif){
			def[0] = def[1] = 0.;
		}
		else{
			e.target = null;
			return null;
		}

		local deltavelo = e.target.velo - e.velo;
		deltavelo -= zh * deltavelo.sp(zh);
		local speed = max(0.5, e.velo.sp(zh));
		local dist = (e.target.pos - e.pos).len();
		local t = min(1., dist / speed);
		if(dist == 0)
			return;
		dv = (e.target.pos - e.pos).norm()
			+ e.rot.trans(Vec3d(1,0,0)) * def[0]
			+ e.rot.trans(Vec3d(0,1,0)) * def[1];
		if(dv.len() == 0)
			0;
		dv.normin();
		dv2 = dv;
	}

	if(e.target != null && e.target.alive)
		run();
	else{
		dv = forward;
		dv2 = forward;
	}

	{
		local maxspeed = PI * (e.velo.len() + 1.) * dt;
		local xh = forward.vp(dv);
		local len = xh.len();
		local angle = asin(len);
		if(maxspeed < angle){
			angle = maxspeed;
			if(!obj.centered)
				obj.centered = true;
		}
		if(len && angle){
			e.rot *= Quatd.rotation(angle, xh);
		}
	}

	local sp = forward.sp(dv);
	if(0 < sp){
		local burnt = dt * 3. * SSM_ACCEL;
		local thrust = burnt / (1. + e.fuel / 20.);
		sp = exp(-thrust);
		e.velo *= sp;
		e.velo += forward * (1. - sp);
		e.fuel -= burnt;
		print("fuel used: " + burnt + ", left: " + e.fuel);
	}
}



print("Hellfire-proc.nut has been loaded");
