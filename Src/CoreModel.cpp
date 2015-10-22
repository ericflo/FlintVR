#include "CoreModel.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static int CURRENT_MODEL_ID = 1;

CoreModel::CoreModel(void) :
  id(CURRENT_MODEL_ID++),
  isHovered(false),
  isTouching(false),
  computedMatrix(NULL) {
}

bool CoreModel::HasFrameCallback() {
  return CallbackDefined(onFrameVal);
}

bool CoreModel::HasGazeCallback() {
  return CallbackDefined(onGazeHoverOverVal) || CallbackDefined(onGazeHoverOutVal);
}

bool CoreModel::HasGestureCallback() {
  return (
    CallbackDefined(onGestureTouchDownVal) ||
    CallbackDefined(onGestureTouchUpVal) ||
    CallbackDefined(onGestureTouchCancelVal)
  );
}

void CoreModel::ComputeMatrix(JSContext *cx) {
  OVR::Matrix4f mtx;

  OVR::Matrix4f* mat = matrix(cx);
  if (mat != NULL) {
    mtx = *mat;
  }

  OVR::Vector3f* rot = rotation(cx);
  if (rot != NULL) {
    mtx *= (
      OVR::Matrix4f::RotationX(rot->x) *
      OVR::Matrix4f::RotationY(rot->y) *
      OVR::Matrix4f::RotationZ(rot->z)
    );
  }

  OVR::Vector3f* scl = scale(cx);
  if (scl != NULL) {
    mtx *= OVR::Matrix4f::Scaling(*scl);
  }

  OVR::Vector3f* pos = position(cx);
  if (pos != NULL) {
    mtx.SetTranslation(*pos);
  }

  if (computedMatrix != NULL) {
    delete computedMatrix;
  }
  computedMatrix = new OVR::Matrix4f(mtx);
}

static JSClass coreModelClass = {
  "Model",                /* name */
  JSCLASS_HAS_PRIVATE    /* flags */
};

#define VRJS_GETSET(ClassName, name) \
  static bool ClassName##_get_##name(JSContext *cx, unsigned argc, JS::Value *vp) { \
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
    JS::RootedObject self(cx, &args.thisv().toObject()); \
    ClassName* model = Get##ClassName(self); \
    args.rval().set(model == NULL || model->name##Val.isNothing() ? JS::NullValue() : model->name##Val.ref()); \
    return true; \
  } \
  \
  static bool ClassName##_set_##name(JSContext *cx, unsigned argc, JS::Value *vp) { \
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
    if (!args[0].isObject()) { \
      JS_ReportError(cx, "Unexpected argument (expected ##name)"); \
      return false; \
    } \
    JS::RootedObject self(cx, &args.thisv().toObject()); \
    ClassName* model = Get##ClassName(self); \
    mozilla::Maybe<JS::PersistentRootedValue>* val = &model->name##Val; \
    val->reset(); \
    val->emplace(cx, *vp); \
    return true; \
  }

