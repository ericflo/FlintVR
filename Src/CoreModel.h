#ifndef CORE_MODEL_H
#define CORE_MODEL_H

#include "BaseInclude.h"
#include "CoreGeometry.h"
#include "CoreProgram.h"
#include "CoreVector3f.h"
#include "CoreMatrix4f.h"

class CoreModel {
public:
	int id;
  bool isHovered;
  bool isTouching;
  OVR::Matrix4f* computedMatrix;

  mozilla::Maybe<JS::PersistentRootedValue> selfVal;

  CoreGeometry* geometry(JSContext *cx);
  OVR::GlProgram* program(JSContext *cx);
  OVR::Matrix4f* matrix(JSContext *cx);
  OVR::Vector3f* position(JSContext *cx);
  OVR::Vector3f* rotation(JSContext *cx);
  OVR::Vector3f* scale(JSContext *cx);
  mozilla::Maybe<JS::PersistentRootedValue> geometryVal;
  mozilla::Maybe<JS::PersistentRootedValue> programVal;
  mozilla::Maybe<JS::PersistentRootedValue> matrixVal;
  mozilla::Maybe<JS::PersistentRootedValue> positionVal;
  mozilla::Maybe<JS::PersistentRootedValue> rotationVal;
  mozilla::Maybe<JS::PersistentRootedValue> scaleVal;

  mozilla::Maybe<JS::PersistentRootedValue> onFrameVal;
  mozilla::Maybe<JS::PersistentRootedValue> onGazeHoverOverVal;
  mozilla::Maybe<JS::PersistentRootedValue> onGazeHoverOutVal;
  mozilla::Maybe<JS::PersistentRootedValue> onGestureTouchDownVal;
  mozilla::Maybe<JS::PersistentRootedValue> onGestureTouchUpVal;
  mozilla::Maybe<JS::PersistentRootedValue> onGestureTouchCancelVal;

  CoreModel();
  ~CoreModel();
  bool HasFrameCallback();
  bool HasGazeCallback();
  bool HasGestureCallback();
  void ComputeMatrix(JSContext *cx);
};

void SetupCoreModel(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreModel(JSContext *cx, CoreModel* model);
CoreModel* GetCoreModel(JS::HandleObject obj);

bool CallbackDefined(mozilla::Maybe<JS::PersistentRootedValue>& val);

#endif