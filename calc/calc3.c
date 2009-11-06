/*
 * calc3.c
 * function and constant values handling calculator
 * 
 */

/*
--- Syntax Definition ---

Ver 3{

primary-expression:
	identifier
	constant
	( additive-expression )

postfix-expression:
	primary-expression
	identifier ( argument-expression-list )

argument-expression-list:
	additive-expression
	argument-expression-list , additive-expression

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
Version 3 introduces variables and function calls, though variables cannot be
assigned a value in the expression; they must be predefined.

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
#if 1 || !defined NDEBUG
#define fprintf
#endif

#define numof(a) (sizeof(a)/sizeof*(a))

#ifdef _WIN32
#define iscsymf(a) __iscsymf(a)
#define iscsym(a) __iscsym(a)
#else
#define iscsymf(a) (isalpha(a)||(a)=='_')
#define iscsym(a) (isalnum(a)||(a)=='_')
#endif

/* Operator definition */
#define isoprtr(c) ((c)=='*'||(c)=='/'||(c)=='+'||(c)=='-')


/* calc internal structure */
typedef struct{
	jmp_buf env;
	const char *err;
	int ss; /* stack nest size */
	struct varlist *vl;
	int (*undname)(const char *, int, double *);
#ifndef NDEBUG
	unsigned tok, exp; /* nextok and pretok's invocation count, nexexp and preexp's respectively. */
#endif
} calc_t;


typedef struct{const char *s; size_t l;} token;

#define define_operator(s) {(s), sizeof(s)-1}
const static token opers[] = {
	define_operator("+"),
	define_operator("-"),
	define_operator("*"),
	define_operator("/"),
	define_operator(","),
	define_operator("("), /* strictly not an operator */
	define_operator(")"),
};

#if 1
/* search through and find a token forward */
static const char *nextok(const char **pp, token *t){
	int pos[numof(opers)] = {0};
	char lc;
	const char *p = *pp;
	size_t sz;
	if(!p) return NULL;
	while(isspace(*p) || *p == '\0'){
		if(!*p) return *pp = NULL; /* all but spaces; no token */
		p++; /* truncate leading spaces prior to enter token body */
	}
/*	fprintf(stderr, "nextok: token body: \"%s\"\n", p);*/
	t->s = p; /* begin token body */
	lc = '\0';
	sz = 0;
	/* while the same type of token component continues, these characters belong to the same token. */
	while(*p && !isspace(*p) && (lc == '\0' || (!!isalnum(lc) == !!isalnum(*p)))){
		int i;
		if(!*p) return *pp = p;
		for(i = 0; i < numof(pos); i++){
			if(pos[i] < 0) continue;
			if(*p != opers[i].s[pos[i]]){
				pos[i] = -1;
				continue;
			}
			if(++pos[i] == opers[i].l){
				t->l = opers[i].l;
				return *pp = p+1;
			}
		}
		lc = *p++;
		sz++;
	}
	t->l = sz;
	return *pp = p;
}
#else
static const char *nextok(const char **pp, token *t){
	char lc;
	const char *p = *pp;
	size_t sz;
	if(!p) return NULL;
	while(isspace(*p) || *p == '\0'){
		if(!*p) return *pp = NULL; /* all but spaces; no token */
		p++; /* truncate leading spaces prior to enter token body */
	}
/*	fprintf(stderr, "nextok: token body: \"%s\"\n", p);*/
	t->s = p; /* begin token body */
	lc = '\0';
	sz = 0;
	/* while the same type of token component continues, these characters belong to the same token. */
	while(*p && !isspace(*p) && lc != '(' && lc != ')' && (lc == '\0' || *p != '(' && *p != ')') && (isalpha(lc) == isalpha(*p) && isdigit(lc) == isdigit(*p) || (lc == ',') == (*p == ',') && isoprtr(lc) == isoprtr(*p))){
		if(!*p) return *pp = p;
		lc = *p++;
		sz++;
	}
	t->l = sz;
	return *pp = p;
}
#endif

