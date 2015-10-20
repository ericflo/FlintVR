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

bool _bindFunction(JSContext* cx, JS::RootedObject* self, JS::RootedValue* funcVal) {
  if (!funcVal->isObject()) {
    JS_ReportError(cx, "Invalid function value");
    return false;
  }
  JS::RootedObject funcObj(cx, &funcVal->toObject());
  if (!JS_ObjectIsFunction(cx, funcObj)) {
    JS_ReportError(cx, "Invalid function value detected");
    return false;
  }
  JS::RootedObject boundFuncObj(cx, JS_BindCallable(cx, funcObj, *self));
  funcVal->set(JS::ObjectOrNullValue(boundFuncObj));
  return true;
}

bool _constructorCallback(JSContext *cx, JS::RootedObject* opts, const char* name, JS::RootedObject* self, mozilla::Maybe<JS::PersistentRootedValue>* out) {
  JS::RootedValue callbackVal(cx);
  if (!JS_GetProperty(cx, *opts, name, &callbackVal) || callbackVal.isNullOrUndefined()) {
    callbackVal = JS::RootedValue(cx, JS::NullValue());
  }
  if (callbackVal.isNullOrUndefined()) {
    out->destroyIfConstructed();
    return true;
  }
  if (!_bindFunction(cx, self, &callbackVal)) {
    return false;
  }
  if (!_setPersistentVal(cx, &callbackVal, out)) {
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

  // Create our self object
  CoreModel* model = new CoreModel;
  model->id = GetNextModelId();
  JS::RootedObject self(cx, NewCoreModel(cx, model));

  CoreModel m;

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
  if (!_setPersistentVal(cx, &geometry, &model->geometryVal)) {
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
  if (!_setPersistentVal(cx, &program, &model->programVal)) {
    delete model;
    return false;
  }

  // Base transform matrix
  JS::RootedValue matrix(cx);
  if (!JS_GetProperty(cx, opts, "transform", &matrix) || matrix.isNullOrUndefined()) {
    matrix = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreMatrix4f(cx, new OVR::Matrix4f)));
  }
  if (!_ensureObject(cx, &matrix)) {
    delete model;
    return false;
  }
  if (!_setPersistentVal(cx, &matrix, &model->matrixVal)) {
    delete model;
    return false;
  }

  // Position
  JS::RootedValue position(cx);
  if (!JS_GetProperty(cx, opts, "position", &position) || position.isNullOrUndefined()) {
    position = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f)));
  }
  if (!_ensureObject(cx, &position)) {
    delete model;
    return false;
  }
  if (!_setPersistentVal(cx, &position, &model->positionVal)) {
    delete model;
    return false;
  }

  // Rotation
  JS::RootedValue rotation(cx);
  if (!JS_GetProperty(cx, opts, "rotation", &rotation) || rotation.isNullOrUndefined()) {
    rotation = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f)));
  }
  if (!_ensureObject(cx, &rotation)) {
    delete model;
    return false;
  }
  if (!_setPersistentVal(cx, &rotation, &model->rotationVal)) {
    delete model;
    return false;
  }

  // Scale
  JS::RootedValue scale(cx);
  if (!JS_GetProperty(cx, opts, "scale", &scale) || scale.isNullOrUndefined()) {
    scale = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f)));
  }
  if (!_ensureObject(cx, &scale)) {
    delete model;
    return false;
  }
  if (!_setPersistentVal(cx, &scale, &model->scaleVal)) {
    delete model;
    return false;
  }

  // Callbacks
  if (!_constructorCallback(cx, &opts, "onFrame", &self, &model->onFrameVal)) {
    delete model;
    return false;
  }
  if (!_constructorCallback(cx, &opts, "onGazeHoverOver", &self, &model->onGazeHoverOverVal)) {
    delete model;
    return false;
  }
  if (!_constructorCallback(cx, &opts, "onGazeHoverOut", &self, &model->onGazeHoverOutVal)) {
    delete model;
    return false;
  }
  if (!_constructorCallback(cx, &opts, "onGestureTouchDown", &self, &model->onGestureTouchDownVal)) {
    delete model;
    return false;
  }
  if (!_constructorCallback(cx, &opts, "onGestureTouchUp", &self, &model->onGestureTouchUpVal)) {
    delete model;
    return false;
  }
  if (!_constructorCallback(cx, &opts, "onGestureTouchCancel", &self, &model->onGestureTouchCancelVal)) {
    delete model;
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

  // OnFrame callback
  if (!_jsStrEq(cx, &propertyName, "onFrame", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->onFrameVal);
  }

  // OnGazeHoverOver callback
  if (!_jsStrEq(cx, &propertyName, "onGazeHoverOver", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->onGazeHoverOverVal);
  }

  // OnGazeHoverOut callback
  if (!_jsStrEq(cx, &propertyName, "onGazeHoverOut", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->onGazeHoverOutVal);
  }

  // OnGestureTouchDown callback
  if (!_jsStrEq(cx, &propertyName, "onGestureTouchDown", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->onGestureTouchDownVal);
  }

  // OnGestureTouchUp callback
  if (!_jsStrEq(cx, &propertyName, "onGestureTouchUp", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->onGestureTouchUpVal);
  }

  // OnGestureTouchCancel callback
  if (!_jsStrEq(cx, &propertyName, "onGestureTouchCancel", &match)) {
    return false;
  } else if (match) {
    if (!_ensureObject(cx, vp)) {
      return false;
    }
    return _setPersistentVal(cx, vp, &model->onGestureTouchCancelVal);
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

  // OnFrame callback
  if (!_jsStrEq(cx, &propertyName, "onFrame", &match)) {
    return false;
  } else if (match) {
    vp.set(model->onFrameVal.empty() ?
      JS::NullValue() : model->onFrameVal.ref());
    return true;
  }

  // OnGazeHoverOver callback
  if (!_jsStrEq(cx, &propertyName, "onGazeHoverOver", &match)) {
    return false;
  } else if (match) {
    vp.set(model->onGazeHoverOverVal.empty() ?
      JS::NullValue() : model->onGazeHoverOverVal.ref());
    return true;
  }

  // OnGazeHoverOut callback
  if (!_jsStrEq(cx, &propertyName, "onGazeHoverOut", &match)) {
    return false;
  } else if (match) {
    vp.set(model->onGazeHoverOutVal.empty() ?
      JS::NullValue() : model->onGazeHoverOutVal.ref());
    return true;
  }

  // OnGestureTouchDown callback
  if (!_jsStrEq(cx, &propertyName, "onGestureTouchDown", &match)) {
    return false;
  } else if (match) {
    vp.set(model->onGestureTouchDownVal.empty() ?
      JS::NullValue() : model->onGestureTouchDownVal.ref());
    return true;
  }

  // OnGestureTouchUp callback
  if (!_jsStrEq(cx, &propertyName, "onGestureTouchUp", &match)) {
    return false;
  } else if (match) {
    vp.set(model->onGestureTouchUpVal.empty() ?
      JS::NullValue() : model->onGestureTouchUpVal.ref());
    return true;
  }

  // OnGestureTouchCancel callback
  if (!_jsStrEq(cx, &propertyName, "onGestureTouchCancel", &match)) {
    return false;
  } else if (match) {
    vp.set(model->onGestureTouchCancelVal.empty() ?
      JS::NullValue() : model->onGestureTouchCancelVal.ref());
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
  model->onFrameVal.destroyIfConstructed();
  model->onGazeHoverOverVal.destroyIfConstructed();
  model->onGazeHoverOutVal.destroyIfConstructed();
  model->onGestureTouchDownVal.destroyIfConstructed();
  model->onGestureTouchUpVal.destroyIfConstructed();
  model->onGestureTouchCancelVal.destroyIfConstructed();
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