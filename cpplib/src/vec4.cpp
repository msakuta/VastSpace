#include "../include/cpplib/vec4.h"

namespace cpplib{

const Vec4d &vec4_0001(){
	static Vec4d ret(0, 0, 0, 1);
	return ret;
}

const Vec4d &vec4_0010(){
	static Vec4d ret(0, 0, 1, 0);
	return ret;
}

const Vec4d &vec4_0100(){
	static Vec4d ret(0, 1, 0, 0);
	return ret;
}

const Vec4d &vec4_1000(){
	static Vec4d ret(1, 0, 0, 0);
	return ret;
}

}
