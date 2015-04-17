#ifndef STELLAR_FILE_H
#define STELLAR_FILE_H
/** \file
 * \brief Functions to load Stellar Structure Definition file. */

#include "CoordSys.h"
#include <squirrel.h>
#include <exception>
#include <deque>
#include <unordered_map>


class StellarStructureScanner;

typedef std::deque<gltestp::dstring> TokenList;

typedef long linenum_t; ///< I think 2 billions would be enough.


/// Context object in the process of interpreting a stellar file.
struct StellarContext{
	typedef std::unordered_map<gltestp::dstring, void (*)(StellarContext &, TokenList &), size_t (*)(const gltestp::dstring &)> CommandMap;
	Game *game;
	const char *fname;
	CoordSys *root;
	std::istream *fp;
	gltestp::dstring buf;
	long line;
	HSQOBJECT vars; ///< Squirrel table to hold locally-defined variables
	HSQUIRRELVM v;
	StellarStructureScanner *scanner;
	CommandMap *commands;

	/// @brief Parse single command
	int parseCommand(TokenList &argv, CoordSys *cs);

	/// @brief Parse a block of commands without growing stack
	int parseBlock(CoordSys *cs);

	/// @brief Parse and interpret a string as a list of commands
	/// @param enterCoordSys Whether to grow the stack frame for the new CoordSys.
	///                      Callers of this function varies whether they need it.
	/// @param linenum Starting line number in the file.
	int parseString(const char *commands, CoordSys *cs, bool enterCoordSys, linenum_t linenum = 0);

	/// @brief Parse a CoordSys, almost equivalent to parseBlock with growing stack
	int parseCoordSys(CoordSys *cs);

	/// @brief Parse single file, could be recursively called
	static int parseFile(const char *fname, CoordSys *root, StellarContext *prev_sc);

	static void scmd_define(StellarContext &sc, TokenList &argv);
};

/// \brief Base type for any errors that could happen in stellar file interpretation.
struct StellarError : std::exception{
	StellarError(const char *s) : std::exception(s){}
};

namespace stellar_util{

double sqcalcd(StellarContext &sc, const char *str, const SQChar *context);
int sqcalci(StellarContext &sc, const char *str, const SQChar *context);
bool sqcalcb(StellarContext &sc, const char *str, const SQChar *context);

}

#endif
