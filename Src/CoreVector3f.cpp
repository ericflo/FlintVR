#include "CoreVector3f.h"


static JSClass coreVector3fClass = {
  "Vector3f",             /* name */
  JSCLASS_HAS_PRIVATE,    /* flags */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  CoreVector3f_finalize
};

static bool CoreVector3f_get_x(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector3f* item = GetVector3f(self);
  args.rval().setNumber(item->x);
  return true;
}

static bool CoreVector3f_set_x(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector3f* item = GetVector3f(self);
  item->x = args[0].toNumber();
  return true;
}

static bool CoreVector3f_get_y(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector3f* item = GetVector3f(self);
  args.rval().setNumber(item->y);
  return true;
}

static bool CoreVector3f_set_y(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector3f* item = GetVector3f(self);
  item->y = args[0].toNumber();
  return true;
}

static bool CoreVector3f_get_z(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector3f* item = GetVector3f(self);
  args.rval().setNumber(item->z);
  return true;
}

static bool CoreVector3f_set_z(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector3f* item = GetVector3f(self);
  item->z = args[0].toNumber();
  return true;
}

static JSPropertySpec CoreVector3f_props[] = {
  VRJS_PROP(CoreVector3f, x),
  VRJS_PROP(CoreVector3f, y),
  VRJS_PROP(CoreVector3f, z),
  JS_PS_END
};

JSObject* NewCoreVector3f(JSContext* cx, OVR::Vector3f* vec) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreVector3fClass));
  if (!JS_DefineProperties(cx, self, CoreVector3f_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on Vector3f\n");
  }
  if (!JS_DefineFunction(cx, self, "add", &CoreVector3f_add, 0, 0)) {
    JS_ReportError(cx, "Could not create Vector3f.add function");
    return NULL;
  }
  if (!JS_DefineFunction(cx, self, "multiply", &CoreVector3f_multiply, 0, 0)) {
    JS_ReportError(cx, "Could not create Vector3f.multiply function");
    return NULL;
  }
  JS_SetPrivate(self, (void *)vec);
  return self;
}

OVR::Vector3f* GetVector3f(JS::HandleObject obj) {
  return (OVR::Vector3f*)JS_GetPrivate(obj);
}

bool CoreVector3f_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 3) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 3);
    return false;
  }

  // Check to make sure the args are numbers
  if (!args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber()) {
    JS_ReportError(cx, "Vector3f arguments must all be numbers");
    return false;
  }

  double x, y, z;
  if (!JS::ToNumber(cx, args[0], &x)) {
    JS_ReportError(cx, "Could not convert first argument to double");
    return false;
  }
  if (!JS::ToNumber(cx, args[1], &y)) {
    JS_ReportError(cx, "Could not convert second argument to double");
    return false;
  }
  if (!JS::ToNumber(cx, args[2], &z)) {
    JS_ReportError(cx, "Could not convert third argument to double");
    return false;
  }

  // Go ahead and create our self object
  JS::RootedObject self(cx, NewCoreVector3f(cx, new OVR::Vector3f(x, y, z)));

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreVector3f_finalize(JSFreeOp *fop, JSObject *obj) {
  OVR::Vector3f* vec = (OVR::Vector3f*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete vec;
}

bool CoreVector3f_add(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Expected a vector3f argument");
    return false;
  }
  JS::RootedObject otherVecObj(cx, &args[0].toObject());
  OVR::Vector3f* otherVec = GetVector3f(otherVecObj);
  if (otherVec == NULL) {
    JS_ReportError(cx, "Expected a vector3f argument");
    return false;
  }

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  OVR::Vector3f* vec = GetVector3f(thisObj);

  // Do the addition
  OVR::Vector3f* res = new OVR::Vector3f(*vec + *otherVec);
  JS::RootedObject result(cx, NewCoreVector3f(cx, res));

  args.rval().set(JS::ObjectOrNullValue(result));
  return true;
}

bool CoreVector3f_multiply(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  OVR::Vector3f* vec = GetVector3f(thisObj);

  OVR::Vector3f* res;
  if (args[0].isNumber()) {
    double d;
    if (!JS::ToNumber(cx, args[0], &d)) {
      JS_ReportError(cx, "Could not convert argument to double");
      return false;
    }
    // Do the multiplication
    res = new OVR::Vector3f(*vec * (float)d);
  } else if (args[0].isObject()) {
    JS::RootedObject otherVecObj(cx, &args[0].toObject());
    OVR::Vector3f* otherVec = GetVector3f(otherVecObj);
    if (otherVec == NULL) {
      JS_ReportError(cx, "Expected a vector3f argument");
      return false;
    }
    // Do the multiplication
    res = new OVR::Vector3f(*vec * *otherVec);
  } else {
    JS_ReportError(cx, "Expected a vector3f or number argument");
    return false;
  }
  JS::RootedObject result(cx, NewCoreVector3f(cx, res));

  args.rval().set(JS::ObjectOrNullValue(result));
  return true;
}

void SetupCoreVector3f(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core) {
  JSObject *obj = JS_InitClass(
      cx,
      *core,
      *global,
      &coreVector3fClass,
      CoreVector3f_constructor,
      3,
      nullptr, /* Properties */
      nullptr, /* Methods */
      nullptr, /* Static Props */
      nullptr  /* Static Methods */);
  if (!obj) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Vector3f class\n");
    return;
  }
}

const JSClass* CoreVector3f_class() {
  return &coreVector3fClass;
}