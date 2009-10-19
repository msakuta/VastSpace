/*
 * calc.c
 * simple formula calculator
 * 
 */

/*
--- Syntax Definition ---
Ver 1{

constant:
	literal arithmetic constant that strtod can parse

multiplicative-expression:
	constant
	multiplicative-expression * constant
	[multiplicative-expression] / constant

additive-expresson:
	multiplicative-expression
	additive-expression + multiplicative-expression
	[additive-expression] - multiplicative-expression

whole:
	additive-expression

}
Version 1 is really not more than simple parser of number sequences.
If multiplicative-expression part of division operator (/) is omitted, it is
assumed to be 1.
If additive-expression part of addition operator (+) is omitted, it is assumed
to be 0, which means the operator is unary negation operator.

Ver 2{

primary-expression:
	constant
	( additive-expression )

multiplicative-expression:
	primary-expression
	multiplicative-expression * primary-expression
	[multiplicative-expression] / primary-expression

additive-expresson:
	multiplicative-expression
	additive-expression + multiplicative-expression
	[additive-expression] - multiplicative-expression

whole:
	additive-expression

}
Version 2 introduces grouping of expressions by parentheses.

*/

#include "calc_p.h"
#include <assert.h>
#include <math.h> /* pow */
#include <ctype.h>
#include <string.h> /* strlen, strcmp, strtok */
#include <stdio.h>
#include <stdlib.h> /* malloc, atof */
#include <setjmp.h>

/* stop debug outputs */
#ifdef NDEBUG
#define fprintf
#endif

static const char *lasterr = NULL;

/* Operator definition */
/*#define isoprtr(c) ((c)=='*'||(c)=='/'||(c)=='+'||(c)=='-')*/
#define isoprtr(c) (chtype(c) & OPERA)

/* calc internal structure */
typedef struct{
	jmp_buf env;
	const char *err;
	int ss; /* stack nest size */
#ifndef NDEBUG
	unsigned tok, exp; /* nextok and pretok's invocation count, nexexp and preexp's respectively. */
#endif
} calc_t;

static double add(calc_t *ca, const char **pp, size_t len);

typedef struct{const char *s; size_t l;} token;

/* search through and find a token forward */
static const char *nextok(const char **pp, token *t){
	char lc;
	const char *p = *pp;
	size_t sz;
	if(!p) return NULL;
	while(isspace(*p) || *p == '\0'){
		if(!*p) return *pp = NULL; /* all but spaces; no token */
		p++; /* truncate leading spaces prior to enter token body */
	}
	fprintf(stderr, "nextok: token body: \"%s\"\n", p);
	t->s = p; /* begin token body */
	lc = '\0';
	sz = 0;
	/* while the same type of token component continues, these characters belong to the same token. */
	while(*p && !isspace(*p) && lc != '(' && lc != ')' && (lc == '\0' || *p != '(' && *p != ')') && (isdigit(lc) == isdigit(*p) || isoprtr(lc) == isoprtr(*p))){
		if(!*p) return *pp = p;
		/* there's no plan to add an operator made of multiple chars in
		  level 1 and 2, so it's safe to determine by a char. */
		if(lc == '\0' && isoprtr(*p)){
			t->l = 1;
			return *pp = p + 1;
		}
		lc = *p++;
		sz++;
	}
	t->l = sz;
	return *pp = p;
}

