#ifndef YSDNMMOT_H
#define YSDNMMOT_H
#include "yssurf.h"
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <assert.h>

struct ysdnm_motion{
	struct keyframe : public ysdnm_var{
		double dt;
		keyframe() : dt(0.){}
		void copy(const keyframe&o){
			ysdnm_var::copy(o);
			dt = o.dt;
		}
		keyframe(const keyframe &o) : ysdnm_var(o), dt(o.dt){
		}
		keyframe &operator=(const keyframe &o){
			this->ysdnm_var::~ysdnm_var();
			copy(o);
			return *this;
		}
	} *kfl;
	int nkfl;
	ysdnm_motion() : kfl(NULL), nkfl(0){}
	ysdnm_motion(const ysdnm_motion &o){
		kfl = new keyframe[o.nkfl];
		for(int i = 0; i < o.nkfl; i++)
			kfl[i] = o.kfl[i];
	}
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
		kfl[nkfl-1] = keyframe();
		kfl[nkfl-1].dt = dt;
		return kfl[nkfl-1];
	}
	keyframe &insertKeyframe(double dt, int index){
		assert(0 <= index && index < nkfl);
		kfl = (keyframe*)realloc(kfl, (nkfl + 1) * sizeof *kfl);
		memmove(&kfl[index+1], &kfl[index], (nkfl - index) * sizeof *kfl);
		kfl[index] = keyframe();
		kfl[index].dt = dt;
		nkfl++;
		return kfl[index];
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
	double getTimeOfKeyframe(int keyframe){
		double ret = 0;
		for(int i = 0; i < keyframe && i < nkfl; i++)
			ret += kfl[i].dt;
		return ret;
	}
	ysdnm_var &interpolate(ysdnm_var &v, double time);
	void save(std::ostream&);
	void save2(std::ostream&);
	bool load(std::istream&);
};

#endif
