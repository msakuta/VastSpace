#include "clib/aquatrot.h"
#include "clib/amat4.h"
#include <math.h>
/* treatment of quaternion is coordinate basis polarity independent,
  so it worth to make it into library. */

void quatrotation(aquat_t qr, double p, double sx, double sy, double sz){
	QUATROTATION(qr, p, sx, sy, sz);
}

void quatrotationv(aquat_t qr, double p, const avec3_t axis){
	QUATROTATION(qr, p, axis[0], axis[1], axis[2]);
}

void quatrotate(aquat_t qr, const aquat_t qa, double p, double sx, double sy, double sz){
	QUATROTATE(qr, qa, p, sx, sy, sz);
}

void quatrotatev(aquat_t qr, const aquat_t qa, double p, const avec3_t axis){
	QUATROTATE(qr, qa, p, axis[0], axis[1], axis[2]);
}



void quat2mat(amat4_t *mat, const aquat_t q){
#if 1
	double *m = *mat;
	double x = q[0], y = q[1], z = q[2], w = q[3];
	m[0] = 1. - (2 * q[1] * q[1] + 2 * q[2] * q[2]); m[1] = 2 * q[0] * q[1] + 2 * q[2] * q[3]; m[2] = 2 * q[0] * q[2] - 2 * q[1] * q[3];
	m[4] = 2 * x * y - 2 * z * w; m[5] = 1. - (2 * x * x +  2 * z * z); m[6] = 2 * y * z + 2 * x * w;
	m[8] = 2 * x * z + 2 * y * w; m[9] = 2 * y * z - 2 * x * w; m[10] = 1. - (2 * x * x + 2 * y * y);
	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.;
	m[15] = 1.;
#else
	aquat_t r = {1., 0., 0.}, qr;
	aquat_t qc;
	QUATCNJ(qc, q);
	VECTOQUAT(r, avec3_100);
	QUATMUL(qr, q, r);
	QUATMUL(&(*mat)[0], qr, qc);
	(*mat)[3] = 0.;
/*	assert(mat[3] == 0.);*/
	VECTOQUAT(r, avec3_010);
	QUATMUL(qr, q, r);
	QUATMUL(&(*mat)[4], qr, qc);
/*	assert(mat[7] == 0.);*/
	(*mat)[7] = 0.;
	VECTOQUAT(r, avec3_001);
	QUATMUL(qr, q, r);
	QUATMUL(&(*mat)[8], qr, qc);
/*	assert(mat[11] == 0.);*/
	(*mat)[11] = 0.;
	VECNULL(&(*mat)[12]);
	(*mat)[15]=1.;
/*	{
		amat4_t mat2;
		double tw, scale;
		double axis[3], angle;
		tw = acos(q[3]) * 2.;
		scale = sin(tw / 2.);
		axis[0] = q[0] / scale;
		axis[1] = q[1] / scale;
		axis[2] = q[2] / scale;
		angle = tw * 360 / 2. / M_PI;
		glPushMatrix();
		glLoadIdentity();
		glRotated(angle, axis[0], axis[1], axis[2]);
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		glPopMatrix();
	}*/
#endif
}

void quat2imat(amat4_t *mat, const aquat_t pq){
	aquat_t q, qr;
	QUATCNJ(q, pq);
	quat2mat(mat, q);
	QUATMUL(qr, q, pq);
}
void quatrot(avec3_t ret, const aquat_t q, const avec3_t src){
	aquat_t r = {1., 0., 0.}, qr;
	aquat_t qc, qret;
	QUATCNJ(qc, q);
	VECTOQUAT(r, src);
	QUATMUL(qr, q, r);
	QUATMUL(qret, qr, qc);
	VECCPY(ret, qret);
}

void quatirot(avec3_t ret, const aquat_t q, const avec3_t src){
	aquat_t r = {1., 0., 0.}, qr;
	aquat_t qc, qret;
	QUATCNJ(qc, q);
	VECTOQUAT(r, src);
	QUATMUL(qr, qc, r);
	QUATMUL(qret, qr, q);
	VECCPY(ret, qret);

	quatrot(ret, qc, src);
}

void quatrotquat(aquat_t ret, const avec3_t v, const aquat_t src){
	aquat_t q, qr;
	VECTOQUAT(q, v);
	QUATMUL(qr, q, src);
	QUATADDIN(qr, src);
	QUATNORM(ret, qr);
}

void quatdirection(aquat_t ret, const avec3_t dir){
	avec3_t v, dr;
	double p;
	VECNORM(dr, dir);

	/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
	ret[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

	if(ret[3] == 1.){
		VECNULL(ret);
	}
	else if(ret[3] == 0.){
		VECCPY(ret, avec3_010);
	}
	else{
		VECVP(v, avec3_001, dr);
		p = sqrt(1. - ret[3] * ret[3]) / VECLEN(v);
		VECSCALE(ret, v, p);
	}
}

void mat2quat(aquat_t q, const amat4_t m){
	double s;
	double tr = m[0] + m[5] + m[10] + 1.0;
	if(tr >= 1.0){
		s = 0.5 / sqrt(tr);
		q[0] = (m[9] - m[6]) * s;
		q[1] = (m[2] - m[8]) * s;
		q[2] = (m[4] - m[1]) * s;
		q[3] = 0.25 / s;
		return;
    }
	else{
        double max;
        if(m[5] > m[10])
			max = m[5];
        else
			max = m[10];
        
        if(max < m[0]){
			double x;
			s = sqrt(m[0] - (m[5] + m[10]) + 1.0);
			x = s * 0.5;
			s = 0.5 / s;
 			q[0] = x;
			q[1] = (m[4] - m[1]) * s;
			q[2] = (m[2] - m[8]) * s;
			q[3] = (m[9] - m[6]) * s;
			return;
		}
		else if(max == m[5]){
			double y;
			s = sqrt(m[5] - (m[10] + m[0]) + 1.0);
			y = s * 0.5;
            s = 0.5 / s;
			q[0] = (m[4] - m[1]) * s;
			q[1] = y;
			q[2] = (m[9] + m[6]) * s;
			q[3] = (m[2] + m[8]) * s;
			return;
        }
		else{
			double z;
			s = sqrt(m[10] - (m[0] + m[5]) + 1.0);
			z = s * 0.5;
			s = 0.5 / s;
			q[0] = (m[2] + m[8]) * s;
			q[1] = (m[9] + m[6]) * s;
			q[2] = z;
			q[3] = (m[4] + m[1]) * s;
			return;
        }
    }

}

void imat2quat(aquat_t q, const amat4_t m){
	aquat_t q1;
	mat2quat(q1, m);
	QUATCNJ(q, q1);
}