/* search through and find a token backward */
static const char *pretok(const char **pp, const char *begin, token *t){
	char lc;
	const char *p;
	size_t sz;
	if(!*pp) return NULL;
	p = *pp-1;
	while(isspace(*p) || p == begin-1){
		if(p == begin-1){
			fprintf(stderr, "pretok empty\n");
			return *pp = NULL; /* all but spaces; no token */
		}
		p--; /* truncate trailing spaces prior to enter token body */
	}
	lc = '\0';
	sz = 0;
	/* while the same type of token component continues, these characters belong to the same token. */
	while(!isspace(*p) && lc != '(' && lc != ')' && (lc == '\0' || *p != '(' && *p != ')') && (isdigit(lc) == isdigit(*p) || isoprtr(lc) == isoprtr(*p))){
		if(p == begin-1){
			p++;
			t->s = p;
			t->l = sz;
			fprintf(stderr, "pretok end: \"%.*s\" (%d)\n", sz, p, sz);
			return *pp = p;
		}
		/* there's no plan to add an operator made of multiple chars in
		  level 1 and 2, so it's safe to determine by a char. */
		if(lc == '\0' && isoprtr(*p)){
			t->s = p;
			t->l = 1;
			return *pp = p;
		}
		lc = *p--;
		sz++;
	}
	p++;
	t->s = p;
	t->l = sz;
	fprintf(stderr, "pretok: \"%.*s\" (%d)\n", sz, p, sz);
	return *pp = p;
}

#ifndef NDEBUG
#	define nextok(pp,pt) (ca->tok++, nextok(pp,pt))
#	define pretok(pp,ep,pt) (ca->tok++, pretok(pp,ep,pt))
#endif

/* find next expression; those enclosed by parentheses are treated as one */
static const char *nexexp(calc_t *ca, const char **pp, token *pt){
	token t;
	if(!nextok(pp, &t)) return NULL;
	if(t.l == 1 && *t.s == ')') lasterrpos = t.s, longjmp(ca->env, CALC_OP);
	if(t.l == 1 && *t.s == '('){ /* if we find opening parenthesis, find matching parenthesis */
		const char *const start = t.s+1; /* reserve the beginning */
		int nest = 1; /* nesting level */
		while(nextok(pp, &t)) if(t.l == 1 && *t.s == '(') nest++;
		else if(t.l == 1 && *t.s == ')' && 0 == --nest){
			pt->l = t.s - start;
			pt->s = start;
			return *pp;
		}
		lasterrpos = start;
		longjmp(ca->env, CALC_CP); /* no luck to find matching parenthesis */
	}
	*pt = t; /* if the first token is not a parenthesis, it should be evaluated by itself. */
	return *pp;
}

/* find previous expression; those enclosed by parentheses are treated as one */
static const char *preexp(calc_t *ca, const char **pp, const char *begin, token *pt){
	token t;
	if(!pretok(pp, begin, &t)) return NULL;
	if(t.l == 1 && *t.s == '(') lasterrpos = t.s, longjmp(ca->env, CALC_CP);
	if(t.l == 1 && *t.s == ')'){ /* if we find closing parenthesis, find matching parenthesis */
		const char *const start = t.s; /* reserve the beginning */
		int nest = 1; /* nesting level */
		while(pretok(pp, begin, &t)){
		fprintf(stderr, "preexp: \"%.*s\" nest = %d\n", t.l, t.s, nest);
		if(t.l == 1 && *t.s == ')') nest++;
		else if(t.l == 1 && *t.s == '(' && 0 == --nest){
			pt->l = start - (t.s+1);
			pt->s = t.s+1;
			fprintf(stderr, "preexp: \"%.*s\" (%lu)\n", pt->l, pt->s, pt->l);
			return *pp;
		}}
		lasterrpos = start;
		longjmp(ca->env, CALC_OP); /* no luck to find matching parenthesis */
	}
	*pt = t; /* if the first token is not a parenthesis, it should be evaluated by itself. */
	return *pp;
}

#ifndef NDEBUG
#	define nexexp(ca,pp,pt) (ca->exp++, nexexp(ca,pp,pt))
#	define preexp(ca,pp,ep,pt) (ca->exp++, preexp(ca,pp,ep,pt))
#endif


