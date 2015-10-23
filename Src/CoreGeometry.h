#ifndef CORE_GEOMETRY_H
#define CORE_GEOMETRY_H

#include "BaseInclude.h"
#include "ParseVertexAttribs.h"

class CoreGeometry {
public:
  OVR::GlGeometry* geometry;
  OVR::VertexAttribs* vertices;
  OVR::Array<OVR::TriangleIndex> indices;

  CoreGeometry(OVR::VertexAttribs* vert, OVR::Array<OVR::TriangleIndex> idc);
  ~CoreGeometry();
};

void SetupCoreGeometry(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core);
//JSObject* NewCoreGeometry(JSContext *cx, OVR::GlGeometry *geometry);
CoreGeometry* GetCoreGeometry(JS::HandleObject obj);

#endif
