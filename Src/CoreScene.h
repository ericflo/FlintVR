#ifndef CORE_SCENE_H
#define CORE_SCENE_H

#include "BaseInclude.h"
#include "CoreModel.h"
#include "SceneGraph.h"
#include "CoreVector4f.h"

class CoreScene {
public:
  SceneGraph* graph;
  OVR::Vector4f* clearColor;
};

CoreScene* SetupCoreScene(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core, JS::RootedObject *env);
JSObject* NewCoreScene(JSContext *cx, CoreScene* scene);
CoreScene* GetCoreScene(JS::HandleObject obj);

bool CoreScene_add(JSContext *cx, unsigned argc, JS::Value *vp);
bool CoreScene_setClearColor(JSContext *cx, unsigned argc, JS::Value *vp);
bool Core_print(JSContext *cx, unsigned argc, JS::Value *vp); // TODO: Move this somewhere else

#endif
