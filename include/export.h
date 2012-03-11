/** \file
 * \brief Defines macros that will bind executable and DLL.
 */
#ifndef EXPORT_H
#define EXPORT_H

#if defined DOXYGEN || !defined _WIN32
#define EXPORT
#else
#ifndef EXPORT
#ifdef DLL
#define EXPORT __declspec(dllimport)
#else
#define EXPORT __declspec(dllexport)
#endif
#endif
#endif


#endif