#if 0 /* DELETEME */
static double mult(calc_t *ca, const char **pp, size_t len){
	token t;
	const char *s = *pp, *pend = &s[len];
	char *p;
	double o1, o2;
	fprintf(stderr, "[%d]enter mult: \"%.*s\" (%lu)\n", ca->ss, len, *pp, len);
	if(!nextok(&s, &t)) return 0;
	o1 = strtod(t.s, &p);
	fprintf(stderr, "mult: operand 1: %s%g = strtod(\"%.*s\")\n", p == t.s ? "parse failed: " : "", o1, t.l, t.s);
	if(t.s == p) return 0;

	/* retrieve operator */
	if(!nextok(&s, &t)){
		fprintf(stderr, "mult: no operator: return operand 1\n");
		*pp = s;
		return o1;
	}
	fprintf(stderr, "mult: operator: \"%.*s\" (%lu)\n", t.l, t.s, t.l);
	if(t.l == 1 && *t.s == '*'){
		const char *po2 = s, *poo2 = s;
		if(!nextok(&s, &t)) longjmp(ca->env, 2);
/*		o2 = strtod(t.s, &p);*/
		ca->ss++;
		o2 = mult(ca, &po2, pend - po2);
		ca->ss--;
/*		fprintf(stderr, "mult: operand 2: %s%g = strtod(\"%.*s\")\n", p == t.s ? "parse failed: " : "", o2, t.l, t.s);*/
		fprintf(stderr, "mult: operand 2: %g = mult(\"%.*s\")\n", o2, (int)(pend - poo2), poo2);
		if(t.s == p) return 0;
		*pp = s;
		return o1 * o2;
	}
	else if(t.l == 1 && *t.s == '/'){
		if(!nextok(&s, &t)) longjmp(ca->env, 2);
		o2 = strtod(t.s, &p);
		fprintf(stderr, "mult: operand 2: %s%g = strtod(\"%.*s\")\n", p == t.s ? "parse failed: " : "", o2, t.l, t.s);
		if(t.s == p) return 0;
		*pp = s;
		return o1 / o2;
	}
	else{
		fprintf(stderr, "mult: unknown operator\n");
		longjmp(ca->env, 1);
	}
	return 0;
}
#endif

/* read backwards */
static double mult2(calc_t *ca, const char **pp, size_t len){
	token t;
	const char *pend = *pp, *s = &pend[len];
	char *p;
	double o1, o2;
	fprintf(stderr, "[%d]enter mult: \"%.*s\" (%lu)\n", ca->ss, len, *pp, len);
	if(!pretok(&s, pend, &t)) lasterrpos = *pp, longjmp(ca->env, CALC_NOP);
	o2 = strtod(t.s, &p);
	fprintf(stderr, "mult: operand 2: %s%g = strtod(\"%.*s\")\n", p == t.s ? "parse failed: " : "", o2, t.l, t.s);
	if(t.s == p) lasterrpos = p, longjmp(ca->env, CALC_FC);

	/* retrieve operator */
	if(!pretok(&s, pend, &t)){
		fprintf(stderr, "mult: no operator: return operand 2\n");
		*pp = s;
		return o2;
	}
	fprintf(stderr, "mult: operator: \"%.*s\" (%lu)\n", t.l, t.s, t.l);
	if(t.l == 1 && *t.s == '*'){
		const char *po = pend;
		ca->ss++;
		o1 = mult2(ca, &pend, s - pend);
		ca->ss--;
		fprintf(stderr, "mult: operand 1: %g = mult(\"%.*s\")\n", o1, (int)(s - po), po);
		*pp = s;
		return o1 * o2;
	}
	else if(t.l == 1 && *t.s == '/'){
		const char *po = pend;
		if(0 == o2) lasterrpos = t.s, longjmp(ca->env, CALC_DIV); /* zero divide is signaled */
#if 0
		if(pend == s) /* if dividend is omitted, assume it to be 1 */
#elif 0
		p = s;
		if(!pretok(&p, pend, &t))
#else
		/* if no token precedes the division operator, the first operand is assumed
		  to be 1. this check is a little not enough by checking pointer equivalency.
		  even if the string preceding the operator have a length, we must check
		  over each character in order to tell wheter it really has an operand.
		  inlining the routine made this check least heavy possible. */
		for(p = s; p != pend; p--) if(!isspace(p[-1])) break;
		if(pend == p)
#endif
		{
			o1 = 1;
			fprintf(stderr, "div: operand 1 omitted: 1 = \"%.*s\"\n", (int)(s - po), po);
		}
		else{
			ca->ss++;
			o1 = mult2(ca, &pend, s - pend);
			ca->ss--;
			fprintf(stderr, "div: operand 1: %g = mult(\"%.*s\")\n", o1, (int)(s - po), po);
		}
		*pp = s;
		return o1 / o2;
	}
	else{
		fprintf(stderr, "mult: unknown operator\n");
		lasterrpos = t.s;
		longjmp(ca->env, CALC_UOP);
	}
	return 0;
}

