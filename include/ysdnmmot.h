#ifndef YSDNMMOT_H
#define YSDNMMOT_H
extern "C"{
#include "yssurf.h"
}
#include <fstream>
#include <sstream>
#include <stdlib.h>

struct ysdnm_motion{
	struct keyframe : public ysdnm_var{
		double dt;
	} *kfl;
	int nkfl;
	ysdnm_motion() : kfl(NULL), nkfl(0){}
	~ysdnm_motion(){
		for(int i = 0; i < nkfl; i++)
			kfl[i].ysdnm_var::~ysdnm_var();
		::free(kfl);
		kfl = NULL;
		nkfl = 0;
	}
	double totalTime()const{
		double ret = 0.;
		for(int i = 0; i < nkfl; i++)
			ret += kfl[i].dt;
		return ret;
	}
	keyframe &addKeyframe(double dt){
		kfl = (keyframe*)realloc(kfl, ++nkfl * sizeof *kfl);
		kfl[nkfl-1].keyframe::keyframe();
		kfl[nkfl-1].dt = dt;
		return kfl[nkfl-1];
	}
	keyframe &getKeyframe(double time){
		return kfl[getKeyframeIndex(time)];
	}
	int getKeyframeIndex(double time){
		if(time < 0.)
			time = 0.;
		int i;
		for(i = 0; i < nkfl && kfl[i].dt <= time; time -= kfl[i].dt, i++);
		return nkfl <= i ? nkfl - 1 : i;
	}
	ysdnm_var &interpolate(ysdnm_var &v, double time);
	void save(std::ostream&);
	bool load(std::istream&);
};

#endif
