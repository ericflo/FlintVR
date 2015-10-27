#ifndef CORE_VECTOR4F_H
#define CORE_VECTOR4F_H

#include "BaseInclude.h"

void SetupCoreVector4f(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreVector4f(JSContext* cx, OVR::Vector4f* vector4f);
OVR::Vector4f* GetVector4f(JS::HandleObject obj);

#endif
