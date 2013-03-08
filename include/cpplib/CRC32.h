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
		uint32_t operator()(const uint8_t *buf, size_t len){
			return ::clib_crc32_direct(buf, len);
		}

		/// \brief The secret spice that initializes necessary table only if used.
		CRC32Gen(){
			clib_crc32_make_crc_table();
		}
	};

	static CRC32Gen crc32;

}

#endif
