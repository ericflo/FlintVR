#ifndef CORE_TEXTURE_H
#define CORE_TEXTURE_H

#include "BaseInclude.h"
#include "stb_image.h"

class CoreTexture {
public:
  JS::Heap<JS::Value> path;
  int width;
  int height;
  bool cube;
  OVR::GlTexture texture;

  CoreTexture(JSContext* cx, JS::HandleValue _path, int _width, int _height, bool _cube = false);
  ~CoreTexture();
  bool Rebuild(JSContext* cx);
private:
  bool RebuildCubemap(JSContext* cx, OVR::String pathStr);
  bool RebuildTexture(JSContext* cx, OVR::String pathStr);
};

void SetupCoreTexture(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreTexture(JSContext* cx, CoreTexture* tex);
CoreTexture* GetCoreTexture(JS::HandleObject obj);
void CoreTexture_finalize(JSFreeOp *fop, JSObject *obj);
void CoreTexture_trace(JSTracer *tracer, JSObject *obj);
const JSClass* CoreTexture_class();

#endif
