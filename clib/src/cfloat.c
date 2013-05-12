#include <clib/cfloat.h>
#include <math.h>

double rangein(double src, double min, double max){
	return src < min ? min : max < src ? max : src;
}

double approach(double src, double dst, double delta, double wrap){
	double ret;
	if(src < dst){
		if(dst - src < delta)
			return dst;
		else if(wrap && wrap / 2 < dst - src){
			ret = src - delta - floor((src - delta) / wrap) * wrap/*fmod(src - delta + wrap, wrap)*/;
			return src < ret && ret < dst ? dst : ret;
		}
		return src + delta;
	}
	else{
		if(src - dst < delta)
			return dst;
		else if(wrap && wrap / 2 < src - dst){
			ret = src + delta - floor((src + delta) / wrap) * wrap/*fmod(src + delta, wrap)*/;
			return ret < src && dst < ret ? dst : ret;
		}
		else return src - delta;
	}
}
