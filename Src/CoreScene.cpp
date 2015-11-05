#include "CoreScene.h"


CoreScene::CoreScene(void) {
  lastCollisionTick = 0;
  collisionConfiguration = new btDefaultCollisionConfiguration();
  dispatcher = new  btCollisionDispatcher(collisionConfiguration);
  overlappingPairCache = new btDbvtBroadphase();
  solver = new btSequentialImpulseConstraintSolver;
  dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache,
    solver, collisionConfiguration);
  backgroundVal = NULL;
  globe = OVR::BuildGlobe();
  cubeProgram = OVR::BuildProgram(
    "uniform mat4 Mvpm;\n"
    "attribute vec4 Position;\n"
    "uniform mediump vec4 UniformColor;\n"
    "varying  lowp vec4 oColor;\n"
    "varying highp vec3 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = Mvpm * Position;\n"
    " oTexCoord = Position.xyz;\n"
    "   oColor = UniformColor;\n"
    "}\n"
    ,
    "uniform samplerCube Texture0;\n"
    "varying highp vec3 oTexCoord;\n"
    "varying lowp vec4  oColor;\n"
    "void main()\n"
    "{\n"
    " gl_FragColor = oColor * textureCube( Texture0, oTexCoord );\n"
    "}\n"
  );
  panoProgram = OVR::BuildProgram(
    "uniform mat4 Mvpm;\n"
    "attribute vec4 Position;\n"
    "attribute vec4 VertexColor;\n"
    "attribute vec2 TexCoord;\n"
    "uniform mediump vec4 UniformColor;\n"
    "varying  lowp vec4 oColor;\n"
    "varying highp vec2 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = Mvpm * Position;\n"
    " oTexCoord = TexCoord;\n"
    "   oColor = /* VertexColor * */ UniformColor;\n"
    "}\n"
    ,
    "uniform sampler2D Texture0;\n"
    "varying highp vec2 oTexCoord;\n"
    "varying lowp vec4  oColor;\n"
    "void main()\n"
    "{\n"
    " gl_FragColor = oColor * texture2D( Texture0, oTexCoord );\n"
    "}\n"
  );
  //dynamicsWorld->setGravity(btVector3(0,-10,0)); // TODO: Remove me
}

CoreScene::~CoreScene(void) {
  delete clearColorVal;
  delete backgroundVal;
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

void CoreScene::CallGazeCallbacks(JSContext* cx, OVR::OvrGuiSys* guiSys, OVR::Vector3f* viewPos, OVR::Vector3f* viewFwd, const OVR::VrFrame& vrFrame, JS::HandleValue ev) {
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->CallGazeCallbacks(cx, guiSys, viewPos, viewFwd, vrFrame, ev);
  }
}

void CoreScene::PerformCollisionDetection(JSContext* cx, double now, JS::HandleValue ev) {
  // Update transforms
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->UpdateCollisionObjects(cx);
  }

  // Advance the simulation
  btScalar tickTime = btScalar(1.0) / btScalar(60.0);
  if (lastCollisionTick == 0) {
    dynamicsWorld->stepSimulation(tickTime, 3, tickTime);
  } else {
    dynamicsWorld->stepSimulation(now - lastCollisionTick, 3, tickTime);
  }
  lastCollisionTick = now;

  // Check for collisions
  int numManifolds = dispatcher->getNumManifolds();
  for (int i = 0; i < numManifolds; ++i) {
    btPersistentManifold* contactManifold = dispatcher->getManifoldByIndexInternal(i);
    const btCollisionObject* obA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
    const btCollisionObject* obB = static_cast<const btCollisionObject*>(contactManifold->getBody1());
    CoreModel* modelA = ModelById(cx, obA->getUserIndex());
    CoreModel* modelB = ModelById(cx, obB->getUserIndex());
    modelA->CollidedWith(cx, modelB, ev);
    modelB->CollidedWith(cx, modelA, ev);
  }

  // Finish collision detection
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->FinishCollisions(cx, ev);
  }
}

void CoreScene::DrawEyeView(JSContext* cx, OVR::OvrGuiSys* guiSys, const int eye, const OVR::Matrix4f& eyeViewMatrix, const OVR::Matrix4f& eyeProjectionMatrix, const OVR::Matrix4f& eyeViewProjection, ovrFrameParms& frameParms) {
  JS::RootedObject rootedClearColorVal(cx, &clearColorVal->toObject());
  OVR::Vector4f* clearClr = GetVector4f(rootedClearColorVal);
  glClearColor(clearClr->x, clearClr->y, clearClr->z, clearClr->w);
  glClear(GL_COLOR_BUFFER_BIT);

  // Render the background texture if it exists
  if (backgroundVal != NULL && !backgroundVal->isNullOrUndefined() && backgroundVal->isObject()) {
    JS::RootedObject bkgObj(cx, &backgroundVal->toObject());
    CoreTexture* tex = GetCoreTexture(bkgObj);
    if (tex != NULL) {
      // Set which texture we're working on
      glActiveTexture(GL_TEXTURE0);

      // Bind the texture
      glBindTexture(tex->texture.target, tex->texture.texture);

      // enable sRGB if we've got it
      if (HasEXT_sRGB_texture_decode) {
        glTexParameteri(tex->texture.target, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT);
      }

      // Choose the correct program based on whether the texture is a cubemap
      OVR::GlProgram prog = tex->cube ? cubeProgram : panoProgram;

      // Use the program (at 100% brightness)
      glUseProgram(prog.program);
      glUniform4f(prog.uColor, 1.0f, 1.0f, 1.0f, 1.0f);

      // Pass in our eye view projection matrix
      glUniformMatrix4fv(prog.uMvp, 1, GL_TRUE, eyeViewProjection.M[0]);

      // Draw the background texture on the surrounding globe
      globe.Draw();

      // Unbind the texture
      glBindTexture(tex->texture.target, 0);

      // Configure frame parms (mostly cargo cult from OVR example program)
      frameParms.WarpOptions = 0; // srgb
      frameParms.WarpProgram = VRAPI_FRAME_PROGRAM_SIMPLE;
      frameParms.LayerCount = 1;
      frameParms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].SrcBlend = VRAPI_FRAME_LAYER_BLEND_ONE;
      frameParms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].DstBlend = VRAPI_FRAME_LAYER_BLEND_ZERO;
      frameParms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].WriteAlpha = false;
      frameParms.Layers[VRAPI_FRAME_LAYER_TYPE_OVERLAY].Textures[eye].ColorTextureSwapChain = NULL;
      for (int i = 0; i < 4; ++i) {
        frameParms.ProgramParms[i] = 1.0f;
      }
      OVR::GL_CheckErrors("draw");
    }
  }

  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->DrawEyeView(cx, guiSys, eye, eyeViewMatrix, eyeProjectionMatrix, eyeViewProjection, frameParms);
  }

  glBindVertexArray(0);
  glUseProgram(0);
}

