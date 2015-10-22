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

#define COREMODEL_IMPL(name) \
  static bool CoreModel_get_##name(JSContext *cx, unsigned argc, JS::Value *vp) { \
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
    JS::RootedObject self(cx, &args.thisv().toObject()); \
    CoreModel* model = GetCoreModel(self); \
    args.rval().set(model == NULL || model->name##Val.isNothing() ? JS::NullValue() : model->name##Val.ref()); \
    return true; \
  } \
  \
  static bool CoreModel_set_##name(JSContext *cx, unsigned argc, JS::Value *vp) { \
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
    if (!args[0].isObject()) { \
      JS_ReportError(cx, "Unexpected argument (expected ##name)"); \
      return false; \
    } \
    JS::RootedObject self(cx, &args.thisv().toObject()); \
    CoreModel* model = GetCoreModel(self); \
    mozilla::Maybe<JS::PersistentRootedValue>* val = &model->name##Val; \
    val->reset(); \
    val->emplace(cx, *vp); \
    return true; \
  }

COREMODEL_IMPL(geometry)
COREMODEL_IMPL(program)
COREMODEL_IMPL(matrix)
COREMODEL_IMPL(position)
COREMODEL_IMPL(rotation)
COREMODEL_IMPL(scale)
COREMODEL_IMPL(onFrame)
COREMODEL_IMPL(onGazeHoverOver)
COREMODEL_IMPL(onGazeHoverOut)
COREMODEL_IMPL(onGestureTouchDown)
COREMODEL_IMPL(onGestureTouchUp)
COREMODEL_IMPL(onGestureTouchCancel)

#define COREMODEL_PROPSPEC(name) \
  JS_PSGS(#name, CoreModel_get_##name, CoreModel_set_##name, JSPROP_PERMANENT)

static JSPropertySpec CoreModel_props[] = {
  COREMODEL_PROPSPEC(geometry),
  COREMODEL_PROPSPEC(program),
  COREMODEL_PROPSPEC(matrix),
  COREMODEL_PROPSPEC(position),
  COREMODEL_PROPSPEC(rotation),
  COREMODEL_PROPSPEC(scale),
  COREMODEL_PROPSPEC(onFrame),
  COREMODEL_PROPSPEC(onGazeHoverOver),
  COREMODEL_PROPSPEC(onGazeHoverOut),
  COREMODEL_PROPSPEC(onGestureTouchDown),
  COREMODEL_PROPSPEC(onGestureTouchUp),
  COREMODEL_PROPSPEC(onGestureTouchCancel),
  JS_PS_END
};

#define COREMODEL_MEMBER(name, getter) \
  CoreModel::name(JSContext *cx) { \
    if (name##Val.isNothing()) { \
      return NULL; \
    } \
    JS::RootedValue val(cx, name##Val.ref()); \
    JS::RootedObject obj(cx, &val.toObject()); \
    return getter(obj); \
  }

CoreGeometry* COREMODEL_MEMBER(geometry, GetGeometry);
OVR::GlProgram* COREMODEL_MEMBER(program, GetProgram);
OVR::Matrix4f* COREMODEL_MEMBER(matrix, GetMatrix4f);
OVR::Vector3f* COREMODEL_MEMBER(position, GetVector3f);
OVR::Vector3f* COREMODEL_MEMBER(rotation, GetVector3f);
OVR::Vector3f* COREMODEL_MEMBER(scale, GetVector3f);

bool _setPersistentVal(JSContext *cx, JS::MutableHandleValue vp, mozilla::Maybe<JS::PersistentRootedValue>& out) {
  out.reset();
  out.emplace(cx, vp);
  return true;
}

bool _ensureObject(JSContext *cx, JS::MutableHandleValue vp) {
  if (!vp.isObject()) {
    JS_ReportError(cx, "Unexpected argument (expected object)");
    return false;
  }
  return true;
}

bool _constructorCallback(JSContext *cx, JS::RootedObject* opts, const char* name, JS::RootedObject* self, mozilla::Maybe<JS::PersistentRootedValue>& out) {
  JS::RootedValue callbackVal(cx);
  if (!JS_GetProperty(cx, *opts, name, &callbackVal) || callbackVal.isNullOrUndefined()) {
    callbackVal = JS::RootedValue(cx, JS::NullValue());
  }
  if (callbackVal.isNullOrUndefined()) {
    out.reset();
    return true;
  }
  if (!_setPersistentVal(cx, &callbackVal, out)) {
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
  if (!_setPersistentVal(cx, &selfVal, model->selfVal)) {
    delete model;
    return false;
  }

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
  if (!_setPersistentVal(cx, &geometry, model->geometryVal)) {
    delete model;
    return false;
  }

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
  if (!_setPersistentVal(cx, &program, model->programVal)) {
    delete model;
    return false;
  }

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
  if (!_setPersistentVal(cx, &matrix, model->matrixVal)) {
    delete model;
    return false;
  }

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
  if (!_setPersistentVal(cx, &position, model->positionVal)) {
    delete model;
    return false;
  }

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
  if (!_setPersistentVal(cx, &rotation, model->rotationVal)) {
    delete model;
    return false;
  }

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
  if (!_setPersistentVal(cx, &scale, model->scaleVal)) {
    delete model;
    return false;
  }

  // Callbacks
  if (!_constructorCallback(cx, &opts, "onFrame", &self, model->onFrameVal)) {
    delete model;
    return false;
  }
  if (!_constructorCallback(cx, &opts, "onGazeHoverOver", &self, model->onGazeHoverOverVal)) {
    delete model;
    return false;
  }
  if (!_constructorCallback(cx, &opts, "onGazeHoverOut", &self, model->onGazeHoverOutVal)) {
    delete model;
    return false;
  }
  if (!_constructorCallback(cx, &opts, "onGestureTouchDown", &self, model->onGestureTouchDownVal)) {
    delete model;
    return false;
  }
  if (!_constructorCallback(cx, &opts, "onGestureTouchUp", &self, model->onGestureTouchUpVal)) {
    delete model;
    return false;
  }
  if (!_constructorCallback(cx, &opts, "onGestureTouchCancel", &self, model->onGestureTouchCancelVal)) {
    delete model;
    return false;
  }

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