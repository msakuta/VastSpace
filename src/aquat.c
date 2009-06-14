#include "clib/aquat.h"
#include <string.h>
#include <math.h>

const aquat_t quatidentity = {0,0,0,1};

void quatmul(aquat_t qr, const aquat_t qa, const aquat_t qb){
	QUATMUL(qr, qa, qb);
}

void quatimul(aquat_t qr, const aquat_t qa, const aquat_t qb){
	QUATIMUL(qr, qa, qb);
}

void quatslerp(aquat_t p, const aquat_t q, const aquat_t r, const double t)
{
  double qr = q[0] * r[0] + q[1] * r[1] + q[2] * r[2] + q[3] * r[3];
  double ss = 1.0 - qr * qr;
  
  if (ss == 0.0) {
    p[0] = q[0];
    p[1] = q[1];
    p[2] = q[2];
    p[3] = q[3];
  }
  else if(!memcmp(q, r, sizeof(double[4]))){
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

