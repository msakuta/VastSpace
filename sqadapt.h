#ifndef SQADAPT_H
#define SQADAPT_H
// Squirrel library adapter for gltestplus project.
#include <stddef.h>
#include <squirrel.h>

void sqa_init();
void sqa_exit();

SQInteger register_global_func(HSQUIRRELVM v,SQFUNCTION f,const SQChar *fname);

extern HSQUIRRELVM g_sqvm;

#endif
