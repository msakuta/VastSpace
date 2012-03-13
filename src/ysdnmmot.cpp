#include "yssurf.h"
extern "C"{
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
	if(bonevar){
		ret->bonevar = new ysdnm_bone_var[bones];
		for(int i = 0; i < bones; i++){
			char *name = new char[strlen(bonevar[i].name) + 1];
			strcpy(name, bonevar[i].name);
			ret->bonevar[i].name = name;
			ret->bonevar[i].rot = bonevar[i].rot;
			ret->bonevar[i].pos = bonevar[i].pos;
			ret->bonevar[i].visible = bonevar[i].visible;
		}
	}
	ret->bones = bones;
	return ret;
}

ysdnm_bone_var *ysdnm_var::findBone(const char *name){
	for(int i = 0; i < bones; i++){
		if(!::strcmp(bonevar[i].name, name))
			return &bonevar[i];
	}
	return NULL;
}

ysdnm_bone_var *ysdnm_var::addBone(const char *name){
	ysdnm_bone_var *newbonevar = new ysdnm_bone_var[bones+1];
	memcpy(newbonevar, bonevar, bones * sizeof *bonevar);
	char *newname = new char[strlen(name)+1];
	strcpy(newname, name);
	newbonevar[bones].name = newname;
	newbonevar[bones].rot = quat_u;
	newbonevar[bones].pos.clear();
	newbonevar[bones].visible = 1.;
	bonevar = newbonevar;

	return &bonevar[bones++];
}

/// Amplify this modifier's offset and rotation by given factor.
///
/// The argument should be between 0 and 1, but values greater than 1
/// might work too.
ysdnm_var &ysdnm_var::amplify(double f){
	for(int i = 0; i < bones; i++){
		bonevar[i].rot = Quatd::slerp(Quatd(0,0,0,1), bonevar[i].rot, f);
		bonevar[i].pos *= f;
	}
	return *this;
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
		for(int j = 0; j < next.bones; j++) if(!strcmp(next.bonevar[j].name, prev.bonevar[i].name)){
			v.bonevar[i].rot = Quatd::slerp(prev.bonevar[i].rot, next.bonevar[j].rot, time / prev.dt);
			v.bonevar[i].pos = prev.bonevar[i].pos * (1. - time / prev.dt) + next.bonevar[j].pos * time / prev.dt;
			v.bonevar[i].visible = prev.bonevar[i].visible * (1. - time / prev.dt) + next.bonevar[j].visible * time / prev.dt;
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
			os << "\t\t\"" << kfl[i].bonevar[j].name << "\"";
			for(int k = 0; k < 4; k++)
				os << " " << std::setprecision(15) << kfl[i].bonevar[j].rot[k];
			for(int k = 0; k < 3; k++)
				os << " " << std::setprecision(15) << kfl[i].bonevar[j].pos[k];
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
			os << "\t\t\"" << kfl[i].bonevar[j].name << "\"";
			for(int k = 0; k < 4; k++)
				os << " " << std::setprecision(15) << kfl[i].bonevar[j].rot[k];
			for(int k = 0; k < 3; k++)
				os << " " << std::setprecision(15) << kfl[i].bonevar[j].pos[k];
			os << " " << kfl[i].bonevar[j].visible;
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

	// Obtain and remember very first token of the file, that's version string.
	int firstspace = line.find(' ');
	std::string firsttoken = line.substr(0, firstspace);
	int version = firsttoken == "model_motion";
	nkfl = atoi(line.substr(firstspace).c_str());

	is.getline(buf, sizeof buf);
	kfl = (keyframe*)malloc(nkfl * sizeof *kfl);
	for(int i = 0; i < nkfl; i++){
		kfl[i].ysdnm_var::ysdnm_var();
		is.getline(buf, sizeof buf);
		sscanf(buf, "\tkeyframe %d %lg", &kfl[i].bones, &kfl[i].dt);
		is.getline(buf, sizeof buf);
		kfl[i].bonevar = new ysdnm_bone_var [kfl[i].bones];
		for(int j = 0; j < kfl[i].bones; j++){
			is.getline(buf, sizeof buf);
			char *name0 = ::strchr(buf, '"')+1;
			char *name1 = ::strchr(name0, '"');
			char *newname = new char[name1 - name0 + 1];
			::strncpy(newname, name0, name1 - name0 + 1);
			newname[name1 - name0] = '\0';
			kfl[i].bonevar[j].name = newname;
			std::istringstream iss(name1+1);
			for(int k = 0; k < 4; k++)
				iss >> kfl[i].bonevar[j].rot[k];
			for(int k = 0; k < 3; k++)
				iss >> kfl[i].bonevar[j].pos[k];
			kfl[i].bonevar[j].rot.normin();
			if(version)
				iss >> kfl[i].bonevar[j].visible;
			else
				kfl[i].bonevar[j].visible = 1.;
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

struct ysdnm_var *YSDNM_MotionInterpolateAmp(struct ysdnm_motion **mot, double *time, int nmot, double *amplitude){
	if(nmot){
		ysdnm_var *ret = new ysdnm_var[nmot];
		for(int i = 0; i < nmot; i++) if(mot[i]){
			mot[i]->interpolate(ret[i], time[i]);
			ret[i].amplify(amplitude[i]);
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

void YSDNM_MotionSave2(const char *fname, ysdnm_motion *mot){
	mot->save2(std::ofstream(fname));
}

void YSDNM_MotionAddKeyframe(ysdnm_motion *mot, double dt){
	mot->addKeyframe(dt);
}
