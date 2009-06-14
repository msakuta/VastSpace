#include "clib/amat3.h"

const amat3_t mat3identity = {
	1,0,0,
	0,1,0,
	0,0,1,
};

void matinv(amat3_t mr, const amat3_t ma){
	double det;
	det = MATDET(ma);
	if(det == 0.)
		return;
	MATINV(mr, ma);
}
