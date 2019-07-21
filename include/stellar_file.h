#ifndef STELLAR_FILE_H
#define STELLAR_FILE_H
/** \file
 * \brief Functions to load Stellar Structure Definition file. */

#include "CoordSys.h"
#include <squirrel.h>
#include <exception>
#include <stdexcept> // runtime_error
#include <deque>
#include <unordered_map>
#include <functional>

class StellarStructureScanner;

typedef std::deque<gltestp::dstring> TokenList;

typedef long linenum_t; ///< I think 2 billions would be enough.

// Due to a bug in gcc, template specialization for a template that is in a
// namespace fails to compile.  This should be allowed, according to the standard.
// (https://stackoverflow.com/questions/25594644/warning-specialization-of-template-in-different-namespace)
// For the time being, adding `namespace std` around the specialization works around the issue.
#ifdef  __GNUC__
namespace std{
#endif

/// Template instantiation for unordered map key hash of gltestp::dstring
#ifdef  __GNUC__
template<> struct hash<gltestp::dstring>
#else
template<> struct std::hash<gltestp::dstring>
#endif
{
	size_t operator()(const gltestp::dstring &s)const{
		return s.hash();
	}
};

#ifdef  __GNUC__
}
#endif

/// Context object in the process of interpreting a stellar file.
struct StellarContext{
	typedef std::unordered_map<gltestp::dstring, std::function<void (StellarContext &, TokenList &)>> CommandMap;
	typedef std::unordered_map<gltestp::dstring, gltestp::dstring> UpvarMap;

	Game *game;
	const char *fname;
	CoordSys *root;
	CoordSys *cs; ///< Active CoordSys, top of the stack
	std::istream *fp;
	gltestp::dstring buf;
	linenum_t line;
	linenum_t lastLine;
	HSQOBJECT vars; ///< Squirrel table to hold locally-defined variables
	HSQUIRRELVM v;
	StellarStructureScanner *scanner;
	CommandMap *commands;
	UpvarMap *upvars;
	gltestp::dstring retval; ///< A variable to hold returned values from nested function calls

	/// @brief Parse single command
	int parseCommand(TokenList &argv);

	/// @brief Parse a block of commands without growing stack
	int parseBlock();

	/// @brief Parse and interpret a string as a list of commands
	/// @param cs If non-NULL, grow the stack frame for the new CoordSys, otherwise the string is executed
	///           in the same frame as the caller's.
	/// @param linenum Starting line number in the file.
	/// @param newCommandMap Temporarily override command map for parsing given commands. Can be nullptr
	//            to not to override.
	int parseString(const char *commands, CoordSys *cs, linenum_t linenum = 0, CommandMap *newCommandMap = nullptr);

	/// @brief Parse a CoordSys, almost equivalent to parseBlock with growing stack
	int parseCoordSys(CoordSys *cs);

	/// @brief Parse single file, could be recursively called
	static int parseFile(const char *fname, CoordSys *root, StellarContext *prev_sc);

	static void scmd_define(StellarContext &sc, TokenList &argv);
	static void scmd_new(StellarContext &sc, TokenList &argv);
	static void scmd_coordsys(StellarContext &sc, TokenList &argv);
};

/// \brief Base type for any errors that could happen in stellar file interpretation.
struct StellarError : std::runtime_error{
	StellarError(const char *s) : std::runtime_error(s){}
};

namespace stellar_util{

double sqcalcd(StellarContext &sc, const char *str, const SQChar *context);
int sqcalci(StellarContext &sc, const char *str, const SQChar *context);
bool sqcalcb(StellarContext &sc, const char *str, const SQChar *context);

}

#endif
