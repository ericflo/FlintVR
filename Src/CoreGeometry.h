#ifndef CORE_GEOMETRY_H
#define CORE_GEOMETRY_H

#include "BaseInclude.h"
#include "ParseVertexAttribs.h"

typedef struct CoreGeometry {
  OVR::GlGeometry* geometry;
  OVR::VertexAttribs* vertices;
  OVR::Array<OVR::TriangleIndex> indices;
} CoreGeometry;

void SetupCoreGeometry(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core);
//JSObject* NewCoreGeometry(JSContext *cx, OVR::GlGeometry *geometry);
CoreGeometry* GetGeometry(JS::HandleObject obj);

#endif