#if 1
static const char *pretok(const char **pp, const char *begin, token *t){
	int pos[numof(opers)];
	char lc;
	int period = 0; /* period state */
	int skipopera = 0; /* whether skip checking of operators */
	int exponentsign = 0; /* such 1e-3 */
	const char *p;
	size_t sz;
	if(!*pp || *pp == begin) return NULL;
	p = *pp-1;
	while(p == begin-1 || isspace(*p)){
		if(p == begin-1){
/*			fprintf(stderr, "pretok empty\n");*/
			return *pp = NULL; /* all but spaces; no token */
		}
		p--; /* truncate trailing spaces prior to enter token body */
	}
	lc = '\0';
	sz = 0;
	{
		int i; /* align to tail */
		for(i = 0; i < numof(pos); i++) pos[i] = opers[i].l-1;
	}

	/* while the same type of token component continues, these characters belong to the same token. */
	while(p != begin-1 && !isspace(*p)){
		int i;

		/* period is treated specially in that it can be a part of numerical expression, but not
		  by alone. */
		if(*p == '.'){

			/* in any case, comma will not appear twice in a token. 
			  also, if adjacent character is not a decimal, it's treated as separate token.
			  but the latter is not the case when it is E notation in a exponential expression. */
			if(period || lc != '\0' && !isdigit(lc) && (lc != 'e' && lc != 'E' || p - begin < 1 || !isdigit(p[-1])))
				break;

			/* if this period is the last character in a token, mark it as so. */
			period = lc == '\0' ? 2 : 1;
		}
		else if(period && isdigit(*p)){
			/* if a period appeared, decimal numbers are considered the same type. */
		}

		/* here is another very special case, which involves a plus or minus sign in a
		  floating-point constant, but not an operator. 
		   we additionally check one character further back to be a decimal, to make
		  sure it's not part of identifier ending with "e". */
		else if(isdigit(lc) && (*p == '-' || *p == '+') && 2 <= p - begin && (p[-1] == 'e' || p[-1] == 'E') && isdigit(p[-2])){
			fprintf(stderr, "pretok: %c %c %c\n", lc, *p, p[-1]);

			/* again, no multiple sign characters in a token. */
			if(exponentsign)
				break;
			exponentsign = 1;

			/* now that we know this token no longer can be an operator, skip matching operators,
			  to prevent the sign operators to match. */
			skipopera = 1;
			p--;
			sz++;
		}
		else if(isdigit(*p) && exponentsign);
		else if(!(lc == '\0' || (lc == '.' ? iscsym(*p) : !!iscsym(lc) == !!iscsym(*p))))
			break;

		if(!skipopera) for(i = 0; i < numof(pos); i++){
			if(pos[i] < 0) continue;
			if(*p != opers[i].s[pos[i]]){
				pos[i] = -1;
				continue;
			}
			if(pos[i]-- == 0){
				t->s = p;
				t->l = opers[i].l;
				return *pp = p;
			}
		}
		lc = *p--;
		sz++;
	}
	if(p == begin-1){
		p++;
		t->s = p;
		t->l = sz;
/*			fprintf(stderr, "pretok end: \"%.*s\" (%d)\n", sz, p, sz);*/
		return *pp = p;
	}
	p++;
	t->s = p;
	t->l = sz;
/*	fprintf(stderr, "pretok: \"%.*s\" (%d)\n", sz, p, sz);*/
	return *pp = p;
}
#else
/* search through and find a token backward */
static const char *pretok(const char **pp, const char *begin, token *t){
	char lc;
	const char *p;
	size_t sz;
	if(!*pp) return NULL;
	p = *pp-1;
	while(isspace(*p) || p == begin-1){
		if(p == begin-1){
/*			fprintf(stderr, "pretok empty\n");*/
			return *pp = NULL; /* all but spaces; no token */
		}
		p--; /* truncate trailing spaces prior to enter token body */
	}
	lc = '\0';
	sz = 0;
	/* while the same type of token component continues, these characters belong to the same token. */
	while(!isspace(*p) && lc != '(' && lc != ')' && (lc == '\0' || *p != '(' && *p != ')') && (iscsymf(lc) == iscsymf(*p) && isdigit(lc) == isdigit(*p) || (lc == ',') == (*p == ',') && isoprtr(lc) == isoprtr(*p))){
		if(p == begin-1){
			p++;
			t->s = p;
			t->l = sz;
/*			fprintf(stderr, "pretok end: \"%.*s\" (%d)\n", sz, p, sz);*/
			return *pp = p;
		}
		lc = *p--;
		sz++;
	}
	p++;
	t->s = p;
	t->l = sz;
/*	fprintf(stderr, "pretok: \"%.*s\" (%d)\n", sz, p, sz);*/
	return *pp = p;
}
#endif

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
	fprintf(stderr, "preexp(\"%.*s\")\n", *pp - begin, begin);
	if(!pretok(pp, begin, &t)) return *pp = NULL;
	if(t.l == 1 && *t.s == '(') lasterrpos = t.s, longjmp(ca->env, CALC_CP);
	if(t.l == 1 && *t.s == ')'){ /* if we find closing parenthesis, find matching parenthesis */
		const char *const start = t.s; /* reserve the beginning */
		int nest = 1; /* nesting level */
		while(pretok(pp, begin, &t)){
/*		fprintf(stderr, "preexp: \"%.*s\" nest = %d\n", t.l, t.s, nest);*/
		if(t.l == 1 && *t.s == ')') nest++;
		else if(t.l == 1 && *t.s == '(' && 0 == --nest){
			pt->l = start - (t.s+1);
			pt->s = t.s+1;
			fprintf(stderr, "preexp: \"%.*s\" (%lu) \"%s\"\n", pt->l, pt->s, pt->l, *pp);
			return *pp;
		}}
		lasterrpos = start;
		longjmp(ca->env, CALC_OP); /* no luck to find matching parenthesis */
	}
	*pt = t; /* if the first token is not a parenthesis, it should be evaluated by itself. */
	fprintf(stderr, "preexp nop: \"%.*s\" (%lu) \"%s\"\n", pt->l, pt->s, pt->l, *pp);
	return *pp;
}

