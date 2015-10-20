#include "CoreModel.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static int CURRENT_MODEL_ID = 1;

int GetNextModelId() {
  return CURRENT_MODEL_ID++;
}

CoreGeometry* CoreModel::geometry(JSContext *cx) {
  if (geometryVal.empty()) {
    return NULL;
  }
  JS::RootedValue val(cx, geometryVal.ref());
  JS::RootedObject obj(cx, &val.toObject());
  return GetGeometry(obj);
}

OVR::GlProgram* CoreModel::program(JSContext *cx) {
  if (programVal.empty()) {
    return NULL;
  }
  JS::RootedValue val(cx, programVal.ref());
  JS::RootedObject obj(cx, &val.toObject());
  return GetProgram(obj);
}

bool _jsStrEq(JSContext *cx, JS::RootedString* first, const char* second, bool *match) {
  if (!JS_StringEqualsAscii(cx, *first, second, match)) {
    JS_ReportError(cx, "Could not compare strings");
    return false;
  }
  return true;
}

bool _setPersistentVal(JSContext *cx, JS::MutableHandleValue vp, mozilla::Maybe<JS::PersistentRootedValue>* out) {
  JS::PersistentRootedValue persistentVal(cx, vp);
  out->destroyIfConstructed();
  out->construct(cx, persistentVal);
  return true;
}

bool _ensureObject(JSContext *cx, JS::MutableHandleValue vp) {
  if (!vp.isObject()) {
    JS_ReportError(cx, "Unexpected argument (expected object)");
    return false;
  }
  return true;
}

bool _ensureGeometry(JSContext *cx, JS::MutableHandleValue vp) {
  if (!_ensureObject(cx, vp)) {
    return false;
  }
  return true;
}

bool _ensureProgram(JSContext *cx, JS::MutableHandleValue vp) {
  if (!_ensureObject(cx, vp)) {
    return false;
  }
  return true;
}

void ComputeModelMatrix(CoreModel* model) {
  OVR::Matrix4f matrix;
  if (model->matrix != NULL) {
    matrix = *(model->matrix);
  }
  if (model->rotation != NULL) {
    matrix *= (
      OVR::Matrix4f::RotationX(model->rotation->x) *
      OVR::Matrix4f::RotationY(model->rotation->y) *
      OVR::Matrix4f::RotationZ(model->rotation->z)
    );
  }
  if (model->scale != NULL) {
    matrix *= OVR::Matrix4f::Scaling(*(model->scale));
  }
  if (model->position != NULL) {
    matrix.SetTranslation(*(model->position));
  }
  if (model->computedMatrix != NULL) {
    delete model->computedMatrix;
    model->computedMatrix = NULL;
  }
  model->computedMatrix = new OVR::Matrix4f(matrix);
}

static JSClass coreModelClass = {
  "Model",                /* name */
  JSCLASS_HAS_PRIVATE,    /* flags */
  JS_PropertyStub,        /* addProperty (JSPropertyOp) */
  JS_DeletePropertyStub,  /* delProperty (JSDeletePropertyOp) */
  JS_PropertyStub,        /* getProperty (JSPropertyOp) */
  JS_StrictPropertyStub,  /* setProperty (JSStrictPropertyOp) */
  JS_EnumerateStub,       /* enumerate   (JSEnumerateOp) */
  JS_ResolveStub,         /* resolve     (JSResolveOp) */
  JS_ConvertStub,         /* convert     (JSConvertOp) */
  NULL,                   /* finalize    (FinalizeOpType) */
  NULL,                   /* call        (JSNative) */
  NULL,                   /* hasInstance (JSHasInstanceOp) */
  NULL,                   /* construct   (JSNative) */
  NULL                    /* trace       (JSTraceOp) */
};

JSObject* NewCoreModel(JSContext *cx, CoreModel* model) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreModelClass, JS::NullPtr(), JS::NullPtr()));
  JS_SetPrivate(self, (void *)model);
  return self;
}

