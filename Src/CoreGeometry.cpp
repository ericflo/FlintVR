#include "CoreGeometry.h"
#include "CoreVector3f.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

CoreGeometry::CoreGeometry(OVR::VertexAttribs* vert, OVR::Array<OVR::TriangleIndex> idc) {
  geometry = new OVR::GlGeometry(*vert, idc);
  vertices = vert;
  indices = idc;
}

CoreGeometry::~CoreGeometry(void) {
  delete geometry;
  delete vertices;
}

static JSClass coreGeometryClass = {
  "Geometry",             /* name */
  JSCLASS_HAS_PRIVATE    /* flags */
};

bool CoreGeometry_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }

  // Check to make sure the arg is an object
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Invalid argument type, must be an object");
    return false;
  }
  JS::RootedObject opts(cx, &args[0].toObject());

  // Get a reference to the VertexAttribs from the opts object
  JS::RootedValue vertices(cx);
  if (!JS_GetProperty(cx, opts, "vertices", &vertices)) {
    JS_ReportError(cx, "Could not find 'vertices' option");
    return false;
  }

  // TODO: Validate object type before casting
  OVR::VertexAttribs* vert = ParseVertexAttribs(cx, vertices);
  if (vert == NULL) {
    // The parser reports the error
    return false;
  }

  // Build up an array of triangle indices from the opts object
  JS::RootedValue indices(cx);
  if (!JS_GetProperty(cx, opts, "indices", &indices)) {
    JS_ReportError(cx, "Could not find 'indices' option");
    return false;
  }
  bool isArray;
  if (!JS_IsArrayObject(cx, indices, &isArray) || !isArray) {
    JS_ReportError(cx, "Expected indices to be an array object");
    return false;
  }
  JS::RootedObject indicesObj(cx, &indices.toObject());
  // Get the length of the index array
  uint32_t indexLength;
  if (!JS_GetArrayLength(cx, indicesObj, &indexLength)) {
    JS_ReportError(cx, "Couldn't get indices array length");
    return false;
  }
  // Fill a TriangleIndex array with the ints
  OVR::Array<OVR::TriangleIndex> idc;
  idc.Resize(indexLength);
  JS::RootedValue tmp(cx);
  for (size_t i = 0; i < indexLength; ++i) {
    if (!JS_GetElement(cx, indicesObj, i, &tmp)) {
      JS_ReportError(cx, "Couldn't get index at slot %d", i);
      return false;
    }
    idc[i] = tmp.toInt32();
  }

  // Go ahead and create our self object
  JS::RootedObject self(cx, JS_NewObject(cx, &coreGeometryClass));
  if (!JS_SetProperty(cx, self, "vertices", vertices)) {
    JS_ReportError(cx, "Could not set vertices property on geometry object");
    return false;
  }
  if (!JS_SetProperty(cx, self, "indices", indices)) {
    JS_ReportError(cx, "Could not set indices property on geometry object");
    return false;
  }

  // Now we create our geometry
  CoreGeometry* geometry = new CoreGeometry(vert, idc);

  // Store the geometry in the private data area
  JS_SetPrivate(self, (void *)geometry);

  if (!JS_FreezeObject(cx, self)) {
    JS_ReportError(cx, "Could not freeze geometry object");
    return false;
  }

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreGeometry_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreGeometry* geometry = (CoreGeometry*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete geometry;
}

CoreGeometry* GetCoreGeometry(JS::HandleObject obj) {
  CoreGeometry* geometry = (CoreGeometry*)JS_GetPrivate(obj);
  return geometry;
}

void SetupCoreGeometry(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core) {
  coreGeometryClass.finalize = CoreGeometry_finalize;
  JSObject *obj = JS_InitClass(
      cx,
      *core,
      *global,
      &coreGeometryClass,
      CoreGeometry_constructor,
      1,
      nullptr, /* Properties */
      nullptr, /* Methods */
      nullptr, /* Static Props */
      nullptr  /* Static Methods */);
  if (!obj) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Geometry class\n");
    return;
  }

  // Now attach our constants for POSITION and COLOR
  if (!JS_SetProperty(cx, *core, "VERTEX_POSITION", JS::RootedValue(cx, JS::NumberValue(0)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_POSITION constant\n");
    return;
  }
  if (!JS_SetProperty(cx, *core, "VERTEX_COLOR", JS::RootedValue(cx, JS::NumberValue(1)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_COLOR constant\n");
    return;
  }
}