// TODO: (PERF) Make this a hash lookup rather than a recursive search
CoreModel* CoreScene::ModelById(JSContext* cx, int id) {
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    CoreModel* model = child->ModelById(cx, id);
    if (model != NULL) {
      return model;
    }
  }
  return NULL;
}

OVR::Vector4f* VRJS_MEMBER(CoreScene, clearColor, GetVector4f);
//CoreTexture* VRJS_MEMBER(CoreScene, background, GetCoreTexture);

VRJS_GETSET(CoreScene, clearColor);
VRJS_GETSET(CoreScene, background);

static JSPropertySpec CoreScene_props[] = {
  VRJS_PROP(CoreScene, clearColor),
  VRJS_PROP(CoreScene, background),
  JS_PS_END
};

static JSClass coreSceneClass = {
  "Scene",                /* name */
  JSCLASS_HAS_PRIVATE,    /* flags */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  CoreScene_finalize,
  NULL,
  NULL,
  NULL,
  CoreScene_trace
};

JSObject* NewCoreScene(JSContext* cx, CoreScene* scene) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreSceneClass));  
  JS_SetPrivate(self, (void *)scene);

  if (!JS_DefineProperties(cx, self, CoreScene_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on scene\n");
  }

  // Set a white clear color by default
  JS::RootedValue clearColor(cx, JS::ObjectOrNullValue(
    NewCoreVector4f(cx, new OVR::Vector4f(1, 1, 1, 1))));
  scene->clearColorVal = new JS::Heap<JS::Value>(clearColor);
  
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

bool CoreScene_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  JS::RootedObject self(cx, NewCoreScene(cx, new CoreScene())); 

  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

bool CoreScene_add(JSContext* cx, unsigned argc, JS::Value *vp) {
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

  JS::RootedObject modelObj(cx, &args[0].toObject());
  CoreModel* model = GetCoreModel(modelObj);

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  CoreScene* scene = (CoreScene*)JS_GetPrivate(thisObj);

  // Annotate the scene object on the model
  model->scene = scene;

  // Make sure collision detection is set up and configured
  model->StartCollisions(cx);

  scene->children.PushBack(JS::Heap<JS::Value>(args[0]));

  return true;
}

bool CoreScene_remove(JSContext* cx, unsigned argc, JS::Value *vp) {
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

  JS::RootedObject modelObj(cx, &args[0].toObject());
  CoreModel* model = GetCoreModel(modelObj);

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  CoreScene* scene = (CoreScene*)JS_GetPrivate(thisObj);

  // Remove the scene object from the model
  model->scene = nullptr;

  // Make sure collision detection is stopped
  model->StopCollisions();

  if (!scene->RemoveModel(cx, model)) {
    JS_ReportError(cx, "Could not find model to remove");
    return false;
  }

  return true;
}

bool CoreScene_setClearColor(JSContext* cx, unsigned argc, JS::Value *vp) {
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

  scene->clearColorVal = new JS::Heap<JS::Value>(args[0]);

  return true;
}

bool Core_print(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }

  // Coerce whatever was passed in to a string
  JS::RootedString s(cx, JS::ToString(cx, args[0]));

  OVR::String str;
  if (!GetOVRString(cx, s, &str)) {
    return false;
  }

  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "PRINT: %s\n", str.ToCStr());
  return true;
}

void CoreScene_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreScene* scene = (CoreScene*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete scene;
}

void CoreScene_trace(JSTracer *tracer, JSObject *obj) {
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Tracing scene\n");
  CoreScene* scene = (CoreScene*)JS_GetPrivate(obj);
  if (scene != NULL) {
    TraceHeap(tracer, scene->clearColorVal, "scene", "clearColorVal");
    TraceHeap(tracer, scene->backgroundVal, "scene", "backgroundVal");
    for (int i = 0; i < scene->children.GetSizeI(); ++i) {
      char buffer[50];
      sprintf(buffer, "child%d", i);
      TraceHeap(tracer, &scene->children[i], "scene", buffer);
    }
  }
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Done tracing scene\n");
}

CoreScene* SetupCoreScene(JSContext* cx, JS::RootedObject* global, JS::RootedObject* core, JS::RootedObject* env) {
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