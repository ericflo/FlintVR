#include "CoreModel.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static int CURRENT_MODEL_ID = 1;

CoreModel::CoreModel(void) :
  isHovered(false),
  isTouching(false),
  computedMatrix(),
  children() {
  id = CURRENT_MODEL_ID++;
}

CoreModel::~CoreModel(void) {
  selfVal.reset();
  geometryVal.reset();
  programVal.reset();
  matrixVal.reset();
  positionVal.reset();
  rotationVal.reset();
  scaleVal.reset();
  onFrameVal.reset();
  onGazeHoverOverVal.reset();
  onGazeHoverOutVal.reset();
  onGestureTouchDownVal.reset();
  onGestureTouchUpVal.reset();
  onGestureTouchCancelVal.reset();
}

bool CoreModel::RemoveModel(JSContext* cx, CoreModel* model) {
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

void CoreModel::ComputeMatrices(JSContext* cx, OVR::Matrix4f& transform) {
  OVR::Matrix4f mtx;

  OVR::Matrix4f* mat = matrix(cx);
  if (mat == NULL) {
    mtx = *mat;
  }

  OVR::Vector3f* rot = rotation(cx);
  if (rot != NULL) {
    mtx *= (
      OVR::Matrix4f::RotationX(rot->x) *
      OVR::Matrix4f::RotationY(rot->y) *
      OVR::Matrix4f::RotationZ(rot->z)
    );
  }

  OVR::Vector3f* scl = scale(cx);
  if (scl != NULL) {
    mtx *= OVR::Matrix4f::Scaling(*scl);
  }

  OVR::Vector3f* pos = position(cx);
  if (pos != NULL) {
    mtx.SetTranslation(*pos);
  }

  computedMatrix = transform * mtx;

  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->ComputeMatrices(cx, computedMatrix);
  }
}

void CoreModel::CallFrameCallbacks(JSContext* cx, JS::HandleValue ev) {
  if (HasFrameCallback()) {
    JS::RootedValue callback(cx, onFrameVal.ref());
    JS::RootedObject modelSelf(cx, &selfVal->toObject());
    JS::RootedValue rval(cx);
    JS::RootedValue evVal(cx, ev);
    if (!JS_CallFunctionValue(cx, modelSelf, callback, JS::HandleValueArray(evVal), &rval)) {
      __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not call onFrame callback\n");
    }
  }

  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->CallFrameCallbacks(cx, ev);
  }
}

void CoreModel::CallGazeCallbacks(JSContext* cx, OVR::Vector3f* viewPos, OVR::Vector3f* viewFwd, const OVR::VrFrame& vrFrame, JS::HandleValue ev) {
  bool touchPressed = ( vrFrame.Input.buttonPressed & ( OVR::BUTTON_TOUCH | OVR::BUTTON_A ) ) != 0;
  bool touchReleased = !touchPressed && ( vrFrame.Input.buttonReleased & ( OVR::BUTTON_TOUCH | OVR::BUTTON_A ) ) != 0;
  bool touchDown = ( vrFrame.Input.buttonState & OVR::BUTTON_TOUCH ) != 0;

  if (HasGazeCallback() || HasGestureCallback()) {
    JS::RootedObject self(cx, &selfVal->toObject());
    CoreGeometry* geom = geometry(cx);

    bool foundIntersection = false;
    OVR::Array<OVR::Vector3f> vertices = geom->vertices->position;
    for (int j = 0; j < geom->indices.GetSizeI(); j += 3) {
      OVR::Vector3f v0 = computedMatrix.Transform(vertices[geom->indices[j]]);
      OVR::Vector3f v1 = computedMatrix.Transform(vertices[geom->indices[j + 1]]);
      OVR::Vector3f v2 = computedMatrix.Transform(vertices[geom->indices[j + 2]]);
      float t0, u, v;
      if (OVR::Intersect_RayTriangle(*viewPos, *viewFwd, v0, v1, v2, t0, u, v)) {
        foundIntersection = true;
        break;
      }
      // Check the backface
      if (OVR::Intersect_RayTriangle(*viewPos, *viewFwd, v2, v1, v0, t0, u, v)) {
        foundIntersection = true;
        break;
      }
    }

    JS::RootedValue evVal(cx, ev);
    JS::RootedValue rval(cx);
    JS::RootedValue callback(cx);
    if (foundIntersection) {
      if (!isHovered) {
        isHovered = true;
        // Call the onGazeHoverOver callback
        if (CallbackDefined(onGazeHoverOverVal)) {
          callback = JS::RootedValue(cx, onGazeHoverOverVal.ref());
          // TODO: Construct an object (with t0, u, v ?) to add to env
          if (!JS_CallFunctionValue(cx, self, callback, JS::HandleValueArray(evVal), &rval)) {
            JS_ReportError(cx, "Could not call onGazeHoverOver callback");
          }
        }
      }

      if (!isTouching && touchPressed) {
        isTouching = true;
        // TODO: Construct an object (with t0, u, v ?) to add to env
        if (CallbackDefined(onGestureTouchDownVal)) {
          callback = JS::RootedValue(cx, onGestureTouchDownVal.ref());
          if (!JS_CallFunctionValue(cx, self, callback, JS::HandleValueArray(evVal), &rval)) {
            JS_ReportError(cx, "Could not call onGestureTouchDown callback");
          }
        }
      }

      if ((isTouching && touchReleased) || (isTouching && !touchDown)) {
        isTouching = false;
        // TODO: Construct an object (with t0, u, v ?) to add to env
        if (CallbackDefined(onGestureTouchUpVal)) {
          callback = JS::RootedValue(cx, onGestureTouchUpVal.ref());
          if (!JS_CallFunctionValue(cx, self, callback, JS::HandleValueArray(evVal), &rval)) {
            //JS_ReportError(cx, "Could not call onGestureTouchUp callback");
          }
        }
      }

    } else {
      if (isTouching) {
        isTouching = false;
        if (CallbackDefined(onGestureTouchCancelVal)) {
          callback = JS::RootedValue(cx, onGestureTouchCancelVal.ref());
          if (!JS_CallFunctionValue(cx, self, callback, JS::HandleValueArray(evVal), &rval)) {
            JS_ReportError(cx, "Could not call onGestureTouchCancel callback");
          }
        }
      }

      if (isHovered) {
        isHovered = false;
        if (CallbackDefined(onGazeHoverOutVal)) {
          callback = JS::RootedValue(cx, onGazeHoverOutVal.ref());
          if (!JS_CallFunctionValue(cx, self, callback, JS::HandleValueArray(evVal), &rval)) {
            JS_ReportError(cx, "Could not call onGazeHoverOut callback");
          }
        }
      }
    }
  }

  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->CallGazeCallbacks(cx, viewPos, viewFwd, vrFrame, ev);
  }
}

