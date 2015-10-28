#ifndef CORE_VECTOR3F_H
#define CORE_VECTOR3F_H

#include "BaseInclude.h"

void SetupCoreVector3f(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreVector3f(JSContext* cx, OVR::Vector3f* vector3f);
OVR::Vector3f* GetVector3f(JS::HandleObject obj);

bool CoreVector3f_add(JSContext* cx, unsigned argc, JS::Value *vp);
bool CoreVector3f_multiply(JSContext* cx, unsigned argc, JS::Value *vp);

//

bool CoreVector3f_getProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp);
bool CoreVector3f_setProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp, JS::ObjectOpResult& result);
void CoreVector3f_finalize(JSFreeOp *fop, JSObject *obj);
const JSClass* CoreVector3f_class();

#endif
