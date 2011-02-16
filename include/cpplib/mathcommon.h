#ifndef CPPLIB_MATHCOMMON_H
#define CPPLIB_MATHCOMMON_H
#include <exception>

namespace mathcommon{

	class RangeCheck : public std::exception{
		const char *what()const throw(){
			return "Range check error";
		};
	};

};

#endif
