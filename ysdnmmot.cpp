extern "C"{
#include "yssurf.h"
#include <clib/aquat.h>
}
#include "ysdnmmot.h"
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <iomanip>

ysdnm_var *ysdnm_var::dup()const{
	ysdnm_var *ret = new ysdnm_var;
	if(bonenames){
		ret->bonenames = new const char *[bones];
		for(int i = 0; i < bones; i++){
			ret->bonenames[i] = new char[strlen(bonenames[i]) + 1];
			strcpy(const_cast<char*>(ret->bonenames[i]), bonenames[i]);
		}
	}
	if(bonerot){
		ret->bonerot = new double[bones][7];
		memcpy(ret->bonerot, bonerot, bones * sizeof *ret->bonerot);
	}
	ret->bones = bones;
	return ret;
}

double *ysdnm_var::addBone(const char *name){
	char **names = new char*[bones+1];
	memcpy(names, bonenames, bones * sizeof *names);
	if(bonenames)
		delete[] bonenames;
	names[bones] = new char[strlen(name)+1];
	strcpy(names[bones], name);
	bonenames = const_cast<const char**>(names);
	double (*rots)[7] = new double[bones+1][7];
	memcpy(rots, bonerot, bones * sizeof *rots);
	QUATIDENTITY(rots[bones]);
	rots[bones][4] = 0;
	rots[bones][5] = 0;
	rots[bones][6] = 0;
	if(bonerot)
		delete[] bonerot;
	bonerot = rots;

	float *newvisible = new float[bones+1];
	if(visible)
		memcpy(newvisible, visible, bones * sizeof *visible);
	else for(int i = 0; i < bones+1; i++)
		newvisible[i] = 1.;
	newvisible[bones] = 1.;
	if(visible)
		delete[] visible;
	visible = newvisible;

	return bonerot[bones++];
}




static void slerp(double p[], const double q[], const double r[], const double t)
{
  double qr = q[0] * r[0] + q[1] * r[1] + q[2] * r[2] + q[3] * r[3];
  double ss = 1.0 - qr * qr;
  
  if (ss < DBL_EPSILON) {
    p[0] = q[0];
    p[1] = q[1];
    p[2] = q[2];
    p[3] = q[3];
  }
  else if(q[0] == r[0] && q[1] == r[1] && q[2] == r[2] && q[3] == r[3]){
	  QUATCPY(p, q);
  }
  else {
    double sp = sqrt(ss);
    double ph = acos(qr);
    double pt = ph * t;
    double t1 = sin(pt) / sp;
    double t0 = sin(ph - pt) / sp;
    
    p[0] = q[0] * t0 + r[0] * t1;
    p[1] = q[1] * t0 + r[1] * t1;
    p[2] = q[2] * t0 + r[2] * t1;
    p[3] = q[3] * t0 + r[3] * t1;
  }
}

ysdnm_var &ysdnm_motion::interpolate(ysdnm_var &v, double time){
	if(!this || !nkfl)
		return v;
	double f;
	int i;
	if(time < 0.)
		time = 0.;
	for(i = 0; i < nkfl && kfl[i].dt <= time; time -= kfl[i].dt, i++);
	keyframe &prev = kfl[i < nkfl ? i : nkfl-1], &next = kfl[i+1 < nkfl ? i+1 : nkfl-1];
	v.ysdnm_var::~ysdnm_var();
	v = *prev.dup();
	if(time == 0. || &prev == &next)
		return v;
	for(i = 0; i < prev.bones; i++){
		for(int j = 0; j < next.bones; j++) if(!strcmp(next.bonenames[j], prev.bonenames[i])){
			slerp(v.bonerot[i], prev.bonerot[i], next.bonerot[j], time / prev.dt);
			Vec3d::atoc(&v.bonerot[i][4]) = Vec3d::atoc(&prev.bonerot[i][4]) * (1. - time / prev.dt) + Vec3d::atoc(&next.bonerot[j][4]) * time / prev.dt;
			break;
		}
	}
	return v;
}

