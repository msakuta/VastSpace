#ifndef AMAT2_H
#define AMAT2_H

typedef double amat2_t[4];

#define MAT2VP(vr,ma,vs) ((vr)[0] = (ma)[0] * (vs)[0] + (ma)[1] * (vs)[1], (vr)[1] = (ma)[2] * (vs)[0] + (ma)[3] * (vs)[1])

#endif
