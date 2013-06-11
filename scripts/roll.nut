function getRoll(rot){
	local q = rot;
	return atan2(-2 * (q.w * q.z + q.x * q.y),
		1 - 2 * (q.z * q.z + q.x * q.x));
}
