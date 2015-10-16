#include "CoreModel.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static int CURRENT_MODEL_ID = 1;

int GetNextModelId() {
  return CURRENT_MODEL_ID++;
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

JSObject* NewCoreModel(JSContext *cx, Model* model) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreModelClass, JS::NullPtr(), JS::NullPtr()));
  JS_SetPrivate(self, (void *)model);
  return self;
}

Model* GetCoreModel(JS::HandleObject obj) {
  Model* model = (Model*)JS_GetPrivate(obj);
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

  // Get a reference to the GlGeometry from the opts object
  JS::RootedValue geometry(cx);
  if (!JS_GetProperty(cx, opts, "geometry", &geometry)) {
    JS_ReportError(cx, "Could not find 'geometry' option");
    return false;
  }
  if (!geometry.isObject()) {
    JS_ReportError(cx, "Expected geometry to be an object");
    return false;
  }
  // TODO: Validate object type before casting
  JS::RootedObject geometryObj(cx, &geometry.toObject());

  // Get a reference to the GlProgram from the opts object
  JS::RootedValue program(cx);
  if (!JS_GetProperty(cx, opts, "program", &program)) {
    JS_ReportError(cx, "Could not find 'program' option");
    return false;
  }
  if (!program.isObject()) {
    JS_ReportError(cx, "Expected program to be an object");
    return false;
  }
  // TODO: Validate object type before casting
  JS::RootedObject programObj(cx, &program.toObject());

  // Get a reference to the position vector
  JS::RootedValue positionVal(cx);
  OVR::Vector3f* position;
  if (JS_GetProperty(cx, opts, "position", &positionVal)) {
    // TODO: Error handling (attn: security)
    JS::RootedObject positionObj(cx, &positionVal.toObject());
    position = GetVector3f(positionObj);
  }
  // TODO: Else clause on this

  // Get a reference to the rotation vector
  JS::RootedValue rotationVal(cx);
  OVR::Vector3f* rotation;
  if (JS_GetProperty(cx, opts, "rotation", &rotationVal)) {
    // TODO: Error handling (attn: security)
    JS::RootedObject rotateObj(cx, &rotationVal.toObject());
    rotation = GetVector3f(rotateObj);
  }
  // TODO: Else clause on this

  // Get a reference to the scale vector
  JS::RootedValue scaleVal(cx);
  OVR::Vector3f* scale;
  if (JS_GetProperty(cx, opts, "scale", &scaleVal)) {
    // TODO: Error handling (attn: security)
    JS::RootedObject rotateObj(cx, &scaleVal.toObject());
    scale = GetVector3f(rotateObj);
  }
  // TODO: Else clause on this

  Model* model = new Model;
  model->id = GetNextModelId();
  model->geometry = GetGeometry(geometryObj);
  model->program = GetProgram(programObj);
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

void CoreModel_finalize(JSFreeOp *fop, JSObject *obj) {
  Model* model = (Model*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  model->onFrame.destroyIfConstructed();
  model->onHoverOver.destroyIfConstructed();
  model->onHoverOut.destroyIfConstructed();
  model->onGestureTouchDown.destroyIfConstructed();
  model->onGestureTouchUp.destroyIfConstructed();
  model->onGestureTouchCancel.destroyIfConstructed();
  // TODO: Figure out what to do about ownership of this value and whether to free it
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