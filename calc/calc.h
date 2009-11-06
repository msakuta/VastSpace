/*
 * calc.h
 * simple expression calculator
 */
#ifndef MASAHIRO_CALC_H
#define MASAHIRO_CALC_H

/* Double variable recording entry. You can construct an array out of this
  structure and set up dvarlist structure to define a set of variables to
  pass to the parser. */
struct var{
	char *name; /* Defines the variable identifier */
	enum calc_type{
		CALC_D, /* double */
		CALC_F, /* pointer to a function */
		NUM_CALC_TYPE
	} type; /* Determines the type of the variable */
	union{
		double d; /* double value */
		struct calc_function{
			double (*p)(); /* the pointer, note that it undefines the arguments. */
			int args; /* count of arguments (and types in future) */
		} f; /* function pointer value */
	} value;
};

struct varlist{
	unsigned c;
	struct var *l;
	struct varlist *next;
};

/* Parses the given null-terminated string into single double.
  The string can have additive (+, -) or multiplicative (*, /) operators.
  The function handles operation order correctly. */
extern double calc(const char *const *s);
extern double calc2(const char *const *s);

/* calc level 3 doesn't change the given variables; they are only referred and
  not to be assigned. undname function is called when undefined name is invoked.
  return 0 if it's assumed error, otherwise return 1 and assign its value to *value. */
extern double calc3(const char *const *s, const struct varlist *variables, int undname(const char *name, int namelen, double *value));

/* If something went wrong on parsing the string, the calc function returns 0.0
  and next invocation of calc_get_last_errno function returns a nonzero value.
  The returning value represents the type of error occurred as following enum
  defines. The calc_err_string simply maps the returning code of the first
  function to explaining string in ASCII. */
enum calc_errno{
	CALC_0,   /* No error occurred */
	CALC_UOP, /* Unexpected Operator, couldn't recognize an operator, consider using higher version */
	CALC_NOP, /* Missing Operand, an operand isn't present where it ought to be */
	CALC_FC,  /* Floating-point Constant value's representation could not be understood */
	CALC_DIV, /* Division by 0 */
	CALC_OP,  /* No match Opening '(' Parenthesis */
	CALC_CP,  /* No match Closing ')' parenthesis */
	CALC_ND,  /* Not Defined Identifier */
	CALC_UVT, /* >= 3 Unknown Variable Type */
	CALC_IT,  /* >= 3 Incompatible Type in the context; conversion couldn't be achieved */
	CALC_ARG, /* >= 3 Number of arguments differs to definition */
	CALC_UNK, /* Unknown error */
	NUM_CALC_ERRNO
};
extern enum calc_errno calc_get_last_errno(void);
extern enum calc_errno calc_set_errno(enum calc_errno);
extern const char *calc_err_string(enum calc_errno err);

/* If something went wrong on parsing the string, the calc function returns 0.0
  and next invocation of calcGetLastError function returns a valid pointer to a
  string explaining what was wrong. If it returns NULL, calc's return value is
  valid calculated value. */
/*extern const char *calcGetLastError();*/
#define calcGetLastError() calc_err_string(calc_get_last_errno())

/* The calc_get_last_err_pos returns a pointer to the character where the parser found a problem. */
extern const char *calc_get_last_err_pos();

extern const struct varlist *calc_mathvars();

#endif
