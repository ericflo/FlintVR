#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include "BaseInclude.h"

#define VRJS_GETSET_POST(ClassName, name, POST) \
  static bool ClassName##_get_##name(JSContext* cx, unsigned argc, JS::Value *vp) { \
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
    JS::RootedObject self(cx, &args.thisv().toObject()); \
    ClassName* item = Get##ClassName(self); \
    args.rval().set(item == NULL || item->name##Val == NULL ? JS::NullValue() : *(item->name##Val)); \
    return true; \
  } \
  \
  static bool ClassName##_set_##name(JSContext* cx, unsigned argc, JS::Value* vp) { \
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
    if (!args[0].isObject()) { \
      JS_ReportError(cx, "Unexpected argument (expected ##name)"); \
      return false; \
    } \
    JS::RootedObject self(cx, &args.thisv().toObject()); \
    ClassName* item = Get##ClassName(self); \
    JS::RootedValue newVal(cx, args[0]); \
    auto* oldVal = item->name##Val; \
    item->name##Val = new JS::Heap<JS::Value>(newVal); \
    delete oldVal; \
    return POST; \
  }

#define VRJS_GETSET(ClassName, name) \
  VRJS_GETSET_POST(ClassName, name, true)

#define VRJS_PROP(ClassName, name) \
  JS_PSGS(#name, ClassName##_get_##name, ClassName##_set_##name, JSPROP_PERMANENT | JSPROP_ENUMERATE)

#define VRJS_MEMBER(ClassName, name, getter) \
  ClassName::name(JSContext* cx) { \
    if (name##Val == NULL) { \
      return NULL; \
    } \
    JS::RootedObject obj(cx, &name##Val->toObject()); \
    return getter(obj); \
  }

void SetMaybeCallback(JSContext* cx, JS::RootedObject* opts, const char* name, JS::Heap<JS::Value>** out);
bool EnsureJSObject(JSContext* cx, JS::MutableHandleValue vp);
bool GetOVRStringVal(JSContext* cx, JS::HandleValue val, OVR::String* out);
bool GetOVRString(JSContext* cx, JS::HandleString s, OVR::String* out);

#endif