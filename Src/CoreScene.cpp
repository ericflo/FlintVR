#include "CoreScene.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

CoreScene::CoreScene(void) : children() {
  collisionConfiguration = new btDefaultCollisionConfiguration();
  dispatcher = new  btCollisionDispatcher(collisionConfiguration);
  overlappingPairCache = new btDbvtBroadphase();
  solver = new btSequentialImpulseConstraintSolver;
  dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache,
    solver, collisionConfiguration);
}

CoreScene::~CoreScene(void) {
  clearColorVal.reset();
  delete dynamicsWorld;
  delete solver;
  delete overlappingPairCache;
  delete dispatcher;
  delete collisionConfiguration;
}

bool CoreScene::RemoveModel(JSContext* cx, CoreModel* model) {
  // Find the index of the model to remove
  int idx = -1;
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    if (child->id == model->id) {
      idx = i;
      break;
    }
  }

  // If we found it, hooray
  if (idx != -1) {
    children.RemoveAt(idx);
    return true;
  }

  // Otherwise, recurse
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    if (child->RemoveModel(cx, model)) {
      return true;
    }
  }

  return false;
}

void CoreScene::ComputeMatrices(JSContext* cx) {
  OVR::Matrix4f transform;
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->ComputeMatrices(cx, transform);
  }
}

void CoreScene::CallFrameCallbacks(JSContext* cx, JS::HandleValue ev) {
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->CallFrameCallbacks(cx, ev);
  }
}

void CoreScene::CallGazeCallbacks(JSContext* cx,  OVR::Vector3f* viewPos, OVR::Vector3f* viewFwd, const OVR::VrFrame& vrFrame, JS::HandleValue ev) {
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->CallGazeCallbacks(cx, viewPos, viewFwd, vrFrame, ev);
  }
}

void CoreScene::DrawEyeView(JSContext* cx, const int eye, const OVR::Matrix4f& eyeViewMatrix, const OVR::Matrix4f& eyeProjectionMatrix, const OVR::Matrix4f& eyeViewProjection, ovrFrameParms& frameParms) {
  OVR::Vector4f* clearClr = clearColor(cx);
  
  glClearColor(clearClr->x, clearClr->y, clearClr->z, clearClr->w);
  glClear(GL_COLOR_BUFFER_BIT);

  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->DrawEyeView(cx, eye, eyeViewMatrix, eyeProjectionMatrix, eyeViewProjection, frameParms);
  }

  glBindVertexArray(0);
  glUseProgram(0);
}

OVR::Vector4f* VRJS_MEMBER(CoreScene, clearColor, GetVector4f);

VRJS_GETSET(CoreScene, clearColor)

static JSPropertySpec CoreScene_props[] = {
  VRJS_PROP(CoreScene, clearColor),
  JS_PS_END
};

static JSClass coreSceneClass = {
  "Scene",                /* name */
  JSCLASS_HAS_PRIVATE    /* flags */
};

JSObject* NewCoreScene(JSContext *cx, CoreScene* scene) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreSceneClass));
  
  JS_SetPrivate(self, (void *)scene);

  if (!JS_DefineProperties(cx, self, CoreScene_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on scene\n");
  }
  
  if (!JS_DefineFunction(cx, self, "add", &CoreScene_add, 0, 0)) {
    JS_ReportError(cx, "Could not create scene.add function");
    return NULL;
  }
  if (!JS_DefineFunction(cx, self, "remove", &CoreScene_remove, 0, 0)) {
    JS_ReportError(cx, "Could not create scene.remove function");
    return NULL;
  }
  if (!JS_DefineFunction(cx, self, "setClearColor", &CoreScene_setClearColor, 0, 0)) {
    JS_ReportError(cx, "Could not create scene.setClearColor function");
    return NULL;
  }

  JS::RootedObject objects(cx, JS_NewArrayObject(cx, 0));
  JS::RootedValue objectsVal(cx, JS::ObjectOrNullValue(objects));
  if (!JS_SetProperty(cx, self, "objects", objectsVal)) {
    JS_ReportError(cx, "Could not set scene.objects");
    return NULL;
  }

  return self;
}

CoreScene* GetCoreScene(JS::HandleObject obj) {
  CoreScene* scene = (CoreScene*)JS_GetPrivate(obj);
  return scene;
}

bool CoreScene_constructor(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject self(cx, NewCoreScene(cx, new CoreScene())); 

  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreScene_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreScene* scene = (CoreScene*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete scene;
}

bool CoreScene_add(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Expected add to take a model argument");
    return false;
  }

  JS::PersistentRootedValue modelVal(cx, args[0]);

  // Read the model object in
  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  CoreScene* scene = (CoreScene*)JS_GetPrivate(thisObj);
  scene->children.PushBack(modelVal);

  return true;
}

bool CoreScene_remove(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Expected remove to take a model argument");
    return false;
  }

  // Read the model object in
  JS::RootedObject modelObj(cx, &args[0].toObject());
  CoreModel* model = GetCoreModel(modelObj);
  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  CoreScene* scene = (CoreScene*)JS_GetPrivate(thisObj);

  if (!scene->RemoveModel(cx, model)) {
    JS_ReportError(cx, "Could not find model to remove");
    return false;
  }

  return true;
}

bool CoreScene_setClearColor(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Expected add to take a geometry argument");
    return false;
  }

  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreScene* scene = GetCoreScene(self);

  SetMaybeValue(cx, args[0], scene->clearColorVal);

  return true;
}

bool Core_print(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  
  // Convert the argument to a string
  JS::RootedString s(cx, JS::ToString(cx, args[0]));

  size_t sLen = JS_GetStringEncodingLength(cx, s) * sizeof(char);
  char* sBuf = new char[sLen+1];
  size_t sCopiedLen = JS_EncodeStringToBuffer(cx, s, sBuf, sLen);
  if (sCopiedLen != sLen) {
    delete[] sBuf;
    JS_ReportError(cx, "Could not encode output");
    return false;
  }
  sBuf[sLen] = '\0';

  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "PRINT: %s\n", sBuf);

  delete[] sBuf;

  return true;
}

CoreScene* SetupCoreScene(JSContext* cx, JS::RootedObject* global, JS::RootedObject* core, JS::RootedObject* env) {
  coreSceneClass.finalize = CoreScene_finalize;
  JSObject *obj = JS_InitClass(
      cx,
      *core,
      *global,
      &coreSceneClass,
      CoreScene_constructor,
      0,
      nullptr, /* Properties */
      nullptr, /* Methods */
      nullptr, /* Static Props */
      nullptr  /* Static Methods */);
  if (!obj) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Scene class\n");
    return NULL;
  }

  // TODO: Free this somewhere, it's a singleton now but who knows later
  CoreScene* scene = new CoreScene();
  JS::RootedObject sceneObj(cx, NewCoreScene(cx, scene));
  JS::RootedValue sceneVal(cx, JS::ObjectOrNullValue(sceneObj));
  if (!JS_SetProperty(cx, *env, "scene", sceneVal)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not set env.scene\n");
    JS_ReportError(cx, "Could not set env.scene");
    return NULL;
  }

  if (!JS_DefineFunction(cx, *global, "print", &Core_print, 1, 0)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not create print function\n");
    JS_ReportError(cx, "Could not create print function");
    return NULL;
  }

  return scene;
}