void CoreModel::DrawEyeView(JSContext* cx, const int eye, const OVR::Matrix4f& eyeViewMatrix, const OVR::Matrix4f& eyeProjectionMatrix, const OVR::Matrix4f& eyeViewProjection, ovrFrameParms& frameParms) {
  OVR::GlProgram* prog = program(cx);
  OVR::GlGeometry* geom = geometry(cx)->geometry;
  glUseProgram(prog->program);
  glUniformMatrix4fv(prog->uModel, 1, GL_TRUE, computedMatrix.M[0]);
  glUniformMatrix4fv(prog->uView, 1, GL_TRUE, eyeViewMatrix.M[0]);
  glUniformMatrix4fv(prog->uProjection, 1, GL_TRUE, eyeProjectionMatrix.M[0]);
  glBindVertexArray(geom->vertexArrayObject);
  glDrawElements(GL_TRIANGLES, geom->indexCount, GL_UNSIGNED_SHORT, NULL);
}

bool CoreModel::HasFrameCallback() {
  return CallbackDefined(onFrameVal);
}

bool CoreModel::HasGazeCallback() {
  return CallbackDefined(onGazeHoverOverVal) || CallbackDefined(onGazeHoverOutVal);
}

bool CoreModel::HasGestureCallback() {
  return (
    CallbackDefined(onGestureTouchDownVal) ||
    CallbackDefined(onGestureTouchUpVal) ||
    CallbackDefined(onGestureTouchCancelVal)
  );
}

static JSClass coreModelClass = {
  "Model",                /* name */
  JSCLASS_HAS_PRIVATE    /* flags */
};

VRJS_GETSET(CoreModel, geometry)
VRJS_GETSET(CoreModel, program)
VRJS_GETSET(CoreModel, matrix)
VRJS_GETSET(CoreModel, position)
VRJS_GETSET(CoreModel, rotation)
VRJS_GETSET(CoreModel, scale)
VRJS_GETSET(CoreModel, onFrame)
VRJS_GETSET(CoreModel, onGazeHoverOver)
VRJS_GETSET(CoreModel, onGazeHoverOut)
VRJS_GETSET(CoreModel, onGestureTouchDown)
VRJS_GETSET(CoreModel, onGestureTouchUp)
VRJS_GETSET(CoreModel, onGestureTouchCancel)

static JSPropertySpec CoreModel_props[] = {
  VRJS_PROP(CoreModel, geometry),
  VRJS_PROP(CoreModel, program),
  VRJS_PROP(CoreModel, matrix),
  VRJS_PROP(CoreModel, position),
  VRJS_PROP(CoreModel, rotation),
  VRJS_PROP(CoreModel, scale),
  VRJS_PROP(CoreModel, onFrame),
  VRJS_PROP(CoreModel, onGazeHoverOver),
  VRJS_PROP(CoreModel, onGazeHoverOut),
  VRJS_PROP(CoreModel, onGestureTouchDown),
  VRJS_PROP(CoreModel, onGestureTouchUp),
  VRJS_PROP(CoreModel, onGestureTouchCancel),
  JS_PS_END
};

