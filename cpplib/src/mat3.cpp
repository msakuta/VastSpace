#include "../include/cpplib/mat3.h"

namespace cpplib{

const Mat3d &mat3d_u(){
	static const Mat3d ret(Vec3d(1,0,0), Vec3d(0,1,0), Vec3d(0,0,1));
	return ret;
}

const Mat3f &mat3f_u(){
	static const Mat3f ret(Vec3f(1,0,0), Vec3f(0,1,0), Vec3f(0,0,1));
	return ret;
}

const Mat3i &mat3i_u(){
	static const Mat3i ret(Vec3i(1,0,0), Vec3i(0,1,0), Vec3i(0,0,1));
	return ret;
}

}
