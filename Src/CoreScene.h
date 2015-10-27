#ifndef CORE_SCENE_H
#define CORE_SCENE_H

#include "BaseInclude.h"
#include "CoreModel.h"
#include "CoreVector4f.h"
#include "Kernel/OVR_Std.h"

class CoreScene {
public:
  OVR::Array<JS::PersistentRootedValue> children;

  // Would it make sense to wrap these all in an object?
  btDefaultCollisionConfiguration* collisionConfiguration;
  btCollisionDispatcher* dispatcher;
  btBroadphaseInterface* overlappingPairCache;
  btSequentialImpulseConstraintSolver* solver;
  btDiscreteDynamicsWorld* dynamicsWorld;

  OVR::Vector4f* clearColor(JSContext* cx);
  mozilla::Maybe<JS::PersistentRootedValue> clearColorVal;

  CoreScene();
  ~CoreScene();
  bool RemoveModel(JSContext* cx, CoreModel* model);
  void ComputeMatrices(JSContext* cx);
  void CallFrameCallbacks(JSContext* cx, JS::HandleValue ev);
  void CallGazeCallbacks(JSContext* cx, OVR::Vector3f* viewPos, OVR::Vector3f* viewFwd, const OVR::VrFrame& vrFrame, JS::HandleValue ev);
  void PerformCollisionDetection(JSContext* cx, double now, JS::HandleValue ev);
  void DrawEyeView(JSContext* cx, const int eye, const OVR::Matrix4f& eyeViewMatrix, const OVR::Matrix4f& eyeProjectionMatrix, const OVR::Matrix4f& eyeViewProjection, ovrFrameParms& frameParms);
  CoreModel* ModelById(JSContext* cx, int id);
private:
  double lastCollisionTick;
};

CoreScene* SetupCoreScene(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core, JS::RootedObject *env);
JSObject* NewCoreScene(JSContext* cx, CoreScene* scene);
CoreScene* GetCoreScene(JS::HandleObject obj);

bool CoreScene_add(JSContext* cx, unsigned argc, JS::Value *vp);
bool CoreScene_remove(JSContext* cx, unsigned argc, JS::Value *vp);
bool CoreScene_setClearColor(JSContext* cx, unsigned argc, JS::Value *vp);
bool Core_print(JSContext* cx, unsigned argc, JS::Value *vp); // TODO: Move this somewhere else

#endif
