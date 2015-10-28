#ifndef CORE_PROGRAM_H
#define CORE_PROGRAM_H

#include "BaseInclude.h"

class CoreProgram {
public:
  JS::Heap<JS::Value>* vertexVal;
  JS::Heap<JS::Value>* fragmentVal;
  OVR::GlProgram* program;
  CoreProgram();
  ~CoreProgram();
  bool Rebuild(JSContext* cx);
};

void SetupCoreProgram(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core);
JSObject* NewCoreProgram(JSContext* cx, CoreProgram* prog);
CoreProgram* GetCoreProgram(JS::HandleObject obj);

#endif
