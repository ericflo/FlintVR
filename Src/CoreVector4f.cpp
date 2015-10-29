#include "CoreVector4f.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static JSClass coreVector4fClass = {
  "Vector4f",             /* name */
  JSCLASS_HAS_PRIVATE,    /* flags */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  CoreVector4f_finalize
};

static bool CoreVector4f_get_x(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector4f* item = GetVector4f(self);
  args.rval().setNumber(item->x);
  return true;
}

static bool CoreVector4f_set_x(JSContext* cx, unsigned argc, JS::Value* vp) { \
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector4f* item = GetVector4f(self);
  item->x = args[0].toNumber();
  return true;
}

static bool CoreVector4f_get_y(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector4f* item = GetVector4f(self);
  args.rval().setNumber(item->y);
  return true;
}

static bool CoreVector4f_set_y(JSContext* cx, unsigned argc, JS::Value* vp) { \
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector4f* item = GetVector4f(self);
  item->y = args[0].toNumber();
  return true;
}

static bool CoreVector4f_get_z(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector4f* item = GetVector4f(self);
  args.rval().setNumber(item->z);
  return true;
}

static bool CoreVector4f_set_z(JSContext* cx, unsigned argc, JS::Value* vp) { \
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector4f* item = GetVector4f(self);
  item->z = args[0].toNumber();
  return true;
}

static bool CoreVector4f_get_w(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector4f* item = GetVector4f(self);
  args.rval().setNumber(item->w);
  return true;
}

static bool CoreVector4f_set_w(JSContext* cx, unsigned argc, JS::Value* vp) { \
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  OVR::Vector4f* item = GetVector4f(self);
  item->w = args[0].toNumber();
  return true;
}

static JSPropertySpec CoreVector4f_props[] = {
  VRJS_PROP(CoreVector4f, x),
  VRJS_PROP(CoreVector4f, y),
  VRJS_PROP(CoreVector4f, z),
  VRJS_PROP(CoreVector4f, w),
  JS_PS_END
};

JSObject* NewCoreVector4f(JSContext* cx, OVR::Vector4f* vec) {
	JS::RootedObject self(cx, JS_NewObject(cx, &coreVector4fClass));
	if (!JS_DefineProperties(cx, self, CoreVector4f_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on Vector4f\n");
  }
	JS_SetPrivate(self, (void *)vec);
	return self;
}

OVR::Vector4f* GetVector4f(JS::HandleObject obj) {
	return (OVR::Vector4f*)JS_GetPrivate(obj);
}

bool CoreVector4f_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	// Check the arguments length
	if (args.length() != 4) {
		JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 4);
		return false;
	}

	// Check to make sure the args are numbers
	if (!args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber() || !args[3].isNumber()) {
		JS_ReportError(cx, "Vector4f arguments must all be numbers");
		return false;
	}

	double x, y, z, w;
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
	if (!JS::ToNumber(cx, args[3], &w)) {
		JS_ReportError(cx, "Could not convert fourth argument to double");
		return false;
	}

	// Go ahead and create our self object
	JS::RootedObject self(cx, NewCoreVector4f(cx, new OVR::Vector4f(x, y, z, w)));

	// Return our self object
	args.rval().set(JS::ObjectOrNullValue(self));
	return true;
}

void CoreVector4f_finalize(JSFreeOp *fop, JSObject *obj) {
	OVR::Vector4f* vector4f = (OVR::Vector4f*)JS_GetPrivate(obj);
	JS_SetPrivate(obj, NULL);
	// TODO: Figure out what to do about ownership of this value and whether to free it
	delete vector4f;
}

void SetupCoreVector4f(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core) {
	JSObject *obj = JS_InitClass(
			cx,
			*core,
			*global,
			&coreVector4fClass,
			CoreVector4f_constructor,
			4,
			nullptr, /* Properties */
			nullptr, /* Methods */
			nullptr, /* Static Props */
			nullptr  /* Static Methods */);
	if (!obj) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Vector4f class\n");
		return;
	}
}

const JSClass* CoreVector4f_class() {
  return &coreVector4fClass;
}