#ifndef NDEBUG
#	define nexexp(ca,pp,pt) (ca->exp++, nexexp(ca,pp,pt))
#	define preexp(ca,pp,ep,pt) (ca->exp++, preexp(ca,pp,ep,pt))
#	define longjmp(env,val) (fprintf(stderr, __FILE__"(%d): exception: %d\n", __LINE__, val), longjmp(env,val))
#endif



/**********************************************************
	Version 3
**********************************************************/

static double add(calc_t *, const char **, size_t len);

typedef struct arglist{double v; struct arglist *p;} arglist_t;
static double args(calc_t *ca, const char **pp, size_t len, struct var *pv, arglist_t *a){
	token t;
	const char *pend = *pp, *s = &pend[len];
	int c;
	fprintf(stderr, "[%d]enter args: \"%.*s\" (%lu)\n", ca->ss, len, *pp, len);

	/* if no arguments at all */
	if(!preexp(ca, &s, pend, &t)){
		fprintf(stderr, "args: %s() [0]\n", pv->name);
		c = 0;
		goto noargs;
	}

	/* find the first occasion of ',' operator */
	while(preexp(ca, &s, pend, &t) && !(t.l == 1 && *t.s == ','));
	fprintf(stderr, "args: t.s = \"%.*s\" (%lu)\n", t.l, t.s, t.l);
	if(!s){
		arglist_t *pa = a; /* arguments are kept in the stack frame. */
		fprintf(stderr, "args: %s(", pv->name);
		for(c = 1; pa; pa = pa->p, c++) /* count up arguments */
			fprintf(stderr, "%g, ", pa->v);
		fprintf(stderr, "%.*s) [%d]\n", len, pend, c);
noargs:
		if(c != pv->value.f.args) lasterrpos = pend, longjmp(ca->env, CALC_ARG); /* pair number of arguments, please */
		switch(c){
			case 0: return pv->value.f.p();
			case 1: return pv->value.f.p(add(ca, pp, len));
			case 2: return pv->value.f.p(add(ca, pp, len), a->v);
			case 3: return pv->value.f.p(a->p->v, a->v, add(ca, pp, len));
			case 4: return pv->value.f.p(a->p->p->v, a->p->v, a->v, add(ca, pp, len));
			default: lasterrpos = pend, longjmp(ca->env, CALC_ARG);
		}
	}
	fprintf(stderr, "args: operator: \"%.*s\" (%lu)\n", t.l, t.s, t.l);

	/* if we do find a ',', calculate the first (possibly compound) operand as add */
	{
		arglist_t a1;
		const char *p = t.s+1;
		ca->ss++;
		a1.v = add(ca, &p, &(*pp)[len] - p); /* tell length till just before the ',' */
		ca->ss--;
		fprintf(stderr, "args: arg: %g = post(\"%.*s\")\n", a1.v, (int)(&(*pp)[len] - p), p);
		p = *pp;
		a1.p = a;
		{double d;
		ca->ss++;
		d = args(ca, &p, t.s - p, pv, &a1);
		ca->ss--;
		return d;
		}
	}

}