void ysdnm_motion::save(std::ostream &os){
	os << "ysdnm_motion " << nkfl << std::endl;
	os << "{" << std::endl;
	for(int i = 0; i < nkfl; i++){
		os << "\tkeyframe " << kfl[i].bones << " " << kfl[i].dt << std::endl;
		os << "\t{" << std::endl;
		for(int j = 0; j < kfl[i].bones; j++){
			os << "\t\t\"" << kfl[i].bonenames[j] << "\"";
			for(int k = 0; k < 7; k++)
				os << " " << std::setprecision(15) << kfl[i].bonerot[j][k];
			os << std::endl;
		}
		os << "\t}" << std::endl;
	}
	os << "}" << std::endl;
}

void ysdnm_motion::save2(std::ostream &os){
	os << "model_motion " << nkfl << std::endl;
	os << "{" << std::endl;
	for(int i = 0; i < nkfl; i++){
		os << "\tkeyframe " << kfl[i].bones << " " << kfl[i].dt << std::endl;
		os << "\t{" << std::endl;
		for(int j = 0; j < kfl[i].bones; j++){
			os << "\t\t\"" << kfl[i].bonenames[j] << "\"";
			for(int k = 0; k < 7; k++)
				os << " " << std::setprecision(15) << kfl[i].bonerot[j][k];
			os << " " << (kfl[i].visible ? kfl[i].visible[j] : 1.);
			os << std::endl;
		}
		os << "\t}" << std::endl;
	}
	os << "}" << std::endl;
}

bool ysdnm_motion::load(std::istream &is){
	try{
	ysdnm_motion::~ysdnm_motion();
	std::string line;
	char buf[512];
	is.getline(buf, sizeof buf);
	line = std::string(buf);
	nkfl = atoi(line.substr(line.find(' ')).c_str());
	is.getline(buf, sizeof buf);
	kfl = (keyframe*)malloc(nkfl * sizeof *kfl);
	for(int i = 0; i < nkfl; i++){
		kfl[i].ysdnm_var::ysdnm_var();
		is.getline(buf, sizeof buf);
		sscanf(buf, "\tkeyframe %d %lg", &kfl[i].bones, &kfl[i].dt);
		is.getline(buf, sizeof buf);
		kfl[i].bonenames = new const char *[kfl[i].bones];
		kfl[i].bonerot = new double [kfl[i].bones][7];
		for(int j = 0; j < kfl[i].bones; j++){
			is.getline(buf, sizeof buf);
			char *name0 = ::strchr(buf, '"')+1;
			char *name1 = ::strchr(name0, '"');
			kfl[i].bonenames[j] = new char[name1 - name0 + 1];
			::strncpy(const_cast<char*>(kfl[i].bonenames[j]), name0, name1 - name0 + 1);
			const_cast<char*>(kfl[i].bonenames[j])[name1 - name0] = '\0';
			std::istringstream iss(name1+1);
			for(int k = 0; k < 7; k++)
				iss >> kfl[i].bonerot[j][k];
			QUATNORMIN(&kfl[i].bonerot[j][0]);
		}
		is.getline(buf, sizeof buf);
	}
	is.getline(buf, sizeof buf);
	}
	catch(...){
		return false;
	}
	return true;
}


struct ysdnm_motion *YSDNM_MotionLoad(const char *fname){
	ysdnm_motion *ret = new ysdnm_motion;
	ret->load(std::ifstream(fname));
	return ret;
}

static ysdnmv_t sdnmv;

struct ysdnm_var *YSDNM_MotionInterpolate(struct ysdnm_motion **mot, double *time, int nmot){
	if(nmot){
		ysdnm_var *ret = new ysdnm_var[nmot];
		for(int i = 0; i < nmot; i++) if(mot[i]){
			mot[i]->interpolate(ret[i], time[i]);
			ret[i].next = i+1 < nmot ? &ret[i+1] : NULL;
		}
		return ret;
	}
	else
		return NULL;
}

void YSDNM_MotionInterpolateFree(struct ysdnm_var *mot){
	if(mot)
		delete[] mot;
/*	while(mot){
		ysdnm_var *next = mot->next;
		delete mot;
	}*/
}

void YSDNM_MotionSave(const char *fname, ysdnm_motion *mot){
	mot->save(std::ofstream(fname));
}

void YSDNM_MotionAddKeyframe(ysdnm_motion *mot, double dt){
	mot->addKeyframe(dt);
}
