#ifndef CORE_PROGRAM_H
#define CORE_PROGRAM_H

#include "BaseInclude.h"

void SetupCoreProgram(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
OVR::GlProgram* GetProgram(JS::HandleObject obj);

#endif
