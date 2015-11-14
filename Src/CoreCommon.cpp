#include "CoreCommon.h"

void SetMaybeCallback(JSContext* cx, JS::RootedObject* opts, const char* name, JS::Heap<JS::Value>** out) {
  JS::RootedValue callbackVal(cx);
  if (!JS_GetProperty(cx, *opts, name, &callbackVal) || callbackVal.isNullOrUndefined()) {
    callbackVal = JS::RootedValue(cx, JS::NullValue());
  }
  *out = new JS::Heap<JS::Value>(callbackVal);
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

bool ValueDefined(JS::Heap<JS::Value>* val) {
  return val != NULL && !val->isNullOrUndefined();
}

void TraceHeap(JSTracer* tracer, JS::Heap<JS::Value>* val, const char* parentName, const char* name) {
  if (val != NULL) {
    __android_log_print(ANDROID_LOG_DEBUG, LOG_COMPONENT, " Trace: %s %s\n", parentName, name);
    JS_CallValueTracer(tracer, val, name);
    __android_log_print(ANDROID_LOG_DEBUG, LOG_COMPONENT, " Traced: %s %s\n", parentName, name);
  }
}