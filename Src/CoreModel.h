#ifndef CORE_MODEL_H
#define CORE_MODEL_H

#include "BaseInclude.h"
#include "CoreGeometry.h"
#include "CoreProgram.h"
#include "CoreVector3f.h"
#include "CoreMatrix4f.h"

typedef struct Model {
	int id;
	CoreGeometry* geometry;
	OVR::GlProgram* program;
  OVR::Matrix4f* matrix;
	OVR::Vector3f* position;
	OVR::Vector3f* rotation;
	OVR::Vector3f* scale;
  OVR::Matrix4f* computedMatrix;
  mozilla::Maybe<JS::PersistentRootedValue> onFrame;
	mozilla::Maybe<JS::PersistentRootedValue> onHoverOver;
	mozilla::Maybe<JS::PersistentRootedValue> onHoverOut;
  mozilla::Maybe<JS::PersistentRootedValue> onGestureTouchDown;
  mozilla::Maybe<JS::PersistentRootedValue> onGestureTouchUp;
  mozilla::Maybe<JS::PersistentRootedValue> onGestureTouchCancel;
	bool isHovered;
  bool isTouching;
} Model;

int GetNextModelId();
void ComputeModelMatrix(Model* model);

void SetupCoreModel(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreModel(JSContext *cx, Model* model);
Model* GetCoreModel(JS::HandleObject obj);

#endif