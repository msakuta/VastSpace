#ifndef STELLAR_FILE_H
#define STELLAR_FILE_H
/** \file
 * \brief Functions to load Stellar Structure Definition file. */

#include "CoordSys.h"
#include <squirrel.h>
#include <exception>


class StellarStructureScanner;


/// Context object in the process of interpreting a stellar file.
struct StellarContext{
	const char *fname;
	CoordSys *root;
	FILE *fp;
	cpplib::dstring buf;
	long line;
	HSQOBJECT vars; ///< Squirrel table to hold locally-defined variables
	HSQUIRRELVM v;
	StellarStructureScanner *scanner;
};

/// \brief Base type for any errors that could happen in stellar file interpretation.
struct StellarError : std::exception{
	StellarError(const char *s) : std::exception(s){}
};

#endif
