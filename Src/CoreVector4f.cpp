#include "CoreVector4f.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static JSClass coreVector4fClass = {
  "Vector4f",             /* name */
  JSCLASS_HAS_PRIVATE,    /* flags */
  NULL,
  NULL,
  CoreVector4f_getProperty,
  CoreVector4f_setProperty,
  NULL,
  NULL,
  NULL,
  CoreVector4f_finalize
};

JSObject* NewCoreVector4f(JSContext* cx, OVR::Vector4f* vector4f) {
	JS::RootedObject self(cx, JS_NewObject(cx, &coreVector4fClass));
	JS_SetPrivate(self, (void *)vector4f);
	return self;
}

OVR::Vector4f* GetVector4f(JS::HandleObject obj) {
	OVR::Vector4f* vector4f = (OVR::Vector4f*)JS_GetPrivate(obj);
	return vector4f;
}

bool CoreVector4f_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	// Check the arguments length
	if (args.length() != 4) {
		JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 4);
		return false;
	}

	// Check to make sure the args are strings
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

	OVR::Vector4f* vector4f = new OVR::Vector4f(x, y, z, w);

	// Go ahead and create our self object
	JS::RootedObject self(cx, NewCoreVector4f(cx, vector4f));

	// Return our self object
	args.rval().set(JS::ObjectOrNullValue(self));
	return true;
}

bool _setVector4fVertex(JSContext* cx, JS::MutableHandleValue vp, JS::RootedString* propertyName, const char* propName, float* out) {
	bool match;
	// TODO: Can we do this in a more efficient way than string scanning?! Maybe interned strings.
	//       But first measure overhead, maybe this is fast enough.
	if (!JS_StringEqualsAscii(cx, *propertyName, propName, &match)) {
		JS_ReportError(cx, "Could not compare strings");
		return false;
	}
	if (match) {
		if (!vp.isNumber()) {
			JS_ReportError(cx, "Vector4f arguments must all be numbers");
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

bool _getVector4fVertex(JSContext* cx, JS::MutableHandleValue vp, JS::RootedString* propertyName, const char* propName, float* num) {
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

bool CoreVector4f_setProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp, JS::ObjectOpResult& result) {
	if (JSID_IS_STRING(id)) {
		JS::RootedString propertyName(cx, JSID_TO_STRING(id));
		OVR::Vector4f* vector4f = GetVector4f(obj);
		if (!_setVector4fVertex(cx, vp, &propertyName, "x", &vector4f->x)) {
			return false;
		}
		if (!_setVector4fVertex(cx, vp, &propertyName, "y", &vector4f->y)) {
			return false;
		}
		if (!_setVector4fVertex(cx, vp, &propertyName, "z", &vector4f->z)) {
			return false;
		}
		if (!_setVector4fVertex(cx, vp, &propertyName, "w", &vector4f->w)) {
			return false;
		}
	}
	return true;
}

bool CoreVector4f_getProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp) {
	if (JSID_IS_STRING(id)) {
		JS::RootedString propertyName(cx, JSID_TO_STRING(id));
		OVR::Vector4f* vector4f = GetVector4f(obj);
		if (!_getVector4fVertex(cx, vp, &propertyName, "x", &vector4f->x)) {
			return false;
		}
		if (!_getVector4fVertex(cx, vp, &propertyName, "y", &vector4f->y)) {
			return false;
		}
		if (!_getVector4fVertex(cx, vp, &propertyName, "z", &vector4f->z)) {
			return false;
		}
		if (!_getVector4fVertex(cx, vp, &propertyName, "w", &vector4f->w)) {
			return false;
		}
	}
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