CoreModel* GetCoreModel(JS::HandleObject obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  return model;
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

  CoreModel m;

  // Get a reference to the GlGeometry from the opts object
  JS::RootedValue geometry(cx);
  if (!JS_GetProperty(cx, opts, "geometry", &geometry)) {
    JS_ReportError(cx, "Could not find 'geometry' option");
    return false;
  }
  if (!_ensureGeometry(cx, &geometry)) {
    return false;
  }
  if (!_setPersistentVal(cx, &geometry, &m.geometryVal)) {
    return false;
  }

  // Get a reference to the GlProgram from the opts object
  JS::RootedValue program(cx);
  if (!JS_GetProperty(cx, opts, "program", &program)) {
    JS_ReportError(cx, "Could not find 'program' option");
    return false;
  }
  if (!_ensureProgram(cx, &program)) {
    return false;
  }
  if (!_setPersistentVal(cx, &program, &m.programVal)) {
    return false;
  }

  // Get a reference to the position vector
  JS::RootedValue positionVal(cx);
  OVR::Vector3f* position = NULL;
  if (JS_GetProperty(cx, opts, "position", &positionVal) && !positionVal.isNullOrUndefined()) {
    // TODO: Error handling (attn: security)
    JS::RootedObject positionObj(cx, &positionVal.toObject());
    position = GetVector3f(positionObj);
  }
  // TODO: Else clause on this

  // Get a reference to the rotation vector
  JS::RootedValue rotationVal(cx);
  OVR::Vector3f* rotation = NULL;
  if (JS_GetProperty(cx, opts, "rotation", &rotationVal) && !rotationVal.isNullOrUndefined()) {
    // TODO: Error handling (attn: security)
    JS::RootedObject rotateObj(cx, &rotationVal.toObject());
    rotation = GetVector3f(rotateObj);
  }
  // TODO: Else clause on this

  // Get a reference to the scale vector
  JS::RootedValue scaleVal(cx);
  OVR::Vector3f* scale = NULL;
  if (JS_GetProperty(cx, opts, "scale", &scaleVal) && !scaleVal.isNullOrUndefined()) {
    // TODO: Error handling (attn: security)
    JS::RootedObject rotateObj(cx, &scaleVal.toObject());
    scale = GetVector3f(rotateObj);
  }
  // TODO: Else clause on this

  // Get a reference to the transform matrix
  JS::RootedValue matrixVal(cx);
  OVR::Matrix4f* matrix = NULL;
  if (JS_GetProperty(cx, opts, "transform", &matrixVal) && !matrixVal.isNullOrUndefined()) {
    // TODO: Error handling (attn: security)
    JS::RootedObject matrixObj(cx, &matrixVal.toObject());
    matrix = GetMatrix4f(matrixObj);
  }

  CoreModel* model = new CoreModel;
  model->id = GetNextModelId();
  _setPersistentVal(cx, &m.geometryVal.ref(), &model->geometryVal);
  _setPersistentVal(cx, &m.programVal.ref(), &model->programVal);
  model->matrix = matrix;
  model->position = position;
  model->rotation = rotation;
  model->scale = scale;

  // Go ahead and create our self object
  JS::RootedObject self(cx, NewCoreModel(cx, model));

  // Pull out the onFrame callback if we find one
  JS::RootedValue onFrameVal(cx);
  if (JS_GetProperty(cx, opts, "onFrame", &onFrameVal) &&
      !onFrameVal.isNullOrUndefined()) {
    if (!onFrameVal.isObject()) {
      JS_ReportError(cx, "onFrame callback is expected to be a function");
      return false;
    }
    JS::RootedObject onFrameObj(cx, &onFrameVal.toObject());
    if (!JS_ObjectIsFunction(cx, onFrameObj)) {
      JS_ReportError(cx, "onFrame callback is expected to be a function");
      return false;
    }
    // Make sure 'this' refers to the model in JS
    JS::RootedObject boundOnFrameObj(cx, JS_BindCallable(cx, onFrameObj, self));
    // Persist the callback in our data structure
    JS::PersistentRootedValue onFrame(cx, JS::ObjectOrNullValue(boundOnFrameObj));
    model->onFrame.destroyIfConstructed();
    model->onFrame.construct(cx, onFrame);
  }

  // Pull out the onHoverOver callback if we find one
  JS::RootedValue onHoverOverVal(cx);
  if (JS_GetProperty(cx, opts, "onHoverOver", &onHoverOverVal) &&
      !onHoverOverVal.isNullOrUndefined()) {
    if (!onHoverOverVal.isObject()) {
      JS_ReportError(cx, "onHoverOver callback is expected to be a function");
      return false;
    }
    JS::RootedObject onHoverOverObj(cx, &onHoverOverVal.toObject());
    if (!JS_ObjectIsFunction(cx, onHoverOverObj)) {
      JS_ReportError(cx, "onHoverOver callback is expected to be a function");
      return false;
    }
    // Make sure 'this' refers to the model in JS
    JS::RootedObject boundOnHoverOverObj(cx, JS_BindCallable(cx, onHoverOverObj, self));
    // Persist the callback in our data structure
    JS::PersistentRootedValue onHoverOver(cx, JS::ObjectOrNullValue(boundOnHoverOverObj));
    model->onHoverOver.destroyIfConstructed();
    model->onHoverOver.construct(cx, onHoverOver);
  }

  // Pull out the onHoverOut callback if we find one
  JS::RootedValue onHoverOutVal(cx);
  if (JS_GetProperty(cx, opts, "onHoverOut", &onHoverOutVal) &&
      !onHoverOutVal.isNullOrUndefined()) {
    if (!onHoverOutVal.isObject()) {
      JS_ReportError(cx, "onHoverOut callback is expected to be a function");
      return false;
    }
    JS::RootedObject onHoverOutObj(cx, &onHoverOutVal.toObject());
    if (!JS_ObjectIsFunction(cx, onHoverOutObj)) {
      JS_ReportError(cx, "onHoverOut callback is expected to be a function");
      return false;
    }
    // Make sure 'this' refers to the model in JS
    JS::RootedObject boundOnHoverOutObj(cx, JS_BindCallable(cx, onHoverOutObj, self));
    // Persist the callback in our data structure
    JS::PersistentRootedValue onHoverOut(cx, JS::ObjectOrNullValue(boundOnHoverOutObj));
    model->onHoverOut.destroyIfConstructed();
    model->onHoverOut.construct(cx, onHoverOut);
  }

  // Pull out the onGestureTouchDown callback if we find one
  JS::RootedValue onGestureTouchDownVal(cx);
  if (JS_GetProperty(cx, opts, "onGestureTouchDown", &onGestureTouchDownVal) &&
      !onGestureTouchDownVal.isNullOrUndefined()) {
    if (!onGestureTouchDownVal.isObject()) {
      JS_ReportError(cx, "onGestureTouchDown callback is expected to be a function");
      return false;
    }
    JS::RootedObject onGestureTouchDownObj(cx, &onGestureTouchDownVal.toObject());
    if (!JS_ObjectIsFunction(cx, onGestureTouchDownObj)) {
      JS_ReportError(cx, "onGestureTouchDown callback is expected to be a function");
      return false;
    }
    // Make sure 'this' refers to the model in JS
    JS::RootedObject boundOnGestureTouchDownObj(cx, JS_BindCallable(cx, onGestureTouchDownObj, self));
    // Persist the callback in our data structure
    JS::PersistentRootedValue onGestureTouchDown(cx, JS::ObjectOrNullValue(boundOnGestureTouchDownObj));
    model->onGestureTouchDown.destroyIfConstructed();
    model->onGestureTouchDown.construct(cx, onGestureTouchDown);
  }

  // Pull out the onGestureTouchUp callback if we find one
  JS::RootedValue onGestureTouchUpVal(cx);
  if (JS_GetProperty(cx, opts, "onGestureTouchUp", &onGestureTouchUpVal) &&
      !onGestureTouchUpVal.isNullOrUndefined()) {
    if (!onGestureTouchUpVal.isObject()) {
      JS_ReportError(cx, "onGestureTouchUp callback is expected to be a function");
      return false;
    }
    JS::RootedObject onGestureTouchUpObj(cx, &onGestureTouchUpVal.toObject());
    if (!JS_ObjectIsFunction(cx, onGestureTouchUpObj)) {
      JS_ReportError(cx, "onGestureTouchUp callback is expected to be a function");
      return false;
    }
    // Make sure 'this' refers to the model in JS
    JS::RootedObject boundOnGestureTouchUpObj(cx, JS_BindCallable(cx, onGestureTouchUpObj, self));
    // Persist the callback in our data structure
    JS::PersistentRootedValue onGestureTouchUp(cx, JS::ObjectOrNullValue(boundOnGestureTouchUpObj));
    model->onGestureTouchUp.destroyIfConstructed();
    model->onGestureTouchUp.construct(cx, onGestureTouchUp);
  }

  // Pull out the onGestureTouchCancel callback if we find one
  JS::RootedValue onGestureTouchCancelVal(cx);
  if (JS_GetProperty(cx, opts, "onGestureTouchCancel", &onGestureTouchCancelVal) &&
      !onGestureTouchCancelVal.isNullOrUndefined()) {
    if (!onGestureTouchCancelVal.isObject()) {
      JS_ReportError(cx, "onGestureTouchCancel callback is expected to be a function");
      return false;
    }
    JS::RootedObject onGestureTouchCancelObj(cx, &onGestureTouchCancelVal.toObject());
    if (!JS_ObjectIsFunction(cx, onGestureTouchCancelObj)) {
      JS_ReportError(cx, "onGestureTouchCancel callback is expected to be a function");
      return false;
    }
    // Make sure 'this' refers to the model in JS
    JS::RootedObject boundOnGestureTouchCancelObj(cx, JS_BindCallable(cx, onGestureTouchCancelObj, self));
    // Persist the callback in our data structure
    JS::PersistentRootedValue onGestureTouchCancel(cx, JS::ObjectOrNullValue(boundOnGestureTouchCancelObj));
    model->onGestureTouchCancel.destroyIfConstructed();
    model->onGestureTouchCancel.construct(cx, onGestureTouchCancel);
  }

  if (!JS_SetProperty(cx, self, "geometry", geometry)) {
    JS_ReportError(cx, "Could not set geometry property on model object");
    return false;
  }
  if (!JS_SetProperty(cx, self, "program", program)) {
    JS_ReportError(cx, "Could not set program property on model object");
    return false;
  }
  if (!JS_SetProperty(cx, self, "transform", matrixVal)) {
    JS_ReportError(cx, "Could not set transform property on model object");
    return false;
  }
  if (!JS_SetProperty(cx, self, "position", positionVal)) {
    JS_ReportError(cx, "Could not set position property on model object");
    return false;
  }
  if (!JS_SetProperty(cx, self, "rotation", rotationVal)) {
    JS_ReportError(cx, "Could not set rotation property on model object");
    return false;
  }
  if (!JS_SetProperty(cx, self, "scale", scaleVal)) {
    JS_ReportError(cx, "Could not set scale property on model object");
    return false;
  }

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

bool CoreModel_setProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id, bool strict, JS::MutableHandleValue vp) {
  if (!JSID_IS_STRING(id)) {
    return true;
  }

  JS::RootedString propertyName(cx, JSID_TO_STRING(id));
  CoreModel* model = GetCoreModel(obj);
  bool match;

  // Geometry property
  if (!_jsStrEq(cx, &propertyName, "geometry", &match)) {
    return false;
  } else if (match) {
    if (!_ensureGeometry(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->geometryVal);
  }

  // Program property
  if (!_jsStrEq(cx, &propertyName, "program", &match)) {
    return false;
  } else if (match) {
    if (!_ensureProgram(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->programVal);
  }

  return true;
}

bool CoreModel_getProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp) {
  return JS_PropertyStub(cx, obj, id, vp);
}

void CoreModel_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  model->geometryVal.destroyIfConstructed();
  model->programVal.destroyIfConstructed();
  model->onFrame.destroyIfConstructed();
  model->onHoverOver.destroyIfConstructed();
  model->onHoverOut.destroyIfConstructed();
  model->onGestureTouchDown.destroyIfConstructed();
  model->onGestureTouchUp.destroyIfConstructed();
  model->onGestureTouchCancel.destroyIfConstructed();
  delete model->computedMatrix;
  // TODO: Figure out what to do about ownership of this value and whether to free it
  delete model;
}

void SetupCoreModel(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core) {
  coreModelClass.finalize = CoreModel_finalize;
  coreModelClass.getProperty = CoreModel_getProperty;
  coreModelClass.setProperty = CoreModel_setProperty;
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