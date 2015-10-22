#ifndef CORE_SCENE_H
#define CORE_SCENE_H

#include "BaseInclude.h"
#include "CoreModel.h"
#include "SceneGraph.h"
#include "CoreVector4f.h"
#include "Kernel/OVR_Std.h"

class CoreScene {
public:
  SceneGraph* graph;

  // Would it make sense to wrap these all in an object?
  btDefaultCollisionConfiguration* collisionConfiguration;
  btCollisionDispatcher* dispatcher;
  btBroadphaseInterface* overlappingPairCache;
  btSequentialImpulseConstraintSolver* solver;
  btDiscreteDynamicsWorld* dynamicsWorld;

  OVR::Vector4f* clearColor(JSContext *cx);
  mozilla::Maybe<JS::PersistentRootedValue> clearColorVal;

  CoreScene();
  ~CoreScene();
};

CoreScene* SetupCoreScene(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core, JS::RootedObject *env);
JSObject* NewCoreScene(JSContext *cx, CoreScene* scene);
CoreScene* GetCoreScene(JS::HandleObject obj);

bool CoreScene_add(JSContext *cx, unsigned argc, JS::Value *vp);
bool CoreScene_remove(JSContext *cx, unsigned argc, JS::Value *vp);
bool CoreScene_setClearColor(JSContext *cx, unsigned argc, JS::Value *vp);
bool Core_print(JSContext *cx, unsigned argc, JS::Value *vp); // TODO: Move this somewhere else

#endif
