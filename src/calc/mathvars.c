/*
 * mathvars.c
 * mathematical variables (precisely functions and constants)
 * if only union initialization is possible...
 */

#include "calc.h"
#include <clib/rseq.h>
#include <assert.h>
#include <math.h> /* pow */
#include <stddef.h>

static int init = 0;
static struct var vars[] = {
	{"pi", CALC_D},
	{"e", CALC_D},
	{"exp", CALC_F},
	{"log", CALC_F},
	{"pow", CALC_F},
	{"sin", CALC_F},
	{"cos", CALC_F},
	{"tan", CALC_F},
	{"asin", CALC_F},
	{"acos", CALC_F},
	{"atan", CALC_F},
	{"atan2", CALC_F},
	{"log10", CALC_F},
	{"sqrt", CALC_F},
	{"rand", CALC_F},
};
static struct varlist vl = {sizeof vars/sizeof*vars, vars, NULL};

static double frand(){
	static struct random_sequence rs;
	static int init = 0;
	if(!init)
		init_rseq(&rs, time(NULL));
	return drseq(&rs);
}

const struct varlist *calc_mathvars(){
	if(init) return &vl;
	init = 1;
	vars[0].value.d = 3.14159265358979;
	vars[1].value.d = 2.71828182845905;
	vars[2].value.f.p = exp;
	vars[2].value.f.args = 1;
	vars[3].value.f.p = log;
	vars[3].value.f.args = 1;
	vars[4].value.f.p = pow;
	vars[4].value.f.args = 2;
	vars[5].value.f.p = sin;
	vars[5].value.f.args = 1;
	vars[6].value.f.p = cos;
	vars[6].value.f.args = 1;
	vars[7].value.f.p = tan;
	vars[7].value.f.args = 1;
	vars[8].value.f.p = asin;
	vars[8].value.f.args = 1;
	vars[9].value.f.p = acos;
	vars[9].value.f.args = 1;
	vars[10].value.f.p = atan;
	vars[10].value.f.args = 1;
	vars[11].value.f.p = atan2;
	vars[11].value.f.args = 2;
	vars[12].value.f.p = log10;
	vars[12].value.f.args = 1;
	vars[13].value.f.p = sqrt;
	vars[13].value.f.args = 1;
	vars[14].value.f.p = frand;
	vars[14].value.f.args = 0;
	return &vl;
}