CoreGeometry* VRJS_MEMBER(CoreModel, geometry, GetGeometry);
OVR::GlProgram* VRJS_MEMBER(CoreModel, program, GetProgram);
OVR::Matrix4f* VRJS_MEMBER(CoreModel, matrix, GetMatrix4f);
OVR::Vector3f* VRJS_MEMBER(CoreModel, position, GetVector3f);
OVR::Vector3f* VRJS_MEMBER(CoreModel, rotation, GetVector3f);
OVR::Vector3f* VRJS_MEMBER(CoreModel, scale, GetVector3f);

bool _ensureObject(JSContext *cx, JS::MutableHandleValue vp) {
  if (!vp.isObject()) {
    JS_ReportError(cx, "Unexpected argument (expected object)");
    return false;
  }
  return true;
}

bool CoreModel_constructor(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }

  // Check to make sure the arg is an object
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Model argument must be an object");
    return false;
  }
  JS::RootedObject opts(cx, &args[0].toObject());

  // Create our self object
  CoreModel* model = new CoreModel();
  JS::RootedObject self(cx, NewCoreModel(cx, model));

  // Make sure we have a reference to self
  JS::RootedValue selfVal(cx, JS::ObjectOrNullValue(self));
  SetMaybeValue(cx, &selfVal, model->selfVal);

  // Geometry
  JS::RootedValue geometry(cx);
  if (!JS_GetProperty(cx, opts, "geometry", &geometry)) {
    JS_ReportError(cx, "Could not find 'geometry' option");
    delete model;
    return false;
  }
  if (!_ensureObject(cx, &geometry)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &geometry, model->geometryVal);

  // Program
  JS::RootedValue program(cx);
  if (!JS_GetProperty(cx, opts, "program", &program)) {
    JS_ReportError(cx, "Could not find 'program' option");
    delete model;
    return false;
  }
  if (!_ensureObject(cx, &program)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &program, model->programVal);

  // Base transform matrix
  JS::RootedValue matrix(cx);
  if (!JS_GetProperty(cx, opts, "transform", &matrix) || matrix.isNullOrUndefined()) {
    matrix = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreMatrix4f(cx, new OVR::Matrix4f())));
  }
  if (!_ensureObject(cx, &matrix)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &matrix, model->matrixVal);

  // Position
  JS::RootedValue position(cx);
  if (!JS_GetProperty(cx, opts, "position", &position) || position.isNullOrUndefined()) {
    position = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f())));
  }
  if (!_ensureObject(cx, &position)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &position, model->positionVal);

  // Rotation
  JS::RootedValue rotation(cx);
  if (!JS_GetProperty(cx, opts, "rotation", &rotation) || rotation.isNullOrUndefined()) {
    rotation = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f())));
  }
  if (!_ensureObject(cx, &rotation)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &rotation, model->rotationVal);

  // Scale
  JS::RootedValue scale(cx);
  if (!JS_GetProperty(cx, opts, "scale", &scale) || scale.isNullOrUndefined()) {
    scale = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f())));
  }
  if (!_ensureObject(cx, &scale)) {
    delete model;
    return false;
  }
  SetMaybeValue(cx, &scale, model->scaleVal);

  // Callbacks
  SetMaybeCallback(cx, &opts, "onFrame", &self, model->onFrameVal);
  SetMaybeCallback(cx, &opts, "onGazeHoverOver", &self, model->onGazeHoverOverVal);
  SetMaybeCallback(cx, &opts, "onGazeHoverOut", &self, model->onGazeHoverOutVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchDown", &self, model->onGestureTouchDownVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchUp", &self, model->onGestureTouchUpVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchCancel", &self, model->onGestureTouchCancelVal);

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreModel_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete model;
}

void SetupCoreModel(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core) {
  coreModelClass.finalize = CoreModel_finalize;
  JSObject *obj = JS_InitClass(
      cx,
      *core,
      *global,
      &coreModelClass,
      CoreModel_constructor,
      1,
      nullptr, /* Properties */
      nullptr, /* Methods */
      nullptr, /* Static Props */
      nullptr  /* Static Methods */);
  if (!obj) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Model class\n");
    return;
  }
}

JSObject* NewCoreModel(JSContext *cx, CoreModel* model) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreModelClass));
  if (!JS_DefineProperties(cx, self, CoreModel_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on model\n");
  }
  JS_SetPrivate(self, (void *)model);
  return self;
}

CoreModel* GetCoreModel(JS::HandleObject obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  return model;
}

bool CallbackDefined(mozilla::Maybe<JS::PersistentRootedValue>& val) {
  return val.isSome() && !val.ref().isNullOrUndefined();
}