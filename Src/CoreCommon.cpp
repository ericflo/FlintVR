#include "CoreCommon.h"

void SetMaybeValue(JSContext *cx, JS::MutableHandleValue vp, mozilla::Maybe<JS::PersistentRootedValue>& out) {
  out.reset();
  out.emplace(cx, vp);
}

void SetMaybeCallback(JSContext *cx, JS::RootedObject* opts, const char* name, JS::RootedObject* self, mozilla::Maybe<JS::PersistentRootedValue>& out) {
  JS::RootedValue callbackVal(cx);
  if (!JS_GetProperty(cx, *opts, name, &callbackVal) || callbackVal.isNullOrUndefined()) {
    callbackVal = JS::RootedValue(cx, JS::NullValue());
  }
  if (callbackVal.isNullOrUndefined()) {
    out.reset();
  }
  SetMaybeValue(cx, &callbackVal, out);
}

bool EnsureJSObject(JSContext *cx, JS::MutableHandleValue vp) {
  if (!vp.isObject()) {
    JS_ReportError(cx, "Unexpected argument (expected object)");
    return false;
  }
  return true;
}