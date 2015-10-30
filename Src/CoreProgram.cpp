#include "CoreProgram.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

CoreProgram::CoreProgram(void) {
  program = NULL;
}

CoreProgram::~CoreProgram(void) {
  if (program != NULL) {
    OVR::DeleteProgram(*program);
  }
  delete program;
  delete vertexVal;
  delete fragmentVal;
}

bool CoreProgram::Rebuild(JSContext* cx) {
  JS::RootedValue rootedVertexVal(cx, *vertexVal);
  OVR::String vertexStr;
  if (!GetOVRStringVal(cx, rootedVertexVal, &vertexStr)) {
    return false;
  }
  JS::RootedValue rootedFragmentVal(cx, *fragmentVal);
  OVR::String fragmentStr;
  if (!GetOVRStringVal(cx, rootedFragmentVal, &fragmentStr)) {
    return false;
  }

  OVR::GlProgram prog = OVR::BuildProgram(vertexStr.ToCStr(), fragmentStr.ToCStr(), false);
  if (prog.program == 0) {
    JS_ReportError(cx, "Could not compile shader");
    return false;
  }

  OVR::GlProgram* heapProgram = new OVR::GlProgram();
  heapProgram->program = prog.program;
  heapProgram->vertexShader = prog.vertexShader;
  heapProgram->uMvp = prog.uMvp;
  heapProgram->uModel = prog.uModel;
  heapProgram->uView = prog.uView;
  heapProgram->uProjection = prog.uProjection;
  heapProgram->uColor = prog.uColor;
  heapProgram->uFadeDirection = prog.uFadeDirection;
  heapProgram->uTexm = prog.uTexm;
  heapProgram->uTexm2 = prog.uTexm2;
  heapProgram->uJoints = prog.uJoints;
  heapProgram->uColorTableOffset = prog.uColorTableOffset;

  OVR::GlProgram* oldProgram = program;
  program = heapProgram;
  delete oldProgram;

  return true;
}

static JSClass coreProgramClass = {
  "Program",              /* name */
  JSCLASS_HAS_PRIVATE    /* flags */
};

VRJS_GETSET_POST(CoreProgram, vertex, item->Rebuild(cx))
VRJS_GETSET_POST(CoreProgram, fragment, item->Rebuild(cx))

static JSPropertySpec CoreProgram_props[] = {
  VRJS_PROP(CoreProgram, vertex),
  VRJS_PROP(CoreProgram, fragment),
  JS_PS_END
};

bool CoreProgram_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() < 2) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 2);
    return false;
  }

  // Check to make sure the args are strings
  if (!args[0].isString() || !args[1].isString()) {
    JS_ReportError(cx, "Vertex and fragment shaders must be strings");
    return false;
  }

  CoreProgram* prog = new CoreProgram();
  JS::RootedObject self(cx, NewCoreProgram(cx, prog));
  prog->vertexVal = new JS::Heap<JS::Value>(args[0]);
  prog->fragmentVal = new JS::Heap<JS::Value>(args[1]);
  if (!prog->Rebuild(cx)) {
    JS_ReportError(cx, "Could not build program");
    delete prog;
    return false;
  }

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreProgram_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreProgram* prog = (CoreProgram*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete prog;
}

void CoreProgram_trace(JSTracer *tracer, JSObject *obj) {
  CoreProgram* program = (CoreProgram*)JS_GetPrivate(obj);
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Tracing program\n");
  JS_CallValueTracer(tracer, program->vertexVal, "vertexVal");
  JS_CallValueTracer(tracer, program->fragmentVal, "fragmentVal");
}

void SetupCoreProgram(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core) {
  coreProgramClass.finalize = CoreProgram_finalize;
  coreProgramClass.trace = CoreProgram_trace;
  JSObject *obj = JS_InitClass(
      cx,
      *core,
      *global,
      &coreProgramClass,
      CoreProgram_constructor,
      2,
      nullptr, /* Properties */
      nullptr, /* Methods */
      nullptr, /* Static Props */
      nullptr  /* Static Methods */);
  if (!obj) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Program class\n");
    return;
  }
}

JSObject* NewCoreProgram(JSContext* cx, CoreProgram* prog) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreProgramClass));
  if (!JS_DefineProperties(cx, self, CoreProgram_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on program\n");
  }
  JS_SetPrivate(self, (void *)prog);
  return self;
}

CoreProgram* GetCoreProgram(JS::HandleObject obj) {
  return (CoreProgram*)JS_GetPrivate(obj);
}