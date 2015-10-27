#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include "BaseInclude.h"

#define VRJS_GETSET(ClassName, name) \
  static bool ClassName##_get_##name(JSContext* cx, unsigned argc, JS::Value *vp) { \
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
    JS::RootedObject self(cx, &args.thisv().toObject()); \
    ClassName* model = Get##ClassName(self); \
    args.rval().set(model == NULL || model->name##Val.isNothing() ? JS::NullValue() : model->name##Val.ref()); \
    return true; \
  } \
  \
  static bool ClassName##_set_##name(JSContext* cx, unsigned argc, JS::Value *vp) { \
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
    if (!args[0].isObject()) { \
      JS_ReportError(cx, "Unexpected argument (expected ##name)"); \
      return false; \
    } \
    JS::RootedObject self(cx, &args.thisv().toObject()); \
    ClassName* model = Get##ClassName(self); \
    mozilla::Maybe<JS::PersistentRootedValue>* val = &model->name##Val; \
    val->reset(); \
    val->emplace(cx, *vp); \
    return true; \
  }

#define VRJS_PROP(ClassName, name) \
  JS_PSGS(#name, ClassName##_get_##name, ClassName##_set_##name, JSPROP_PERMANENT | JSPROP_ENUMERATE)

#define VRJS_MEMBER(ClassName, name, getter) \
  ClassName::name(JSContext* cx) { \
    if (name##Val.isNothing()) { \
      return NULL; \
    } \
    JS::RootedObject obj(cx, &name##Val.ref().toObject()); \
    return getter(obj); \
  }

void SetMaybeValue(JSContext* cx, JS::MutableHandleValue vp, mozilla::Maybe<JS::PersistentRootedValue>& out);
void SetMaybeCallback(JSContext* cx, JS::RootedObject* opts, const char* name, JS::RootedObject* self, mozilla::Maybe<JS::PersistentRootedValue>& out);
bool EnsureJSObject(JSContext* cx, JS::MutableHandleValue vp);
bool GetOVRStringVal(JSContext* cx, JS::HandleValue val, OVR::String* out);
bool GetOVRString(JSContext* cx, JS::HandleString s, OVR::String* out);

#endif