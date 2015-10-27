#ifndef CORE_MODEL_H
#define CORE_MODEL_H

#include "BaseInclude.h"
#include "Kernel/OVR_Geometry.h"
#include "CoreGeometry.h"
#include "CoreProgram.h"
#include "CoreVector3f.h"
#include "CoreMatrix4f.h"

class CoreScene;

class CoreModel {
public:
	int id;
  bool isHovered;
  bool isTouching;
  OVR::Matrix4f localMatrix;
  OVR::Matrix4f worldMatrix;
  OVR::Array<JS::PersistentRootedValue> children;
  OVR::Array<int> collidingWithIds;
  OVR::Array<int> seenCollidingIds;

  btTriangleMesh* triMesh;
  btCollisionShape* collisionShape;
  btCollisionObject* collisionObj;

  mozilla::Maybe<JS::PersistentRootedValue> selfVal;
  CoreScene* scene;

  CoreGeometry* geometry(JSContext* cx);
  OVR::GlProgram* program(JSContext* cx);
  OVR::Matrix4f* matrix(JSContext* cx);
  OVR::Vector3f* position(JSContext* cx);
  OVR::Vector3f* rotation(JSContext* cx);
  OVR::Vector3f* scale(JSContext* cx);
  mozilla::Maybe<JS::PersistentRootedValue> geometryVal;
  mozilla::Maybe<JS::PersistentRootedValue> programVal;
  mozilla::Maybe<JS::PersistentRootedValue> matrixVal;
  mozilla::Maybe<JS::PersistentRootedValue> positionVal;
  mozilla::Maybe<JS::PersistentRootedValue> rotationVal;
  mozilla::Maybe<JS::PersistentRootedValue> scaleVal;
  mozilla::Maybe<JS::PersistentRootedValue> collideTagVal;
  mozilla::Maybe<JS::PersistentRootedValue> collidesWithVal;

  mozilla::Maybe<JS::PersistentRootedValue> onFrameVal;
  mozilla::Maybe<JS::PersistentRootedValue> onGazeHoverOverVal;
  mozilla::Maybe<JS::PersistentRootedValue> onGazeHoverOutVal;
  mozilla::Maybe<JS::PersistentRootedValue> onGestureTouchDownVal;
  mozilla::Maybe<JS::PersistentRootedValue> onGestureTouchUpVal;
  mozilla::Maybe<JS::PersistentRootedValue> onGestureTouchCancelVal;
  mozilla::Maybe<JS::PersistentRootedValue> onCollideStartVal;
  mozilla::Maybe<JS::PersistentRootedValue> onCollideEndVal;

  CoreModel();
  ~CoreModel();
  bool RemoveModel(JSContext* cx, CoreModel* model);
  void ComputeMatrices(JSContext* cx, OVR::Matrix4f& transform);
  void CallFrameCallbacks(JSContext* cx, JS::HandleValue ev);
  void CallGazeCallbacks(JSContext* cx,  OVR::Vector3f* viewPos, OVR::Vector3f* viewFwd, const OVR::VrFrame& vrFrame, JS::HandleValue ev);
  void DrawEyeView(JSContext* cx, const int eye, const OVR::Matrix4f& eyeViewMatrix, const OVR::Matrix4f& eyeProjectionMatrix, const OVR::Matrix4f& eyeViewProjection, ovrFrameParms& frameParms);
  bool HasFrameCallback();
  bool HasGazeCallback();
  bool HasGestureCallback();
  btTransform GetTransform();
  void StartCollisions(JSContext* cx);
  void StopCollisions();
  void UpdateCollisionObjects(JSContext* cx);
  bool CheckCollision(JSContext* cx, CoreModel* otherModel);
  void CollidedWith(JSContext* cx, CoreModel* otherModel, JS::HandleValue ev);
  void FinishCollisions(JSContext* cx, JS::HandleValue ev);
  CoreModel* ModelById(JSContext* cx, int otherId);
};

void SetupCoreModel(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreModel(JSContext* cx, CoreModel* model);
CoreModel* GetCoreModel(JS::HandleObject obj);

bool CallbackDefined(mozilla::Maybe<JS::PersistentRootedValue>& val);

#endif