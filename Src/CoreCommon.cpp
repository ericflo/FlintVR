#include "CoreCommon.h"

void SetMaybeValue(JSContext* cx, JS::MutableHandleValue vp, mozilla::Maybe<JS::PersistentRootedValue>& out) {
  out.reset();
  out.emplace(cx, vp);
}

void SetMaybeCallback(JSContext* cx, JS::RootedObject* opts, const char* name, JS::RootedObject* self, mozilla::Maybe<JS::PersistentRootedValue>& out) {
  JS::RootedValue callbackVal(cx);
  if (!JS_GetProperty(cx, *opts, name, &callbackVal) || callbackVal.isNullOrUndefined()) {
    callbackVal = JS::RootedValue(cx, JS::NullValue());
  }
  if (callbackVal.isNullOrUndefined()) {
    out.reset();
  }
  SetMaybeValue(cx, &callbackVal, out);
}

bool EnsureJSObject(JSContext* cx, JS::MutableHandleValue vp) {
  if (!vp.isObject()) {
    JS_ReportError(cx, "Unexpected argument (expected object)");
    return false;
  }
  return true;
}

bool GetOVRStringVal(JSContext* cx, JS::HandleValue val, OVR::String* out) {
  if (!val.isString()) {
    JS_ReportError(cx, "Expected a string");
    return false;
  }
  JS::RootedString str(cx, val.toString());
  bool resp = GetOVRString(cx, str, out);
  return resp;
}

bool GetOVRString(JSContext* cx, JS::HandleString s, OVR::String* out) {
  // Fill a buffer with the string data
  size_t sLen = JS_GetStringEncodingLength(cx, s) * sizeof(char);
  char* sBuf = new char[sLen+1];
  size_t sCopiedLen = JS_EncodeStringToBuffer(cx, s, sBuf, sLen);
  if (sCopiedLen != sLen) {
    delete[] sBuf;
    JS_ReportError(cx, "Could not encode output");
    return false;
  }
  sBuf[sLen] = '\0';

  // Create the string to return from the buffer
  // TODO: See whether we can avoid this extra string copy
  *out = OVR::String(sBuf);

  // Make sure we free the buffer before returning our string
  delete[] sBuf;
  
  return true;
}