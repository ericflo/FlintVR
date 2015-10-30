#ifndef CORE_TEXTURE_H
#define CORE_TEXTURE_H

#include "BaseInclude.h"

class CoreTexture {
public:
  JS::Heap<JS::Value> path;
  int width;
  int height;
  OVR::GlTexture texture;

  CoreTexture(JSContext* cx, JS::HandleValue path, int width, int height);
  ~CoreTexture();
  bool Rebuild(JSContext* cx);
};

void SetupCoreTexture(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreTexture(JSContext* cx, CoreTexture* tex);
CoreTexture* GetCoreTexture(JS::HandleObject obj);
void CoreTexture_finalize(JSFreeOp *fop, JSObject *obj);
void CoreTexture_trace(JSTracer *tracer, JSObject *obj);
const JSClass* CoreTexture_class();

#endif
