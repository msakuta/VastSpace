function getRoll(rot){
	local heading = rot.trans(Vec3d(0, 0, -1));
//	print("roll.nut: heading: " + heading)
	local yaw = atan2(heading.x, heading.z);
	local pitch = asin(heading.y);
//	print("yaw: " + yaw);

	local fyaw = @(a) a.rotate(yaw * yawscale,0,1,0);
	local fpitch = @(a) a.rotate(pitch * pitchscale,1,0,0);
//	local lrot = fpitch(fyaw(rot));
	local lrot = fyaw(fpitch(rot));
//	local lrot = fyaw((rot));
	local xhat = lrot.trans(Vec3d(1,0,0));
//	return atan2(xhat.y, xhat.x);
	local q = rot;
//	return atan2(2 * (q.w * q.z + q.x * q.y),
//		q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
//	return atan2(2 * (q.w * q.y + q.z * q.x),
//		q.w * q.w + q.z * q.z - q.x * q.x - q.y * q.y);
	return atan2(-2 * (q.w * q.z + q.x * q.y),
		1 - 2 * (q.z * q.z + q.x * q.x));
}
