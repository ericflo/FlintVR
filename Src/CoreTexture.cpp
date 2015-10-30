#include "CoreTexture.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

CoreTexture::CoreTexture(
  JSContext* cx,
  JS::HandleValue _path,
  int _width,
  int _height) :
    path(_path),
    width(_width),
    height(_height) {
  Rebuild(cx);
}

CoreTexture::~CoreTexture(void) {
  OVR::FreeTexture(texture);

}

bool CoreTexture::Rebuild(JSContext* cx) {
  // First, free any texture we already have (no-op if it's a 0-texture)
  OVR::FreeTexture(texture);

  // Get the path string
  OVR::String pathStr;
  JS::RootedValue pathVal(cx, path);
  if (!GetOVRStringVal(cx, pathVal, &pathStr)) {
    return false;
  }

  // Get it from the package for now
  // TODO: Pull from somewhere else?
  OVR::MemBufferFile bufferFile(OVR::MemBufferFile::NoInit);
  if (!OVR::ovr_ReadFileFromApplicationPackage(pathStr.ToCStr(), bufferFile)) {
    JS_ReportError(cx, "Could not read texture from application package");
    return false;
  }

  // Now load it
  texture = OVR::LoadTextureFromBuffer(
    pathStr.ToCStr(),
    bufferFile,
    OVR::TextureFlags_t(OVR::TEXTUREFLAG_NO_DEFAULT), // TODO: Make configurable
    width,
    height
  );

  BuildTextureMipmaps(texture); // Optional?

  // TODO: MakeTextureClamped MakeTextureLodClamped MakeTextureTrilinear
  //       MakeTextureLinear, MakeTextureAniso

  return true;
}

static JSClass coreTextureClass = {
  "Texture",              /* name */
  JSCLASS_HAS_PRIVATE,    /* flags */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  CoreTexture_finalize,
  NULL,
  NULL,
  NULL,
  CoreTexture_trace
};

static bool CoreTexture_get_path(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreTexture* item = GetCoreTexture(self);
  JS::RootedValue val(cx, item->path);
  args.rval().set(val);
  return true;
}

static bool CoreTexture_set_path(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isString()) {
    JS_ReportError(cx, "Invalid path specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreTexture* item = GetCoreTexture(self);
  item->path = JS::Heap<JS::Value>(args[0]);
  item->Rebuild(cx);
  return true;
}

static bool CoreTexture_get_width(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreTexture* item = GetCoreTexture(self);
  args.rval().setInt32(item->width);
  return true;
}

static bool CoreTexture_set_width(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isInt32()) {
    JS_ReportError(cx, "Invalid width specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreTexture* item = GetCoreTexture(self);
  item->width = args[0].toInt32();
  item->Rebuild(cx);
  return true;
}

static bool CoreTexture_get_height(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreTexture* item = GetCoreTexture(self);
  args.rval().setInt32(item->height);
  return true;
}

static bool CoreTexture_set_height(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isInt32()) {
    JS_ReportError(cx, "Invalid height specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreTexture* item = GetCoreTexture(self);
  item->height = args[0].toInt32();
  item->Rebuild(cx);
  return true;
}

static JSPropertySpec CoreTexture_props[] = {
  VRJS_PROP(CoreTexture, path),
  VRJS_PROP(CoreTexture, width),
  VRJS_PROP(CoreTexture, height),
  JS_PS_END
};

JSObject* NewCoreTexture(JSContext* cx, CoreTexture* tex) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreTextureClass));
  if (!JS_DefineProperties(cx, self, CoreTexture_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on Texture\n");
  }
  JS_SetPrivate(self, (void *)tex);
  return self;
}

CoreTexture* GetCoreTexture(JS::HandleObject obj) {
  return (CoreTexture*)JS_GetPrivate(obj);
}

bool CoreTexture_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }

  // Check to make sure the opts argument is an object
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Texture options must be an object");
    return false;
  }

  JS::RootedObject opts(cx, &args[0].toObject());

  // Path
  JS::RootedValue pathVal(cx);
  JS_GetProperty(cx, opts, "path", &pathVal);
  if (!pathVal.isNullOrUndefined() && !pathVal.isString()) {
    JS_ReportError(cx, "Expected path to be a string");
    return false;
  }

  // Width
  JS::RootedValue widthVal(cx);
  if (!JS_GetProperty(cx, opts, "width", &widthVal) || widthVal.isNullOrUndefined()) {
    JS_ReportError(cx, "Texture missing required width property");
    return false;
  }
  if (!widthVal.isInt32()) {
    JS_ReportError(cx, "Texture width must be an integer");
    return false;
  }

  // Height
  JS::RootedValue heightVal(cx);
  if (!JS_GetProperty(cx, opts, "height", &heightVal) || heightVal.isNullOrUndefined()) {
    JS_ReportError(cx, "Texture missing required height property");
    return false;
  }
  if (!heightVal.isInt32()) {
    JS_ReportError(cx, "Texture height must be an integer");
    return false;
  }

  // Create our self object
  CoreTexture* tex = new CoreTexture(cx, pathVal, widthVal.toInt32(), heightVal.toInt32());
  JS::RootedObject self(cx, NewCoreTexture(cx, tex));

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreTexture_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreTexture* tex = (CoreTexture*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete tex;
}

void CoreTexture_trace(JSTracer *tracer, JSObject *obj) {
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Tracing texture\n");
  CoreTexture* tex = (CoreTexture*)JS_GetPrivate(obj);
  JS_CallValueTracer(tracer, &tex->path, "pathVal");
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Finished tracing texture\n");
}

void SetupCoreTexture(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core) {
  JSObject *obj = JS_InitClass(
      cx,
      *core,
      *global,
      &coreTextureClass,
      CoreTexture_constructor,
      1,
      nullptr, /* Properties */
      nullptr, /* Methods */
      nullptr, /* Static Props */
      nullptr  /* Static Methods */);
  if (!obj) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Texture class\n");
    return;
  }
}

const JSClass* CoreTexture_class() {
  return &coreTextureClass;
}