/* postfix-expression */
static double post(calc_t *ca, const char **pp, size_t len){
	token t;
	double o;
	const char *pend = *pp, *s = &pend[len];
	fprintf(stderr, "[%d]enter post: \"%.*s\" (%lu)\n", ca->ss, len, *pp, len);
	if(!preexp(ca, &s, pend, &t)) lasterrpos = pend, longjmp(ca->env, CALC_NOP);
	{
		token t1;
		const char *s1 = &pend[len];
		pretok(&s1, pend, &t1);
		fprintf(stderr, "post: t.s = \"%.*s\" (%lu), t1.s = \"%.*s\" (%lu)\n", t.l, t.s, t.l, t1.l, t1.s, t1.l);
		if(t1.l == t.l && t1.s == t.s){ /* if preexp and pretok tokenizes the same way, it's an atom(not enclosed by parentheses) */
			if(ca->vl && ca->vl->l){
				struct varlist *vl;
				struct var *pv, *pvend;
				/* search through all given variable identifiers */
				for(vl = ca->vl; vl; vl = vl->next) for(pv = vl->l, pvend = &vl->l[vl->c]; pvend != pv; pv++) if(strlen(pv->name) == t1.l && !strncmp(pv->name, t1.s, t1.l)) switch(pv->type){
					case CALC_D: return pv->value.d;
					case CALC_F: 
					default: lasterrpos = t1.s; longjmp(ca->env, CALC_UVT);
				}
			}
			{
				char *p;
				o = strtod(t.s, &p);
				/* the strtod leaves p intact if it failed to parse the given string. sort of missing operand? */
//				if(t.s == p) lasterrpos = p, longjmp(ca->env, CALC_FC);
				if(t.s == p){
					int ret;
					double val;
					if(!ca->undname)
						lasterrpos = p, longjmp(ca->env, CALC_FC);
					if(ret = ca->undname(t1.s, t1.l, &val))
						return val;
					lasterrpos = t1.s;
					longjmp(ca->env, CALC_FC);
				}
			}
		}
		else if(t1.l == 1 && *t1.s == ')' && *pend != '('){
			token t2, t3;
			const char *s2 = pend;
			fprintf(stderr, "post: fcall test: ");
			if(nextok(&s2, &t3) && (t3.l != 1 || *t3.s != '(') && (fprintf(stderr, "\"%s\" ", s2), nextok(&s2, &t2)) && t2.l == 1 && *t2.s == '('){
				/* it seems much like a function call. */
				struct varlist *vl;
				struct var *pv, *pvend;
				fprintf(stderr, "\"%.*s\" (%lu) ", t3.l, t3.s, t3.l);
				/* check the identifier */
				for(vl = ca->vl; vl; vl = vl->next) for(pv = vl->l, pvend = &vl->l[vl->c]; pvend != pv; pv++) if(strlen(pv->name) == t3.l && !strncmp(pv->name, t3.s, t3.l)) switch(pv->type){
					case CALC_D: lasterrpos = t1.s; longjmp(ca->env, CALC_IT);
					case CALC_F:
					{
						const char *p = t2.s;
						fprintf(stderr, "function %s (%d)\n", pv->name, pv->value.f.args);
						if(!nexexp(ca, &p, &t2)) lasterrpos = p, longjmp(ca->env, CALC_NOP);
						return args(ca, &t2.s, t2.l, pv, NULL);
					}
					default:
						fprintf(stderr, "name = %s, type = %d\n", pv->name, pv->type);
						lasterrpos = t1.s; longjmp(ca->env, CALC_UVT);
				}
				/* if it has form of function call but no definition, raise an exception */
				lasterrpos = t3.s - t3.l + 1; longjmp(ca->env, CALC_ND);
			}
			fprintf(stderr, "no luck on %s\n", s2);
			goto addexp;
		}
		else{ /* otherwise, enter additive-expression between the parenthesis */
addexp:
			s1 = t.s;
			ca->ss++;
			o = add(ca, &s1, t.l);
			ca->ss--;
		}
		fprintf(stderr, "post: operand: %g = %s(\"%.*s\")\n",
			o, t1.l == t.l ? "strtod" : "add", t.l, t.s);
	}
	return o;
}

