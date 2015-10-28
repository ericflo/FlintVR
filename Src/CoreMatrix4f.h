#ifndef CORE_MATRIX4F_H
#define CORE_MATRIX4F_H

#include "BaseInclude.h"
#include "CoreVector3f.h"

void SetupCoreMatrix4f(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreMatrix4f(JSContext* cx, OVR::Matrix4f* matrix4f);
OVR::Matrix4f* GetMatrix4f(JS::HandleObject obj);

bool CoreMatrix4f_setTranslation(JSContext* cx, unsigned argc, JS::Value *vp);
bool CoreMatrix4f_multiply(JSContext* cx, unsigned argc, JS::Value *vp);
bool CoreMatrix4f_rotationX(JSContext* cx, unsigned argc, JS::Value *vp);
bool CoreMatrix4f_rotationY(JSContext* cx, unsigned argc, JS::Value *vp);
bool CoreMatrix4f_rotationZ(JSContext* cx, unsigned argc, JS::Value *vp);
bool CoreMatrix4f_transform(JSContext* cx, unsigned argc, JS::Value *vp);

//

void CoreMatrix4f_finalize(JSFreeOp *fop, JSObject *obj);
const JSClass* CoreMatrix4f_class();

#endif
