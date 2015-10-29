#ifndef CORE_VECTOR4F_H
#define CORE_VECTOR4F_H

#include "BaseInclude.h"

void SetupCoreVector4f(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreVector4f(JSContext* cx, OVR::Vector4f* vec);
OVR::Vector4f* GetVector4f(JS::HandleObject obj);
void CoreVector4f_finalize(JSFreeOp *fop, JSObject *obj);
const JSClass* CoreVector4f_class();

#endif
