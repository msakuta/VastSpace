/*
 * calc.h
 * simple expression calculator
 */
#ifndef MASAHIRO_CALC_P_H
#define MASAHIRO_CALC_P_H
#include "calc.h"

/* Keep linker symbol namespace clearer */
#define lasterrpos calc_lasterrpos
#define cerrno calc_cerrno
#define tokchar_table calc_tokchar_table

/* Shared macro */
#define chtype(c) (tokchar_table[(c) % 128])
#define TOKEN (1<<2) /* if can form a part of a token */
#define OPERA (1<<3) /* if can be a part of an operator */
#define IDENT (1<<4) /* if can be a part of an identifier */
#define IDHED (1<<5) /* if can be first character of an identifier */
#define PUNCT (1<<6) /* if it's punctuator */
enum calc_chartype{
	CT_0,  /* undefined */
	CT_SP, /* spaces */
	CT_MU, /* multiplicator */
	CT_AD, /* addition */
	CT_PA, /* parenthesis */
	NUM_CT
};

/* internal shared variables */
extern const char *lasterrpos;
extern enum calc_errno cerrno;
extern const unsigned char tokchar_table[128];

#endif
