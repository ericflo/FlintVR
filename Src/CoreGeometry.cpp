#include "CoreGeometry.h"
#include "CoreVector3f.h"


CoreGeometry::CoreGeometry(OVR::VertexAttribs* vert, OVR::Array<OVR::TriangleIndex> idc) {
  geometry = new OVR::GlGeometry(*vert, idc);
  vertices = vert;
  indices = idc;
}

CoreGeometry::~CoreGeometry(void) {
  geometry->Free();
  delete geometry;
  delete vertices;
}

static JSClass coreGeometryClass = {
  "Geometry",             /* name */
  JSCLASS_HAS_PRIVATE,    /* flags */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  CoreGeometry_finalize,
  NULL,
  NULL,
  NULL,
  CoreGeometry_trace
};

//VRJS_GETSET(CoreGeometry, vertices)
//VRJS_GETSET(CoreGeometry, indices)

static JSPropertySpec CoreGeometry_props[] = {
//  VRJS_PROP(CoreGeometry, vertices),
//  VRJS_PROP(CoreGeometry, indices),
  JS_PS_END
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

  // Vertices
  JS::RootedValue vertices(cx);
  if (!JS_GetProperty(cx, opts, "vertices", &vertices) || vertices.isNullOrUndefined() || !vertices.isObject()) {
    JS_ReportError(cx, "Could not parse vertices");
    return false;
  }
  bool isArray;
  if (!JS_IsArrayObject(cx, vertices, &isArray) || !isArray) {
    JS_ReportError(cx, "Expected vertices to be an array object");
    return false;
  }

  // Indices
  JS::RootedValue indices(cx);
  if (!JS_GetProperty(cx, opts, "indices", &indices) || indices.isNullOrUndefined() || !indices.isObject()) {
    JS_ReportError(cx, "Could not parse indices");
    return false;
  }
  if (!JS_IsArrayObject(cx, indices, &isArray) || !isArray) {
    JS_ReportError(cx, "Expected indices to be an array object");
    return false;
  }

  OVR::VertexAttribs* vert = ParseVertexAttribs(cx, vertices);
  if (vert == NULL) {
    // The parser reports the error
    return false;
  }

  // Build the indices array
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
  JS::RootedValue index(cx);
  for (size_t i = 0; i < indexLength; ++i) {
    if (!JS_GetElement(cx, indicesObj, i, &index)) {
      JS_ReportError(cx, "Couldn't get index at slot %d", i);
      return false;
    }
    idc[i] = index.toInt32();
  }

  // Now we create our geometry
  JS::RootedObject self(cx, NewCoreGeometry(cx, new CoreGeometry(vert, idc)));

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreGeometry_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreGeometry* geometry = (CoreGeometry*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete geometry;
}

void CoreGeometry_trace(JSTracer *tracer, JSObject *obj) {
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Tracing geometry\n");
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Finished tracing geometry\n");
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

  // Now attach our constants for VERTEX_*
  if (!JS_SetProperty(cx, *core, "VERTEX_POSITION", JS::RootedValue(cx, JS::NumberValue(VERTEX_POSITION)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_POSITION constant\n");
    return;
  }
  if (!JS_SetProperty(cx, *core, "VERTEX_NORMAL", JS::RootedValue(cx, JS::NumberValue(VERTEX_NORMAL)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_NORMAL constant\n");
    return;
  }
  if (!JS_SetProperty(cx, *core, "VERTEX_TANGENT", JS::RootedValue(cx, JS::NumberValue(VERTEX_TANGENT)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_TANGENT constant\n");
    return;
  }
  if (!JS_SetProperty(cx, *core, "VERTEX_BINORMAL", JS::RootedValue(cx, JS::NumberValue(VERTEX_BINORMAL)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_BINORMAL constant\n");
    return;
  }
  if (!JS_SetProperty(cx, *core, "VERTEX_COLOR", JS::RootedValue(cx, JS::NumberValue(VERTEX_COLOR)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_COLOR constant\n");
    return;
  }
  if (!JS_SetProperty(cx, *core, "VERTEX_UV0", JS::RootedValue(cx, JS::NumberValue(VERTEX_UV0)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_UV0 constant\n");
    return;
  }
  if (!JS_SetProperty(cx, *core, "VERTEX_UV1", JS::RootedValue(cx, JS::NumberValue(VERTEX_UV1)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_UV1 constant\n");
    return;
  }
  if (!JS_SetProperty(cx, *core, "VERTEX_JOINT_INDICES", JS::RootedValue(cx, JS::NumberValue(VERTEX_JOINT_INDICES)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_JOINT_INDICES constant\n");
    return;
  }
  if (!JS_SetProperty(cx, *core, "VERTEX_JOINT_WEIGHTS", JS::RootedValue(cx, JS::NumberValue(VERTEX_JOINT_WEIGHTS)))) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.VERTEX_JOINT_WEIGHTS constant\n");
    return;
  }
}

JSObject* NewCoreGeometry(JSContext* cx, CoreGeometry* geometry) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreGeometryClass));
  if (!JS_DefineProperties(cx, self, CoreGeometry_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on geometry\n");
  }
  JS_SetPrivate(self, (void *)geometry);
  return self;
}

CoreGeometry* GetCoreGeometry(JS::HandleObject obj) {
  CoreGeometry* geometry = (CoreGeometry*)JS_GetPrivate(obj);
  return geometry;
}