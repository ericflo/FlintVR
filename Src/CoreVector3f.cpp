#include "CoreVector3f.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static JSClass coreVector3fClass = {
	"Vector3f",             /* name */
	JSCLASS_HAS_PRIVATE    /* flags */
};

JSObject* NewCoreVector3f(JSContext* cx, OVR::Vector3f* vector3f) {
	JS::RootedObject self(cx, JS_NewObject(cx, &coreVector3fClass));
	if (!JS_DefineFunction(cx, self, "add", &CoreVector3f_add, 0, 0)) {
    JS_ReportError(cx, "Could not create vector3f.add function");
    return NULL;
  }
  if (!JS_DefineFunction(cx, self, "multiply", &CoreVector3f_multiply, 0, 0)) {
    JS_ReportError(cx, "Could not create vector3f.multiply function");
    return NULL;
  }
	JS_SetPrivate(self, (void *)vector3f);
	return self;
}

OVR::Vector3f* GetVector3f(JS::HandleObject obj) {
	OVR::Vector3f* vector3f = (OVR::Vector3f*)JS_GetPrivate(obj);
	return vector3f;
}

bool CoreVector3f_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	// Check the arguments length
	if (args.length() != 3) {
		JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 3);
		return false;
	}

	// Check to make sure the args are strings
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

	OVR::Vector3f* vector3f = new OVR::Vector3f(x, y, z);

	// Go ahead and create our self object
	JS::RootedObject self(cx, NewCoreVector3f(cx, vector3f));

	// Return our self object
	args.rval().set(JS::ObjectOrNullValue(self));
	return true;
}

bool _setVector3fVertex(JSContext* cx, JS::MutableHandleValue vp, JS::RootedString* propertyName, const char* propName, float* out) {
	bool match;
	// TODO: Can we do this in a more efficient way than string scanning?! Maybe interned strings.
	//       But first measure overhead, maybe this is fast enough.
	if (!JS_StringEqualsAscii(cx, *propertyName, propName, &match)) {
		JS_ReportError(cx, "Could not compare strings");
		return false;
	}
	if (match) {
		if (!vp.isNumber()) {
			JS_ReportError(cx, "Vector3f arguments must all be numbers");
			return false;
		}
		double d;
		if (!JS::ToNumber(cx, vp, &d)) {
			JS_ReportError(cx, "Could not convert argument to double");
			return false;
		}
		*out = (float)d;
	}
	return true;
}

bool _getVector3fVertex(JSContext* cx, JS::MutableHandleValue vp, JS::RootedString* propertyName, const char* propName, float* num) {
	bool match;
	// TODO: Can we do this in a more efficient way than string scanning?! Maybe interned strings.
	//       But first measure overhead, maybe this is fast enough.
	if (!JS_StringEqualsAscii(cx, *propertyName, propName, &match)) {
		JS_ReportError(cx, "Could not compare strings");
		return false;
	}
	if (match) {
		vp.setNumber((double)*num);
	}
	return true;
}

bool CoreVector3f_setProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp, JS::ObjectOpResult& result) {
	if (JSID_IS_STRING(id)) {
		JS::RootedString propertyName(cx, JSID_TO_STRING(id));
		OVR::Vector3f* vector3f = GetVector3f(obj);
		if (!_setVector3fVertex(cx, vp, &propertyName, "x", &vector3f->x)) {
			return false;
		}
		if (!_setVector3fVertex(cx, vp, &propertyName, "y", &vector3f->y)) {
			return false;
		}
		if (!_setVector3fVertex(cx, vp, &propertyName, "z", &vector3f->z)) {
			return false;
		}
	}
	return true;
}

bool CoreVector3f_getProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp) {
	if (JSID_IS_STRING(id)) {
		JS::RootedString propertyName(cx, JSID_TO_STRING(id));
		OVR::Vector3f* vector3f = GetVector3f(obj);
		if (!_getVector3fVertex(cx, vp, &propertyName, "x", &vector3f->x)) {
			return false;
		}
		if (!_getVector3fVertex(cx, vp, &propertyName, "y", &vector3f->y)) {
			return false;
		}
		if (!_getVector3fVertex(cx, vp, &propertyName, "z", &vector3f->z)) {
			return false;
		}
	}
	return true;
}

void CoreVector3f_finalize(JSFreeOp *fop, JSObject *obj) {
	OVR::Vector3f* vector3f = (OVR::Vector3f*)JS_GetPrivate(obj);
	JS_SetPrivate(obj, NULL);
	// TODO: Figure out what to do about ownership of this value and whether to free it
	delete vector3f;
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
	coreVector3fClass.finalize = CoreVector3f_finalize;
	coreVector3fClass.setProperty = CoreVector3f_setProperty;
	coreVector3fClass.getProperty = CoreVector3f_getProperty;
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
