#ifndef MQOADAPT_H
#define MQOADAPT_H
/** \file
 * \brief This file is to adapt mqo.h to VastSpaceEngine.
 *
 * The module mqo.cpp should be independent from drawing routines like OpenGL,
 * because it could be used in dedicated server or other tools too.
 * To make the module more encapsulated, this file connects it with the application.
 */
#include "mqo.h"

EXPORT Model *LoadMQOModel(const char *fname, double scale = 1.);

#endif

