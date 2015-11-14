#include "CoreTexture.h"


CoreTexture::CoreTexture(
  JSContext* cx,
  JS::Heap<JS::Value>* _path,
  int _width,
  int _height,
  bool _cube) :
    width(_width),
    height(_height),
    cube(_cube) {
  path = _path;
  Rebuild(cx);
}

CoreTexture::CoreTexture(
  OVR::GlTexture _tex,
  int _width,
  int _height) :
    width(_width),
    height(_height),
    cube(false),
    texture(_tex) {
  path = NULL;
}

CoreTexture::~CoreTexture(void) {
  OVR::FreeTexture(texture);
  delete path;
}

bool CoreTexture::RebuildCubemap(JSContext* cx, OVR::String pathStr) {
  OVR::FreeTexture(texture);

  if (pathStr.IsEmpty()) {
    return true;
  }

  // Get the file extension, and a copy of the path with no extension
  OVR::String ext = pathStr.GetExtension();
  OVR::String noExt(pathStr);
  noExt.StripExtension();

  // Create some membuffers for the files we're going to open
  OVR::MemBufferFile mbfs[6] = { 
    OVR::MemBufferFile(OVR::MemBufferFile::NoInit), 
    OVR::MemBufferFile(OVR::MemBufferFile::NoInit),
    OVR::MemBufferFile(OVR::MemBufferFile::NoInit),
    OVR::MemBufferFile(OVR::MemBufferFile::NoInit),
    OVR::MemBufferFile(OVR::MemBufferFile::NoInit),
    OVR::MemBufferFile(OVR::MemBufferFile::NoInit) 
  };
  
  // Load all of them up
  const char* const cubeSuffix[6] = {"_px", "_nx", "_py", "_ny", "_pz", "_nz"};
  for (int side = 0; side < 6; ++side) {
    OVR::String sidePath = noExt + OVR::String(cubeSuffix[side]) + ext;
    // Get it from the package for now
    // TODO: Pull from somewhere else?
    if (!OVR::ovr_ReadFileFromApplicationPackage(sidePath.ToCStr(), mbfs[side])) {
      return false;
    }
  }

  unsigned char* data[6];
  int comp, imgWidth, imgHeight;
  // For each side of the cube
  for (int i = 0; i < 6; ++i) {
    // Load the image
    data[i] = (unsigned char *)stbi_load_from_memory(
      (unsigned char *)mbfs[i].Buffer, mbfs[i].Length, &imgWidth, &imgHeight, &comp, 4);

    // Sanity check image dimensions
    if (imgWidth != width) {
      JS_ReportError(cx, "Cubemap has mismatched image width");
      return false;
    }
    if (imgHeight != height) {
      JS_ReportError(cx, "Cubemap has mismatched image height");
      return false;
    }
    if (imgWidth <= 0 || imgWidth > 32768 || imgHeight <= 0 || imgHeight > 32768) {
      JS_ReportError(cx, "Invalid texture size");
      return false;
    }
  }

  GLenum glFormat;
  GLenum glInternalFormat;
  if (!TextureFormatToGlFormat(OVR::Texture_RGBA, true, glFormat, glInternalFormat)) {
    JS_ReportError(cx, "Invalid texture format OVR::Texture_RGBA");
    return false;
  }

  GLuint texId;
  glGenTextures(1, &texId);
  glBindTexture(GL_TEXTURE_CUBE_MAP, texId);

  // Get the total size (GetOvrTextureSize(OVR::Texture_RGBA, width, height) * 6)
  // size_t totalSize = (((width + 3) / 4) * ((height + 3) / 4) * 8) * 6;

  for (int i = 0; i < 6; ++i) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glInternalFormat, width, height, 0, glFormat, GL_UNSIGNED_BYTE, data[i]);
  }

  // Generate mipmaps and bind the texture
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  // Construct our actual texture object
  texture = OVR::GlTexture(texId, GL_TEXTURE_CUBE_MAP);

  // Free our image data
  for (int i = 0; i < 6; ++i) {
    free(data[i]);
  }

  // Wait for the upload to complete.
  glFinish();
  return true;
}

bool CoreTexture::RebuildTexture(JSContext* cx, OVR::String pathStr) {
  OVR::FreeTexture(texture);

  if (pathStr.IsEmpty()) {
    return true;
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

  BuildTextureMipmaps(texture); // Optional? Also does this happen in LoadTextureFromBuffer?

  // TODO: MakeTextureClamped MakeTextureLodClamped MakeTextureTrilinear
  //       MakeTextureLinear, MakeTextureAniso

  return true;
}

bool CoreTexture::Rebuild(JSContext* cx) {
  // First, free any texture we already have (no-op if it's a 0-texture)
  OVR::FreeTexture(texture);

  // Get the path string
  OVR::String pathStr;
  JS::RootedValue pathVal(cx, *path);
  if (!GetOVRStringVal(cx, pathVal, &pathStr)) {
    return false;
  }

  // If it's a cube map
  if (cube) {
    return RebuildCubemap(cx, pathStr);
  } else {
    return RebuildTexture(cx, pathStr);
  }

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
  JS::RootedValue val(cx, *item->path);
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
  JS::Heap<JS::Value>* oldPath = item->path;
  item->path = new JS::Heap<JS::Value>(args[0]);
  delete oldPath;
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

static bool CoreTexture_get_cube(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreTexture* item = GetCoreTexture(self);
  args.rval().setBoolean(item->cube);
  return true;
}

static bool CoreTexture_set_cube(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isBoolean()) {
    JS_ReportError(cx, "Invalid cube boolean specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreTexture* item = GetCoreTexture(self);
  item->cube = args[0].toBoolean();
  item->Rebuild(cx);
  return true;
}

static JSPropertySpec CoreTexture_props[] = {
  VRJS_PROP(CoreTexture, path),
  VRJS_PROP(CoreTexture, width),
  VRJS_PROP(CoreTexture, height),
  VRJS_PROP(CoreTexture, cube),
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

  // Cube
  JS::RootedValue cubeVal(cx);
  if (!JS_GetProperty(cx, opts, "cube", &cubeVal) || cubeVal.isNullOrUndefined() || !cubeVal.isBoolean()) {
    cubeVal = JS::RootedValue(cx, JS::FalseValue());
  }

  // Create our self object
  CoreTexture* tex = new CoreTexture(cx, new JS::Heap<JS::Value>(pathVal),
                                     widthVal.toInt32(), heightVal.toInt32(),
                                     cubeVal.toBoolean());
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
  __android_log_print(ANDROID_LOG_DEBUG, LOG_COMPONENT, "Tracing texture\n");
  CoreTexture* tex = (CoreTexture*)JS_GetPrivate(obj);
  if (tex != NULL) {
    TraceHeap(tracer, tex->path, "texture", "pathVal");
  }
  __android_log_print(ANDROID_LOG_DEBUG, LOG_COMPONENT, "Finished tracing texture\n");
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