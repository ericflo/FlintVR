#ifndef CORE_MODEL_H
#define CORE_MODEL_H

#include "BaseInclude.h"
#include "CoreGeometry.h"
#include "CoreProgram.h"
#include "CoreVector3f.h"

typedef struct Model {
	int id;
	CoreGeometry* geometry;
	OVR::GlProgram* program;
	OVR::Vector3f* position;
	OVR::Vector3f* rotation;
	OVR::Vector3f* scale;
  mozilla::Maybe<JS::PersistentRootedValue> onFrame;
	mozilla::Maybe<JS::PersistentRootedValue> onHoverOver;
	mozilla::Maybe<JS::PersistentRootedValue> onHoverOut;
	bool isHovered;
} Model;

int GetNextModelId();

void SetupCoreModel(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreModel(JSContext *cx, Model* model);
Model* GetCoreModel(JS::HandleObject obj);

#endif