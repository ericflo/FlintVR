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
  OVR::Array<JS::Heap<JS::Value>> children;
  OVR::Array<int> collidingWithIds;
  OVR::Array<int> seenCollidingIds;

  btTriangleMesh* triMesh;
  btCollisionShape* collisionShape;
  btCollisionObject* collisionObj;

  // Parent
  CoreScene* scene;
  // Self
  JS::Heap<JS::Value>* selfVal;

  // Engine objects
  CoreGeometry* geometry(JSContext* cx);
  CoreProgram* program(JSContext* cx);
  OVR::Matrix4f* matrix(JSContext* cx);
  OVR::Vector3f* position(JSContext* cx);
  OVR::Vector3f* rotation(JSContext* cx);
  OVR::Vector3f* scale(JSContext* cx);
  JS::Heap<JS::Value>* geometryVal;
  JS::Heap<JS::Value>* programVal;
  JS::Heap<JS::Value>* matrixVal;
  JS::Heap<JS::Value>* positionVal;
  JS::Heap<JS::Value>* rotationVal;
  JS::Heap<JS::Value>* scaleVal;

  // Collision properties
  JS::Heap<JS::Value>* collideTagVal;
  JS::Heap<JS::Value>* collidesWithVal;

  // State/uniforms
  JS::Heap<JS::Value>* uniformsVal;

  // Callbacks
  JS::Heap<JS::Value>* onFrameVal;

  JS::Heap<JS::Value>* onGazeHoverOverVal;
  JS::Heap<JS::Value>* onGazeHoverOutVal;

  JS::Heap<JS::Value>* onGestureTouchDownVal;
  JS::Heap<JS::Value>* onGestureTouchUpVal;
  JS::Heap<JS::Value>* onGestureTouchCancelVal;

  JS::Heap<JS::Value>* onCollideStartVal;
  JS::Heap<JS::Value>* onCollideEndVal;

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

bool CallbackDefined(JS::Heap<JS::Value>* val);

#endif