/* read backwards */
static double mult(calc_t *ca, const char **pp, size_t len){
	token t;
	const char *pend = *pp, *s = &pend[len];
	double o1, o2;
	fprintf(stderr, "[%d]enter mult: \"%.*s\" (%lu)\n", ca->ss, len, *pp, len);

	/* find the first occasion of * or / operator */
	while(preexp(ca, &s, pend, &t) && !(t.l == 1 && (*t.s == '*' || *t.s == '/')));
	if(!s) return post(ca, pp, len); /* if not, assume it's postfix-expression */
	fprintf(stderr, "mult: operator: \"%.*s\" (%lu)\n", t.l, t.s, t.l);

	/* if we do find a * or /, calculate the first (possibly compound) operand as postfix */
	{
		const char *p = t.s+1;
		ca->ss++;
		o2 = post(ca, &p, &(*pp)[len] - p); /* tell length till just before the * or / */
		ca->ss--;
		fprintf(stderr, "mult: operand 2: %g = post(\"%.*s\")\n", o2, (int)(&(*pp)[len] - p), p);
	}

	/* retrieve operator is already done */
/*	if(!pretok(&s, pend, &t)){
		fprintf(stderr, "mult: no operator: return operand 2\n");
		*pp = s;
		return o2;
	}
	fprintf(stderr, "mult: operator: \"%.*s\" (%lu)\n", t.l, t.s, t.l);*/
	if(t.l == 1 && *t.s == '*'){
		const char *po = pend;
		ca->ss++;
		o1 = mult(ca, &pend, s - pend);
		ca->ss--;
		fprintf(stderr, "mult: operand 1: %g = mult(\"%.*s\")\n", o1, (int)(s - po), po);
		*pp = s;
		return o1 * o2;
	}
	else if(t.l == 1 && *t.s == '/'){
		const char *po = pend, *p;
		if(0 == o2) lasterrpos = t.s, longjmp(ca->env, CALC_DIV); /* zero divide is signaled */
#if 0
		if(pend == s) /* if dividend is omitted, assume it to be 1 */
#else /* make sure no token precedes the division operator */
		for(p = s; p != pend; p--) if(!isspace(p[-1])) break;
		if(pend == p)
#endif
			o1 = 1;
		else{
			ca->ss++;
			o1 = mult(ca, &pend, s - pend);
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

static double add(calc_t *ca, const char **pp, size_t len){
	token t;
	const char *pend = *pp, *s = &pend[len];
	double o1, o2;
	fprintf(stderr, "[%d]enter add: \"%.*s\" (%lu)\n", ca->ss, len, *pp, len);

	/* find the first occasion of + or - operator */
	while(preexp(ca, &s, pend, &t) && !(t.l == 1 && (*t.s == '+' || *t.s == '-')))
		fprintf(stderr, "add: \"%.*s\" (%lu)\n", t.l, t.s, t.l);
	if(!s) return mult(ca, pp, len); /* if not, assume it's multiplicative operation */
	fprintf(stderr, "add: operator: \"%.*s\" (%lu)\n", t.l, t.s, t.l);

	/* if we do find a + or -, calculate the first (possibly compound) operand as multiplication */
	{
		const char *p = t.s+1;
		ca->ss++;
		o2 = mult(ca, &p, &(*pp)[len] - p); /* tell length till just before the + or - */
		ca->ss--;
		fprintf(stderr, "add: operand 2: %g = mult(\"%.*s\")\n", o2, (int)(&(*pp)[len] - p), p);
	}

	{
		const char *p = t.s; /* save pointer to end of operand 1 i.e. pointer to the operator */
		/* if operator 1 is lacking, it's negation unary operator */
		if(*p == '-' && !pretok(&s, pend, &t)) return -o2;

		/* parse the second operand, this time it may include additive operators */
		ca->ss++;
		o1 = add(ca, pp, p - pend);
		ca->ss--;
		fprintf(stderr, "add: operand 1: %g = add(\"%.*s\")\n", o1, (int)(p - pend), pend);
		*pp = s;
		return *p == '+' ? o1 + o2 : o1 - o2;
	}
}


double calc3(const char *const *s, const struct varlist *v, int undname(const char *, int, double*)){
	double ret = 1.;
	calc_t ca = {0};
	ca.ss = 0; /* top level */
	ca.vl = (struct varlist*)v; /* discard the const for convenience, but never change! */
	ca.undname = undname;

/*	fprintf(stderr, "calc: parse error: ");*/
	switch(setjmp(ca.env)){
		case 0: break;
		case CALC_UOP: cerrno = CALC_UOP; return 0;
		case CALC_NOP: cerrno = CALC_NOP; return 0;
		case CALC_FC: cerrno = CALC_FC; return 0;
		case CALC_DIV: cerrno = CALC_DIV; return 0;
		case CALC_OP: cerrno = CALC_OP; return 0;
		case CALC_CP: cerrno = CALC_CP; return 0;
		case CALC_ND: cerrno = CALC_ND; return 0;
		case CALC_UVT: cerrno = CALC_UVT; return 0;
		case CALC_IT: cerrno = CALC_IT; return 0;
		case CALC_ARG: cerrno = CALC_ARG; return 0;
		default: cerrno = CALC_UNK; return 0;
	}

	ret = add(&ca, s, strlen(*s));

#ifndef NDEBUG
	fprintf(stderr, "calc3: tok = %d, exp = %d \n", ca.tok, ca.exp);
#endif

	return ret;
}

