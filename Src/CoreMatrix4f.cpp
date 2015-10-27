#include "CoreMatrix4f.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static JSClass coreMatrix4fClass = {
  "Matrix4f",             /* name */
  JSCLASS_HAS_PRIVATE    /* flags */
};

JSObject* NewCoreMatrix4f(JSContext* cx, OVR::Matrix4f* matrix4f) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreMatrix4fClass));
  if (!JS_DefineFunction(cx, self, "setTranslation", &CoreMatrix4f_setTranslation, 0, 0)) {
    JS_ReportError(cx, "Could not create matrix4f.setTranslation function");
    return NULL;
  }
  if (!JS_DefineFunction(cx, self, "multiply", &CoreMatrix4f_multiply, 0, 0)) {
    JS_ReportError(cx, "Could not create matrix4f.multiply function");
    return NULL;
  }
  if (!JS_DefineFunction(cx, self, "rotationX", &CoreMatrix4f_rotationX, 0, 0)) {
    JS_ReportError(cx, "Could not create matrix4f.rotationX function");
    return NULL;
  }
  if (!JS_DefineFunction(cx, self, "rotationY", &CoreMatrix4f_rotationY, 0, 0)) {
    JS_ReportError(cx, "Could not create matrix4f.rotationY function");
    return NULL;
  }
  if (!JS_DefineFunction(cx, self, "rotationZ", &CoreMatrix4f_rotationZ, 0, 0)) {
    JS_ReportError(cx, "Could not create matrix4f.rotationZ function");
    return NULL;
  }
  if (!JS_DefineFunction(cx, self, "transform", &CoreMatrix4f_transform, 0, 0)) {
    JS_ReportError(cx, "Could not create matrix4f.transform function");
    return NULL;
  }
  JS_SetPrivate(self, (void *)matrix4f);
  return self;
}

OVR::Matrix4f* GetMatrix4f(JS::HandleObject obj) {
  OVR::Matrix4f* matrix4f = (OVR::Matrix4f*)JS_GetPrivate(obj);
  return matrix4f;
}

bool CoreMatrix4f_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  OVR::Matrix4f* matrix4f = new OVR::Matrix4f();
  JS::RootedObject self(cx, NewCoreMatrix4f(cx, matrix4f));

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreMatrix4f_finalize(JSFreeOp *fop, JSObject *obj) {
  OVR::Matrix4f* matrix4f = (OVR::Matrix4f*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  // TODO: Figure out what to do about ownership of this value and whether to free it
  delete matrix4f;
}

bool CoreMatrix4f_setTranslation(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Expected a vector3f argument");
    return false;
  }
  JS::RootedObject vecObj(cx, &args[0].toObject());
  OVR::Vector3f* vec = GetVector3f(vecObj);
  if (vec == NULL) {
    JS_ReportError(cx, "Expected a vector3f argument");
    return false;
  }

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  OVR::Matrix4f* mat = GetMatrix4f(thisObj);
  mat->SetTranslation(*vec);

  return true;
}

bool CoreMatrix4f_multiply(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Expected a matrix4f argument");
    return false;
  }
  JS::RootedObject otherMatObj(cx, &args[0].toObject());
  OVR::Matrix4f* otherMat = GetMatrix4f(otherMatObj);
  if (otherMat == NULL) {
    JS_ReportError(cx, "Expected a matrix4f argument");
    return false;
  }

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  OVR::Matrix4f* mat = GetMatrix4f(thisObj);

  // Do the multiplication
  OVR::Matrix4f* res = new OVR::Matrix4f(*mat * *otherMat);
  JS::RootedObject result(cx, NewCoreMatrix4f(cx, res));

  args.rval().set(JS::ObjectOrNullValue(result));
  return true;
}

bool CoreMatrix4f_rotationX(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Expected a number argument");
    return false;
  }
  double deg;
  if (!JS::ToNumber(cx, args[0], &deg)) {
    JS_ReportError(cx, "Could not convert first argument to double");
    return false;
  }

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  OVR::Matrix4f* mat = GetMatrix4f(thisObj);
  OVR::Matrix4f* res = new OVR::Matrix4f(mat->RotationX((float)deg));
  JS::RootedObject result(cx, NewCoreMatrix4f(cx, res));

  args.rval().set(JS::ObjectOrNullValue(result));
  return true;
}

bool CoreMatrix4f_rotationY(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Expected a number argument");
    return false;
  }
  double deg;
  if (!JS::ToNumber(cx, args[0], &deg)) {
    JS_ReportError(cx, "Could not convert first argument to double");
    return false;
  }

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  OVR::Matrix4f* mat = GetMatrix4f(thisObj);
  OVR::Matrix4f* res = new OVR::Matrix4f(mat->RotationY((float)deg));
  JS::RootedObject result(cx, NewCoreMatrix4f(cx, res));

  args.rval().set(JS::ObjectOrNullValue(result));
  return true;
}

bool CoreMatrix4f_rotationZ(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Expected a number argument");
    return false;
  }
  double deg;
  if (!JS::ToNumber(cx, args[0], &deg)) {
    JS_ReportError(cx, "Could not convert first argument to double");
    return false;
  }

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  OVR::Matrix4f* mat = GetMatrix4f(thisObj);
  OVR::Matrix4f* res = new OVR::Matrix4f(mat->RotationZ((float)deg));
  JS::RootedObject result(cx, NewCoreMatrix4f(cx, res));

  args.rval().set(JS::ObjectOrNullValue(result));
  return true;
}

bool CoreMatrix4f_transform(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Expected a vector3f argument");
    return false;
  }
  JS::RootedObject vecObj(cx, &args[0].toObject());
  OVR::Vector3f* vec = GetVector3f(vecObj);
  if (vec == NULL) {
    JS_ReportError(cx, "Expected a vector3f argument");
    return false;
  }

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  OVR::Matrix4f* mat = GetMatrix4f(thisObj);

  OVR::Vector3f* res = new OVR::Vector3f(mat->Transform(*vec));
  JS::RootedObject result(cx, NewCoreVector3f(cx, res));

  args.rval().set(JS::ObjectOrNullValue(result));
  return true;
}

void SetupCoreMatrix4f(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core) {
  coreMatrix4fClass.finalize = CoreMatrix4f_finalize;
  JSObject *obj = JS_InitClass(
      cx,
      *core,
      *global,
      &coreMatrix4fClass,
      CoreMatrix4f_constructor,
      3,
      nullptr, /* Properties */
      nullptr, /* Methods */
      nullptr, /* Static Props */
      nullptr  /* Static Methods */);
  if (!obj) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Matrix4f class\n");
    return;
  }
}