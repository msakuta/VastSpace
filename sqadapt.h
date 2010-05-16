#ifndef SQADAPT_H
#define SQADAPT_H
// Squirrel library adapter for gltestplus project.
#include <stddef.h>
#include <squirrel.h>

namespace sqa{

void sqa_init();
void sqa_anim0();
void sqa_exit();

SQInteger register_global_func(HSQUIRRELVM v,SQFUNCTION f,const SQChar *fname);

extern HSQUIRRELVM g_sqvm;

}

using namespace sqa;

#endif
