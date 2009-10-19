/* calc0.c common things among all levels */
#include "calc_p.h"
#include <stddef.h>

#include "tokker.h" /* token char type definition table, automatically generated */

const char *lasterrpos = NULL;
enum calc_errno cerrno = CALC_0;
enum calc_errno calc_get_last_errno(){ return cerrno; }
enum calc_errno calc_set_errno(enum calc_errno e){ enum calc_errno ret = cerrno; cerrno = e; return ret; }
const char *calc_err_string(enum calc_errno e){
	switch(e){
		case CALC_0: return "No Error";
		case CALC_UOP: return "Unexpected Operator";
		case CALC_NOP: return "Missing Operand";
		case CALC_FC: return "Floating-point Constant Format Error";
		case CALC_DIV: return "Division by 0";
		case CALC_OP: return "No match Opening '(' Parenthesis";
		case CALC_CP: return "No match Closing ')' Parenthesis";
		case CALC_ND: return "Not Defined Identifier";
		case CALC_UVT: return "Unknown Variable Type";
		case CALC_IT: return "Incompatible Type";
		case CALC_ARG: return "Number of arguments differs to definition";
		case CALC_UNK: return "Unknown Error";
		default: return "Undefined Error Code";
	}
}
const char *calc_get_last_err_pos(){ return lasterrpos; }
/*const char *calcGetLastError(){ return lasterr; }*/