#if 0 /* DELETEME */
static double add(calc_t *ca, const char **pp, size_t len){
	token t;
	const char *s = *pp, *pend = &s[len];
	char *p;
	double o1, o2;
	fprintf(stderr, "[%d]enter add: \"%.*s\" (%lu)\n", ca->ss, len, *pp, len);

	/* find the first occasion of + or - operator */
	while(s < pend && nextok(&s, &t) && !(t.l == 1 && (*t.s == '+' || *t.s == '-')));
	if(pend <= s || !s) return mult2(ca, pp, len); /* if not, assume it's multiplicative operation */
	fprintf(stderr, "add: operator: \"%.*s\" (%lu)\n", t.l, t.s, t.l);

	/* if we do find a + or -, calculate the first (possibly compound) operand as multiplication */
	{
	const char *p = *pp;
	ca->ss++;
	o1 = mult2(ca, pp, t.s - *pp); /* tell length till just before the + or - */
	ca->ss--;
	fprintf(stderr, "add: operand 1: %g = mult(\"%.*s\")\n", o1, (int)(t.s - p), p);
	}

	{ /* parse the second operand, this time it may include additive operators */
		const char *po2 = &t.s[t.l], *poo2 = po2;
		ca->ss++;
		o2 = add(ca, &po2, pend - po2);
		ca->ss--;
		fprintf(stderr, "add: operand 2: %g = mult(\"%.*s\")\n", o2, (int)(pend - poo2), poo2);
		*pp = s;
		return *t.s == '+' ? o1 + o2 : o1 - o2;
	}
}
#endif

/* the original add had a bug that it associates in wrong direction */
static double add1(calc_t *ca, const char **pp, size_t len){
	token t;
	const char *pend = *pp, *s = &pend[len];
	double o1, o2;
	fprintf(stderr, "[%d]enter add: \"%.*s\" (%lu)\n", ca->ss, len, *pp, len);

	/* find the first occasion of + or - operator */
	while(pretok(&s, pend, &t) && !(t.l == 1 && (*t.s == '+' || *t.s == '-')));
	if(!s) return mult2(ca, pp, len); /* if not, assume it's multiplicative operation */
	fprintf(stderr, "add: operator: \"%.*s\" (%lu)\n", t.l, t.s, t.l);

	/* if we do find a + or -, calculate the first (possibly compound) operand as multiplication */
	{
	const char *p = t.s+1;
	ca->ss++;
	o2 = mult2(ca, &p, &(*pp)[len] - p); /* tell length till just before the + or - */
	ca->ss--;
	fprintf(stderr, "add: operand 2: %g = mult(\"%.*s\")\n", o2, (int)(&(*pp)[len] - p), p);
	}

	{
		const char *p = t.s; /* save pointer to end of operand 1 i.e. pointer to the operator */
		/* if operator 1 is lacking, it's negation unary operator */
		if(*p == '-' && !pretok(&s, pend, &t)) return -o2;

		/* parse the second operand, this time it may include additive operators */
		ca->ss++;
		o1 = add1(ca, pp, p - pend);
		ca->ss--;
		fprintf(stderr, "add: operand 1: %g = add(\"%.*s\")\n", o1, (int)(p - pend), pend);
		*pp = s;
		return *p == '+' ? o1 + o2 : o1 - o2;
	}
}

