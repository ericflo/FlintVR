#ifndef CORE_VECTOR2F_H
#define CORE_VECTOR2F_H

#include "BaseInclude.h"

void SetupCoreVector2f(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreVector2f(JSContext* cx, OVR::Vector2f* vec);
OVR::Vector2f* GetVector2f(JS::HandleObject obj);

//

void CoreVector2f_finalize(JSFreeOp *fop, JSObject *obj);
const JSClass* CoreVector2f_class();

#endif
