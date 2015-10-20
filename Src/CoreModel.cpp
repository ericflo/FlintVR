#include "CoreModel.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static int CURRENT_MODEL_ID = 1;

int GetNextModelId() {
  return CURRENT_MODEL_ID++;
}

void CoreModel::ComputeMatrix(JSContext *cx) {
  OVR::Matrix4f mtx;
  if (matrix(cx) != NULL) {
    mtx = *(matrix(cx));
  }
  if (rotation(cx) != NULL) {
    mtx *= (
      OVR::Matrix4f::RotationX(rotation(cx)->x) *
      OVR::Matrix4f::RotationY(rotation(cx)->y) *
      OVR::Matrix4f::RotationZ(rotation(cx)->z)
    );
  }
  if (scale(cx) != NULL) {
    mtx *= OVR::Matrix4f::Scaling(*(scale(cx)));
  }
  if (position(cx) != NULL) {
    mtx.SetTranslation(*(position(cx)));
  }
  if (computedMatrix != NULL) {
    delete computedMatrix;
    computedMatrix = NULL;
  }
  computedMatrix = new OVR::Matrix4f(mtx);
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

OVR::Matrix4f* CoreModel::matrix(JSContext *cx) {
  if (matrixVal.empty()) {
    return NULL;
  }
  JS::RootedValue val(cx, matrixVal.ref());
  JS::RootedObject obj(cx, &val.toObject());
  return GetMatrix4f(obj);
}

OVR::Vector3f* CoreModel::position(JSContext *cx) {
  if (positionVal.empty()) {
    return NULL;
  }
  JS::RootedValue val(cx, positionVal.ref());
  JS::RootedObject obj(cx, &val.toObject());
  return GetVector3f(obj);
}

OVR::Vector3f* CoreModel::rotation(JSContext *cx) {
  if (rotationVal.empty()) {
    return NULL;
  }
  JS::RootedValue val(cx, rotationVal.ref());
  JS::RootedObject obj(cx, &val.toObject());
  return GetVector3f(obj);
}

OVR::Vector3f* CoreModel::scale(JSContext *cx) {
  if (scaleVal.empty()) {
    return NULL;
  }
  JS::RootedValue val(cx, scaleVal.ref());
  JS::RootedObject obj(cx, &val.toObject());
  return GetVector3f(obj);
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

  // Geometry
  JS::RootedValue geometry(cx);
  if (!JS_GetProperty(cx, opts, "geometry", &geometry)) {
    JS_ReportError(cx, "Could not find 'geometry' option");
    return false;
  }
  if (!_ensureObject(cx, &geometry)) {
    return false;
  }
  if (!_setPersistentVal(cx, &geometry, &m.geometryVal)) {
    return false;
  }

  // Program
  JS::RootedValue program(cx);
  if (!JS_GetProperty(cx, opts, "program", &program)) {
    JS_ReportError(cx, "Could not find 'program' option");
    return false;
  }
  if (!_ensureObject(cx, &program)) {
    return false;
  }
  if (!_setPersistentVal(cx, &program, &m.programVal)) {
    return false;
  }

  // Base transform matrix
  JS::RootedValue matrix(cx);
  if (!JS_GetProperty(cx, opts, "transform", &matrix) || matrix.isNullOrUndefined()) {
    matrix = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreMatrix4f(cx, new OVR::Matrix4f)));
  }
  if (!_ensureObject(cx, &matrix)) {
    return false;
  }
  if (!_setPersistentVal(cx, &matrix, &m.matrixVal)) {
    return false;
  }

  // Position
  JS::RootedValue position(cx);
  if (!JS_GetProperty(cx, opts, "position", &position) || position.isNullOrUndefined()) {
    position = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f)));
  }
  if (!_ensureObject(cx, &position)) {
    return false;
  }
  if (!_setPersistentVal(cx, &position, &m.positionVal)) {
    return false;
  }

  // Rotation
  JS::RootedValue rotation(cx);
  if (!JS_GetProperty(cx, opts, "rotation", &rotation) || rotation.isNullOrUndefined()) {
    rotation = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f)));
  }
  if (!_ensureObject(cx, &rotation)) {
    return false;
  }
  if (!_setPersistentVal(cx, &rotation, &m.rotationVal)) {
    return false;
  }

  // Scale
  JS::RootedValue scale(cx);
  if (!JS_GetProperty(cx, opts, "scale", &scale) || scale.isNullOrUndefined()) {
    scale = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f)));
  }
  if (!_ensureObject(cx, &scale)) {
    return false;
  }
  if (!_setPersistentVal(cx, &scale, &m.scaleVal)) {
    return false;
  }

  CoreModel* model = new CoreModel;
  model->id = GetNextModelId();
  _setPersistentVal(cx, &m.geometryVal.ref(), &model->geometryVal);
  _setPersistentVal(cx, &m.programVal.ref(), &model->programVal);
  _setPersistentVal(cx, &m.matrixVal.ref(), &model->matrixVal);
  _setPersistentVal(cx, &m.positionVal.ref(), &model->positionVal);
  _setPersistentVal(cx, &m.rotationVal.ref(), &model->rotationVal);
  _setPersistentVal(cx, &m.scaleVal.ref(), &model->scaleVal);

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
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->geometryVal);
  }

  // Program property
  if (!_jsStrEq(cx, &propertyName, "program", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->programVal);
  }

  // Matrix property
  if (!_jsStrEq(cx, &propertyName, "transform", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->matrixVal);
  }

  // Position property
  if (!_jsStrEq(cx, &propertyName, "position", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->positionVal);
  }

  // Rotation property
  if (!_jsStrEq(cx, &propertyName, "rotation", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->rotationVal);
  }

  // Scale property
  if (!_jsStrEq(cx, &propertyName, "scale", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->scaleVal);
  }

  return true;
}

bool CoreModel_getProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp) {
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
    vp.set(model->geometryVal.ref());
    return true;
  }

  // Program property
  if (!_jsStrEq(cx, &propertyName, "program", &match)) {
    return false;
  } else if (match) {
    vp.set(model->programVal.ref());
    return true;
  }

  // Matrix property
  if (!_jsStrEq(cx, &propertyName, "transform", &match)) {
    return false;
  } else if (match) {
    vp.set(model->matrixVal.ref());
    return true;
  }

  // Position property
  if (!_jsStrEq(cx, &propertyName, "position", &match)) {
    return false;
  } else if (match) {
    vp.set(model->positionVal.ref());
    return true;
  }

  // Rotation property
  if (!_jsStrEq(cx, &propertyName, "rotation", &match)) {
    return false;
  } else if (match) {
    vp.set(model->rotationVal.ref());
    return true;
  }

  // Scale property
  if (!_jsStrEq(cx, &propertyName, "scale", &match)) {
    return false;
  } else if (match) {
    vp.set(model->scaleVal.ref());
    return true;
  }

  return true;
}

void CoreModel_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  model->geometryVal.destroyIfConstructed();
  model->programVal.destroyIfConstructed();
  model->matrixVal.destroyIfConstructed();
  model->positionVal.destroyIfConstructed();
  model->rotationVal.destroyIfConstructed();
  model->scaleVal.destroyIfConstructed();
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