#define VRJS_PROP(ClassName, name) \
  JS_PSGS(#name, ClassName##_get_##name, ClassName##_set_##name, JSPROP_PERMANENT)

#define VRJS_MEMBER(ClassName, name, getter) \
  ClassName::name(JSContext *cx) { \
    if (name##Val.isNothing()) { \
      return NULL; \
    } \
    JS::RootedObject obj(cx, &name##Val.ref().toObject()); \
    return getter(obj); \
  }

VRJS_GETSET(CoreModel, geometry)
VRJS_GETSET(CoreModel, program)
VRJS_GETSET(CoreModel, matrix)
VRJS_GETSET(CoreModel, position)
VRJS_GETSET(CoreModel, rotation)
VRJS_GETSET(CoreModel, scale)
VRJS_GETSET(CoreModel, onFrame)
VRJS_GETSET(CoreModel, onGazeHoverOver)
VRJS_GETSET(CoreModel, onGazeHoverOut)
VRJS_GETSET(CoreModel, onGestureTouchDown)
VRJS_GETSET(CoreModel, onGestureTouchUp)
VRJS_GETSET(CoreModel, onGestureTouchCancel)

static JSPropertySpec CoreModel_props[] = {
  VRJS_PROP(CoreModel, geometry),
  VRJS_PROP(CoreModel, program),
  VRJS_PROP(CoreModel, matrix),
  VRJS_PROP(CoreModel, position),
  VRJS_PROP(CoreModel, rotation),
  VRJS_PROP(CoreModel, scale),
  VRJS_PROP(CoreModel, onFrame),
  VRJS_PROP(CoreModel, onGazeHoverOver),
  VRJS_PROP(CoreModel, onGazeHoverOut),
  VRJS_PROP(CoreModel, onGestureTouchDown),
  VRJS_PROP(CoreModel, onGestureTouchUp),
  VRJS_PROP(CoreModel, onGestureTouchCancel),
  JS_PS_END
};

CoreGeometry* VRJS_MEMBER(CoreModel, geometry, GetGeometry);
OVR::GlProgram* VRJS_MEMBER(CoreModel, program, GetProgram);
OVR::Matrix4f* VRJS_MEMBER(CoreModel, matrix, GetMatrix4f);
OVR::Vector3f* VRJS_MEMBER(CoreModel, position, GetVector3f);
OVR::Vector3f* VRJS_MEMBER(CoreModel, rotation, GetVector3f);
OVR::Vector3f* VRJS_MEMBER(CoreModel, scale, GetVector3f);

void SetMaybeValue(JSContext *cx, JS::MutableHandleValue vp, mozilla::Maybe<JS::PersistentRootedValue>& out) {
  out.reset();
  out.emplace(cx, vp);
}

void SetMaybeCallback(JSContext *cx, JS::RootedObject* opts, const char* name, JS::RootedObject* self, mozilla::Maybe<JS::PersistentRootedValue>& out) {
  JS::RootedValue callbackVal(cx);
  if (!JS_GetProperty(cx, *opts, name, &callbackVal) || callbackVal.isNullOrUndefined()) {
    callbackVal = JS::RootedValue(cx, JS::NullValue());
  }
  if (callbackVal.isNullOrUndefined()) {
    out.reset();
  }
  SetMaybeValue(cx, &callbackVal, out);
}

bool _ensureObject(JSContext *cx, JS::MutableHandleValue vp) {
  if (!vp.isObject()) {
    JS_ReportError(cx, "Unexpected argument (expected object)");
    return false;
  }
  return true;
}



bool CoreModel_constructor(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }

  // Check to make sure the arg is an object
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Model argument must be an object");
    return false;
  }
  JS::RootedObject opts(cx, &args[0].toObject());

  // Create our self object
  CoreModel* model = new CoreModel();
  JS::RootedObject self(cx, NewCoreModel(cx, model));

  // Make sure we have a reference to self
  JS::RootedValue selfVal(cx, JS::ObjectOrNullValue(self));
  SetMaybeValue(cx, &selfVal, model->selfVal);

  // Geometry
  JS::RootedValue geometry(cx);
  if (!JS_GetProperty(cx, opts, "geometry", &geometry)) {
    JS_ReportError(cx, "Could not find 'geometry' option");
    delete model;
    return false;
  }
  if (!_ensureObject(cx, &geometry)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &geometry, model->geometryVal);

  // Program
  JS::RootedValue program(cx);
  if (!JS_GetProperty(cx, opts, "program", &program)) {
    JS_ReportError(cx, "Could not find 'program' option");
    delete model;
    return false;
  }
  if (!_ensureObject(cx, &program)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &program, model->programVal);

  // Base transform matrix
  JS::RootedValue matrix(cx);
  if (!JS_GetProperty(cx, opts, "transform", &matrix) || matrix.isNullOrUndefined()) {
    matrix = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreMatrix4f(cx, new OVR::Matrix4f())));
  }
  if (!_ensureObject(cx, &matrix)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &matrix, model->matrixVal);

  // Position
  JS::RootedValue position(cx);
  if (!JS_GetProperty(cx, opts, "position", &position) || position.isNullOrUndefined()) {
    position = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f())));
  }
  if (!_ensureObject(cx, &position)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &position, model->positionVal);

  // Rotation
  JS::RootedValue rotation(cx);
  if (!JS_GetProperty(cx, opts, "rotation", &rotation) || rotation.isNullOrUndefined()) {
    rotation = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f())));
  }
  if (!_ensureObject(cx, &rotation)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &rotation, model->rotationVal);

  // Scale
  JS::RootedValue scale(cx);
  if (!JS_GetProperty(cx, opts, "scale", &scale) || scale.isNullOrUndefined()) {
    scale = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f())));
  }
  if (!_ensureObject(cx, &scale)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &scale, model->scaleVal);

  // Callbacks
  SetMaybeCallback(cx, &opts, "onFrame", &self, model->onFrameVal);
  SetMaybeCallback(cx, &opts, "onGazeHoverOver", &self, model->onGazeHoverOverVal);
  SetMaybeCallback(cx, &opts, "onGazeHoverOut", &self, model->onGazeHoverOutVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchDown", &self, model->onGestureTouchDownVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchUp", &self, model->onGestureTouchUpVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchCancel", &self, model->onGestureTouchCancelVal);

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreModel_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  model->selfVal.reset();
  model->geometryVal.reset();
  model->programVal.reset();
  model->matrixVal.reset();
  model->positionVal.reset();
  model->rotationVal.reset();
  model->scaleVal.reset();
  model->onFrameVal.reset();
  model->onGazeHoverOverVal.reset();
  model->onGazeHoverOutVal.reset();
  model->onGestureTouchDownVal.reset();
  model->onGestureTouchUpVal.reset();
  model->onGestureTouchCancelVal.reset();
  delete model->computedMatrix;
  delete model;
}

void SetupCoreModel(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core) {
  coreModelClass.finalize = CoreModel_finalize;
  JSObject *obj = JS_InitClass(
      cx,
      *core,
      *global,
      &coreModelClass,
      CoreModel_constructor,
      1,
      nullptr, /* Properties */
      nullptr, /* Methods */
      nullptr, /* Static Props */
      nullptr  /* Static Methods */);
  if (!obj) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Model class\n");
    return;
  }
}

JSObject* NewCoreModel(JSContext *cx, CoreModel* model) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreModelClass));
  if (!JS_DefineProperties(cx, self, CoreModel_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on model\n");
  }
  JS_SetPrivate(self, (void *)model);
  return self;
}

CoreModel* GetCoreModel(JS::HandleObject obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  return model;
}

bool CallbackDefined(mozilla::Maybe<JS::PersistentRootedValue>& val) {
  return val.isSome() && !val.ref().isNullOrUndefined();
}