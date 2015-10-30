#include "CoreVector2f.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static JSClass coreVector2fClass = {
  "Vector2f",             /* name */
  JSCLASS_HAS_PRIVATE,    /* flags */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  CoreVector2f_finalize
};

static bool CoreVector2f_get_x(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector2f* item = GetVector2f(self);
  args.rval().setNumber(item->x);
  return true;
}

static bool CoreVector2f_set_x(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector2f* item = GetVector2f(self);
  item->x = args[0].toNumber();
  return true;
}

static bool CoreVector2f_get_y(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector2f* item = GetVector2f(self);
  args.rval().setNumber(item->y);
  return true;
}

static bool CoreVector2f_set_y(JSContext* cx, unsigned argc, JS::Value* vp) { \
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector2f* item = GetVector2f(self);
  item->y = args[0].toNumber();
  return true;
}

static JSPropertySpec CoreVector2f_props[] = {
  VRJS_PROP(CoreVector2f, x),
  VRJS_PROP(CoreVector2f, y),
  JS_PS_END
};

JSObject* NewCoreVector2f(JSContext* cx, OVR::Vector2f* vec) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreVector2fClass));
  if (!JS_DefineProperties(cx, self, CoreVector2f_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on Vector2f\n");
  }
  JS_SetPrivate(self, (void *)vec);
  return self;
}

OVR::Vector2f* GetVector2f(JS::HandleObject obj) {
  return (OVR::Vector2f*)JS_GetPrivate(obj);
}

bool CoreVector2f_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 2) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 2);
    return false;
  }

  // Check to make sure the args are numbers
  if (!args[0].isNumber() || !args[1].isNumber()) {
    JS_ReportError(cx, "Vector3f arguments must all be numbers");
    return false;
  }

  double x, y;
  if (!JS::ToNumber(cx, args[0], &x)) {
    JS_ReportError(cx, "Could not convert first argument to double");
    return false;
  }
  if (!JS::ToNumber(cx, args[1], &y)) {
    JS_ReportError(cx, "Could not convert second argument to double");
    return false;
  }

  // Go ahead and create our self object
  JS::RootedObject self(cx, NewCoreVector2f(cx, new OVR::Vector2f(x, y)));

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreVector2f_finalize(JSFreeOp *fop, JSObject *obj) {
  OVR::Vector2f* vec = (OVR::Vector2f*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete vec;
}

void SetupCoreVector2f(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core) {
  JSObject *obj = JS_InitClass(
      cx,
      *core,
      *global,
      &coreVector2fClass,
      CoreVector2f_constructor,
      2,
      nullptr, /* Properties */
      nullptr, /* Methods */
      nullptr, /* Static Props */
      nullptr  /* Static Methods */);
  if (!obj) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Vector2f class\n");
    return;
  }
}

const JSClass* CoreVector2f_class() {
  return &coreVector2fClass;
}