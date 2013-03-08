/** \file
 * \brief Definition of very simple CRC32 implementation APIs.
 */
#ifndef CLIB_CRC32_H
#define CLIB_CRC32_H
#include <stdint.h>
#include <stddef.h> /* size_t */

/** \brief Creates a table for fast CRC32 calculation.
 *
 * Necessary to be invoked before crc32(). */
void make_crc_table(void);

/** \brief Returns a CRC32 calculated value. */
uint32_t crc32(const uint8_t *buf, size_t len);

/** \brief Gets internal crc table, for ebugging and development purposes.
 * \returns Constant pointer to the table. */
const uint32_t *get_crc_table();

#endif
