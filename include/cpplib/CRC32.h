/** \file
 * \brief Definition of class packaged CRC32 calculator, without overhead of initialization on first use.
 */
#ifndef CPPLIB_CRC32_H
#define CPPLIB_CRC32_H
extern "C"{
#include "clib/crc32.h"
}

namespace cpplib{

	/// \brief Dummy class that initializes at program startup only if ever used.
	class CRC32Gen{
	public:
		/// \brief Returns CRC32 value of given buffer.
		///
		/// The function needs crc_table to be initialized in clib, but we do not
		/// want initialization on first use idiom for its overhead.
		/// The CRC32Gen class object initializes the table in constructor,
		/// but it's possibly not initialized in some implementations if it's
		/// not ever used, so we make sure it's used by overloading function call
		/// operator of the object to use it like a functionoid.
		uint32_t operator()(const void *buf, size_t len, uint32_t crc32 = 0){
			return ::clib_crc32_direct(buf, len, crc32);
		}

		/// \brief The secret spice that initializes necessary table only if used.
		CRC32Gen(){
			clib_crc32_make_crc_table();
		}
	};

	/// \brief This static object replicates in every source that include this file;
	/// you invoke the constructor only if you ever use it.
	static CRC32Gen crc32;

}

#endif
