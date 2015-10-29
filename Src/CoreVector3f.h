#ifndef CORE_VECTOR3F_H
#define CORE_VECTOR3F_H

#include "BaseInclude.h"

void SetupCoreVector3f(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreVector3f(JSContext* cx, OVR::Vector3f* vec);
OVR::Vector3f* GetVector3f(JS::HandleObject obj);
void CoreVector3f_finalize(JSFreeOp *fop, JSObject *obj);
const JSClass* CoreVector3f_class();

bool CoreVector3f_add(JSContext* cx, unsigned argc, JS::Value *vp);
bool CoreVector3f_multiply(JSContext* cx, unsigned argc, JS::Value *vp);

#endif