/* turn a formula into double */
double calc(const char **s){
	int i;
	double ret = 1.;
	char *c;
	const char *p = *s;
	calc_t ca;
	ca.ss = 0; /* top level */
	lasterr = lasterrpos = NULL;

/*	fprintf(stderr, "calc: parse error: ");*/
	switch(setjmp(ca.env)){
		case 0: fprintf(stderr, "calc: setjump()\n"); break;
		case CALC_UOP: fprintf(stderr, lasterr = "unexpected operator\n", (int)(*s - p)); cerrno = CALC_UOP; return 0;
		case CALC_NOP: fprintf(stderr, lasterr = "missing operand\n", (int)(*s - p)); cerrno = CALC_NOP; return 0;
		case CALC_FC: fprintf(stderr, lasterr = "floating constant parse error\n", (int)(*s - p)); cerrno = CALC_FC; return 0;
		case CALC_DIV: fprintf(stderr, lasterr = "division by 0\n", (int)(*s - p)); cerrno = CALC_DIV; return 0;
		case CALC_OP: fprintf(stderr, lasterr = "no match \'(\' parenthesis\n", (int)(*s - p)); cerrno = CALC_OP; return 0;
		case CALC_CP: fprintf(stderr, lasterr = "no match \')\' parenthesis\n", (int)(*s - p)); cerrno = CALC_CP; return 0;
		default: fprintf(stderr, lasterr = "unknown\n"); return 0;
	}

	ret = add1(&ca, s, strlen(*s));

	return ret;
}

/**********************************************************
	Version 2
**********************************************************/

static double add2(calc_t *, const char **, size_t len);

/* read backwards */
static double mult3(calc_t *ca, const char **pp, size_t len){
	token t;
	const char *pend = *pp, *s = &pend[len];
	char *p;
	double o1, o2;
	fprintf(stderr, "[%d]enter mult: \"%.*s\" (%lu)\n", ca->ss, len, *pp, len);
	if(!preexp(ca, &s, pend, &t)) lasterrpos = pend, longjmp(ca->env, CALC_NOP);
	{ /* primary expression is expected here */
		token t1;
		const char *s1 = &pend[len];
		pretok(&s1, pend, &t1);
		if(t1.l == t.l){ /* if preexp and pretok tokenizes the same way, it's an atom(not enclosed by parentheses) */
			o2 = strtod(t.s, &p); /* and an atom is constant expression */
			/* the strtod leaves p intact if it failed to parse the given string. sort of missing operand? */
			if(t.s == p) lasterrpos = p, longjmp(ca->env, CALC_FC);
		}
		else{ /* otherwise, enter additive-expression between the parenthesis */
			s1 = t.s;
			ca->ss++;
			o2 = add2(ca, &s1, t.l);
			ca->ss--;
		}
		fprintf(stderr, "mult: operand 2: %s%g = %s(\"%.*s\")\n", t1.l == t.l && p == t.s ? "parse failed: " : "",
			o2, t1.l == t.l ? "strtod" : "add", t.l, t.s);
	}

	/* retrieve operator */
	if(!pretok(&s, pend, &t)){
		fprintf(stderr, "mult: no operator: return operand 2\n");
		*pp = s;
		return o2;
	}
	fprintf(stderr, "mult: operator: \"%.*s\" (%lu)\n", t.l, t.s, t.l);
	if(t.l == 1 && *t.s == '*'){
		const char *po = pend;
		ca->ss++;
		o1 = mult3(ca, &pend, s - pend);
		ca->ss--;
		fprintf(stderr, "mult: operand 1: %g = mult(\"%.*s\")\n", o1, (int)(s - po), po);
		*pp = s;
		return o1 * o2;
	}
	else if(t.l == 1 && *t.s == '/'){
		const char *po = pend;
		if(0 == o2) lasterrpos = t.s, longjmp(ca->env, CALC_DIV); /* zero divide is signaled */
#if 0
		if(pend == s) /* if dividend is omitted, assume it to be 1 */
#else
		for(p = s; p != pend; p--) if(!isspace(p[-1])) break;
		if(pend == p)
#endif
			o1 = 1;
		else{
			ca->ss++;
			o1 = mult3(ca, &pend, s - pend);
			ca->ss--;
		}
		fprintf(stderr, "div: operand 1: %g = mult(\"%.*s\")\n", o1, (int)(s - po), po);
		*pp = s;
		return o1 / o2;
	}
	else{
		fprintf(stderr, "mult: unknown operator\n");
		lasterrpos = t.s;
		longjmp(ca->env, CALC_UOP);
	}
	return 0;
}

