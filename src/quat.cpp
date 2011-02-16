#include "../include/cpplib/quat.h"

namespace cpplib{

quat_0_t::operator const Quatd&()const{
	static Quatd value(0, 0, 0, 0);
	return value;
}


quat_u_t::operator const Quatd&()const{
	static Quatd value(0, 0, 0, 1);
	return value;
}

const quat_0_t quat_0 = quat_0_t();
const quat_u_t quat_u = quat_u_t();
}