static double add2(calc_t *ca, const char **pp, size_t len){
	token t;
	const char *pend = *pp, *s = &pend[len];
	double o1, o2;
	fprintf(stderr, "[%d]enter add: \"%.*s\" (%lu)\n", ca->ss, len, *pp, len);

	/* find the first occasion of + or - operator */
	while(preexp(ca, &s, pend, &t) && !(t.l == 1 && (*t.s == '+' || *t.s == '-')));
	if(!s) return mult3(ca, pp, len); /* if not, assume it's multiplicative operation */
	fprintf(stderr, "add: operator: \"%.*s\" (%lu)\n", t.l, t.s, t.l);

	/* if we do find a + or -, calculate the first (possibly compound) operand as multiplication */
	{
	const char *p = t.s+1;
	ca->ss++;
	o2 = mult3(ca, &p, &(*pp)[len] - p); /* tell length till just before the + or - */
	ca->ss--;
	fprintf(stderr, "add: operand 2: %g = mult(\"%.*s\")\n", o2, (int)(&(*pp)[len] - p), p);
	}

	{
		const char *p = t.s; /* save pointer to end of operand 1 i.e. pointer to the operator */
		/* if operator 1 is lacking, it's negation unary operator */
		if(*p == '-' && !pretok(&s, pend, &t)) return -o2;

		/* parse the second operand, this time it may include additive operators */
		ca->ss++;
		o1 = add2(ca, pp, p - pend);
		ca->ss--;
		fprintf(stderr, "add: operand 1: %g = add(\"%.*s\")\n", o1, (int)(p - pend), pend);
		*pp = s;
		return *p == '+' ? o1 + o2 : o1 - o2;
	}
}


/* Version 2 main */
double calc2(const char **s){
	int i;
	double ret = 1.;
	char *c;
	const char *p = *s;
	calc_t ca = {0};
	ca.ss = 0; /* top level */
	cerrno = 0;
	lasterr = lasterrpos = NULL;

/*	fprintf(stderr, "calc: parse error: ");*/
	switch(setjmp(ca.env)){
		case 0: fprintf(stderr, "calc2: setjump()\n"); break;
		case CALC_UOP: fprintf(stderr, lasterr = "unexpected operator\n", (int)(*s - p)); cerrno = CALC_UOP; return 0;
		case CALC_NOP: fprintf(stderr, lasterr = "missing operand\n", (int)(*s - p)); cerrno = CALC_NOP; return 0;
		case CALC_FC: fprintf(stderr, lasterr = "floating constant parse error\n", (int)(*s - p)); cerrno = CALC_FC; return 0;
		case CALC_DIV: fprintf(stderr, lasterr = "division by 0\n", (int)(*s - p)); cerrno = CALC_DIV; return 0;
		case CALC_OP: fprintf(stderr, lasterr = "no match \'(\' parenthesis\n", (int)(*s - p)); cerrno = CALC_OP; return 0;
		case CALC_CP: fprintf(stderr, lasterr = "no match \')\' parenthesis\n", (int)(*s - p)); cerrno = CALC_CP; return 0;
		default: fprintf(stderr, lasterr = "unknown\n"); return 0;
	}

	ret = add2(&ca, s, strlen(*s));

#ifndef NDEBUG
	fprintf(stderr, "calc2: tok = %d, exp = %d \n", ca.tok, ca.exp);
#endif

	return ret;
}


