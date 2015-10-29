#include "CoreModel.h"
#include "CoreScene.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static int CURRENT_MODEL_ID = 1;

CoreModel::CoreModel(void) :
  isHovered(false),
  isTouching(false),
  localMatrix(),
  worldMatrix() {
  id = CURRENT_MODEL_ID++;
  collisionShape = NULL;
  collisionObj = NULL;
}

CoreModel::~CoreModel(void) {
  delete selfVal;
  delete geometryVal;
  delete programVal;
  delete matrixVal;
  delete positionVal;
  delete rotationVal;
  delete scaleVal;
  delete collideTagVal;
  delete collidesWithVal;
  delete uniformsVal;
  delete onFrameVal;
  delete onGazeHoverOverVal;
  delete onGazeHoverOutVal;
  delete onGestureTouchDownVal;
  delete onGestureTouchUpVal;
  delete onGestureTouchCancelVal;
  delete onCollideStartVal;
  delete onCollideEndVal;
  StopCollisions();
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

  // If we found it, remove it
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

  localMatrix = mtx;
  worldMatrix = transform * mtx;

  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->ComputeMatrices(cx, worldMatrix);
  }
}

void CoreModel::CallFrameCallbacks(JSContext* cx, JS::HandleValue ev) {
  if (HasFrameCallback()) {
    JS::RootedValue callback(cx, *onFrameVal);
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
      OVR::Vector3f v0 = worldMatrix.Transform(vertices[geom->indices[j]]);
      OVR::Vector3f v1 = worldMatrix.Transform(vertices[geom->indices[j + 1]]);
      OVR::Vector3f v2 = worldMatrix.Transform(vertices[geom->indices[j + 2]]);
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
          callback = JS::RootedValue(cx, *onGazeHoverOverVal);
          // TODO: Construct an object (with t0, u, v ?) to add to env
          if (!JS_CallFunctionValue(cx, self, callback,
                                    JS::HandleValueArray(evVal), &rval)) {
            JS_ReportError(cx, "Could not call onGazeHoverOver callback");
          }
        }
      }

      if (!isTouching && touchPressed) {
        isTouching = true;
        // TODO: Construct an object (with t0, u, v ?) to add to env
        if (CallbackDefined(onGestureTouchDownVal)) {
          callback = JS::RootedValue(cx, *onGestureTouchDownVal);
          if (!JS_CallFunctionValue(cx, self, callback,
                                    JS::HandleValueArray(evVal), &rval)) {
            JS_ReportError(cx, "Could not call onGestureTouchDown callback");
          }
        }
      }

      if ((isTouching && touchReleased) || (isTouching && !touchDown)) {
        isTouching = false;
        // TODO: Construct an object (with t0, u, v ?) to add to env
        if (CallbackDefined(onGestureTouchUpVal)) {
          callback = JS::RootedValue(cx, *onGestureTouchUpVal);
          if (!JS_CallFunctionValue(cx, self, callback,
                                    JS::HandleValueArray(evVal), &rval)) {
            JS_ReportError(cx, "Could not call onGestureTouchUp callback");
          }
        }
      }

    } else {
      if (isTouching) {
        isTouching = false;
        if (CallbackDefined(onGestureTouchCancelVal)) {
          callback = JS::RootedValue(cx, *onGestureTouchCancelVal);
          if (!JS_CallFunctionValue(cx, self, callback,
                                    JS::HandleValueArray(evVal), &rval)) {
            JS_ReportError(cx, "Could not call onGestureTouchCancel callback");
          }
        }
      }

      if (isHovered) {
        isHovered = false;
        if (CallbackDefined(onGazeHoverOutVal)) {
          callback = JS::RootedValue(cx, *onGazeHoverOutVal);
          if (!JS_CallFunctionValue(cx, self, callback,
                                    JS::HandleValueArray(evVal), &rval)) {
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

void CoreModel::DrawEyeView(JSContext* cx, const int eye,
                            const OVR::Matrix4f& eyeViewMatrix,
                            const OVR::Matrix4f& eyeProjectionMatrix,
                            const OVR::Matrix4f& eyeViewProjection,
                            ovrFrameParms& frameParms) {
  // Extract the rendering primitives
  OVR::GlProgram* prog = program(cx)->program;
  OVR::GlGeometry* geom = geometry(cx)->geometry;

  // Switch to our program
  glUseProgram(prog->program);

  // Now we bind our uniforms
  glUniformMatrix4fv(prog->uModel, 1, GL_TRUE, worldMatrix.M[0]);
  glUniformMatrix4fv(prog->uView, 1, GL_TRUE, eyeViewMatrix.M[0]);
  glUniformMatrix4fv(prog->uProjection, 1, GL_TRUE, eyeProjectionMatrix.M[0]);

  JS::RootedObject uniforms(cx, &uniformsVal->toObject());
  JS::Rooted<JS::IdVector> names(cx, JS::IdVector(cx));
  if (!JS_Enumerate(cx, uniforms, &names)) {
    JS_ReportError(cx, "Couldn't enumerate uniforms");
    return;
  }
  JS::RootedValue nameVal(cx);
  JS::RootedValue val(cx);
  OVR::String name;
  for (uint32_t i = 0; i < names.length(); ++i) {
    // Root the ID
    JS::RootedId nameId(cx, names[i]);

    // Get the name string as a value
    if (!JS_IdToValue(cx, nameId, &nameVal)) {
      JS_ReportError(cx, "Could not convert the uniform id to a value");
      return;
    }

    // Get the string out of the name value
    if (!GetOVRStringVal(cx, nameVal, &name)) {
      JS_ReportError(cx, "Could not convert the uniform name to a string");
      return;
    }

    // Get the value from the id
    if (!JS_GetPropertyById(cx, uniforms, nameId, &val)) {
      JS_ReportError(cx, "Could not get the uniform value");
      return;
    }

    // Get the location of the uniform
    int loc = glGetUniformLocation(prog->program, name.ToCStr());

    // Figure out what type the uniform is and then set it
    if (val.isBoolean()) {
      glUniform1i(loc, val.isTrue() ? 1 : 0);
    } else if (val.isNumber()) {
      glUniform1f(loc, val.toNumber());
    } else if (val.isObject()) {
      JS::RootedObject valObj(cx, &val.toObject());
      if (JS_InstanceOf(cx, valObj, CoreVector2f_class(), NULL)) {
        OVR::Vector2f* vec = GetVector2f(valObj);
        glUniform2fv(loc, 1, &vec->x);
      } else if (JS_InstanceOf(cx, valObj, CoreVector3f_class(), NULL)) {
        OVR::Vector3f* vec = GetVector3f(valObj);
        glUniform3fv(loc, 1, &vec->x);
      } else if (JS_InstanceOf(cx, valObj, CoreVector4f_class(), NULL)) {
        OVR::Vector4f* vec = GetVector4f(valObj);
        glUniform4fv(loc, 1, &vec->x);
      } else if (JS_InstanceOf(cx, valObj, CoreMatrix4f_class(), NULL)) {
        OVR::Matrix4f* mat = GetMatrix4f(valObj);
        glUniformMatrix4fv(loc, 1, GL_TRUE, mat->M[0]);
      } else {
        JS_ReportError(cx, "Uniform type unknown");
        return;
      }
    } else {
      JS_ReportError(cx, "Uniform type unknown");
      return;
    }
  }

  // Bind the vertex array
  glBindVertexArray(geom->vertexArrayObject);

  // Finally, draw the elements
  glDrawElements(GL_TRIANGLES, geom->indexCount, GL_UNSIGNED_SHORT, NULL);

  // Recurse
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->DrawEyeView(cx, eye, eyeViewMatrix, eyeProjectionMatrix, eyeViewProjection, frameParms);
  }
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

btTransform CoreModel::GetTransform() {
  OVR::Vector3f translation = worldMatrix.GetTranslation();
  btTransform transform;
  transform.setIdentity();
  transform.setBasis(btMatrix3x3(
    worldMatrix.M[0][0], worldMatrix.M[0][1], worldMatrix.M[0][2],
    worldMatrix.M[1][0], worldMatrix.M[1][1], worldMatrix.M[1][2],
    worldMatrix.M[2][0], worldMatrix.M[2][1], worldMatrix.M[2][2]
  ));
  transform.setOrigin(btVector3(translation.x, translation.y, translation.z));
  return transform;
}

void CoreModel::StartCollisions(JSContext* cx) {
  if (collisionShape != NULL) {
    delete collisionShape;
    collisionShape = NULL;
  }
  if (collisionObj != NULL) {
    delete collisionObj;
    collisionObj = NULL;
  }
  // TODO: Ensure there are none missing

  CoreGeometry *geom = geometry(cx);

  triMesh = new btTriangleMesh();
  for (int i = 0; i < geom->indices.GetSizeI(); i += 3) {
    OVR::Vector3f v0 = geom->vertices->position[geom->indices[i]];
    OVR::Vector3f v1 = geom->vertices->position[geom->indices[i + 1]];
    OVR::Vector3f v2 = geom->vertices->position[geom->indices[i + 2]];
    triMesh->addTriangle(
      btVector3(v0.x, v0.y, v0.z),
      btVector3(v1.x, v1.y, v1.z),
      btVector3(v2.x, v2.y, v2.z)
    );
  }
  collisionShape = new btConvexTriangleMeshShape(triMesh, true);

  btRigidBody::btRigidBodyConstructionInfo rbInfo(btScalar(1.0), NULL,
    collisionShape, btVector3(0, 0, 0));
  btRigidBody* body = new btRigidBody(rbInfo);

  scene->dynamicsWorld->addRigidBody(body);

  collisionObj = static_cast<btCollisionObject*>(body);
  collisionObj->setUserIndex(id);

  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->StartCollisions(cx);
  }
}

void CoreModel::StopCollisions() {
  if (triMesh != NULL) {
    delete triMesh;
    triMesh = NULL;
  }
  if (collisionObj != NULL) {
    if (scene != NULL && scene->dynamicsWorld != NULL) {
      scene->dynamicsWorld->removeCollisionObject(collisionObj);
    }
    delete collisionObj;
    collisionObj = NULL;
  }
  if (collisionShape != NULL) {
    delete collisionShape;
    collisionShape = NULL;
  }
  // TODO: Determine how to properly do this. We don't have access to cx.
  /*
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->StopCollisions();
  }
  */
}

void CoreModel::UpdateCollisionObjects(JSContext* cx) {
  collisionObj->setWorldTransform(GetTransform());
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->UpdateCollisionObjects(cx);
  }
}

bool CoreModel::CheckCollision(JSContext* cx, CoreModel* otherModel) {
  if (collideTagVal == NULL || otherModel->collidesWithVal == NULL) {
    return false;
  }

  OVR::String tag;
  JS::RootedValue rootedCollideTagVal(cx, *collideTagVal);
  if (!GetOVRStringVal(cx, rootedCollideTagVal, &tag)) {
    return false;
  }

  JS::RootedObject collidesWith(cx, &otherModel->collidesWithVal->toObject());
  JS::RootedValue rval(cx);
  if (!JS_GetProperty(cx, collidesWith, tag.ToCStr(), &rval)) {
    return false;
  }

  return rval.isTrue();
}

void CoreModel::CollidedWith(JSContext* cx, CoreModel* otherModel, JS::HandleValue ev) {
  if (!CheckCollision(cx, otherModel) || !otherModel->CheckCollision(cx, this)) {
    return;
  }

  // Mark this id as seen
  seenCollidingIds.PushBack(otherModel->id);

  // First check to see if we're already colliding with 
  for (int i = 0; i < collidingWithIds.GetSizeI(); ++i) {
    if (collidingWithIds[i] == otherModel->id) {
      return;
    }
  }

  // If we made it here, it's a new collision, so trigger a callback
  if (CallbackDefined(onCollideStartVal)) {
    JS::RootedValue callback(cx, *onCollideStartVal);
    JS::RootedValue rval(cx);
    JS::RootedValue evVal(cx, ev);
    JS::RootedObject selfObj(cx, &selfVal->toObject());
    if (!JS_CallFunctionValue(cx, selfObj, callback, JS::HandleValueArray(evVal), &rval)) {
      JS_ReportError(cx, "Could not call onCollideStart callback");
    }
  }

  // Now add this model's id to the colliding ids
  collidingWithIds.PushBack(otherModel->id);
}

void CoreModel::FinishCollisions(JSContext* cx, JS::HandleValue ev) {
  for (int i = 0; i < collidingWithIds.GetSizeI(); ++i) {
    bool found = false;
    for (int j = 0; j < seenCollidingIds.GetSizeI(); ++j) {
      if (collidingWithIds[i] == seenCollidingIds[j]) {
        found = true;
        break;
      }
    }
    if (!found) {
      if (CallbackDefined(onCollideEndVal)) {
        JS::RootedValue callback(cx, *onCollideEndVal);
        //CoreModel* otherModel = scene->ModelById(cx, collidingWithIds[i]);
        JS::RootedValue rval(cx);
        JS::RootedValue evVal(cx, ev);
        JS::RootedObject selfObj(cx, &selfVal->toObject());
        if (!JS_CallFunctionValue(cx, selfObj, callback, JS::HandleValueArray(evVal), &rval)) {
          JS_ReportError(cx, "Could not call onCollideEnd callback");
        }
        break;
      }
    }
  }

  collidingWithIds = seenCollidingIds;
  seenCollidingIds.Clear();
}

CoreModel* CoreModel::ModelById(JSContext* cx, int otherId) {
  if (id == otherId) {
    // FIXME: This is pretty gross, we're going the long way to get our own pointer
    JS::RootedObject selfObj(cx, &selfVal->toObject());
    CoreModel* model = GetCoreModel(selfObj);
    return model;
  }
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    CoreModel* model = child->ModelById(cx, otherId);
    if (model != NULL) {
      return model;
    }
  }
  return NULL;
}

static JSClass coreModelClass = {
  "Model",               /* name */
  JSCLASS_HAS_PRIVATE    /* flags */
};

VRJS_GETSET(CoreModel, geometry)
VRJS_GETSET(CoreModel, program)
VRJS_GETSET(CoreModel, matrix)
VRJS_GETSET(CoreModel, position)
VRJS_GETSET(CoreModel, rotation)
VRJS_GETSET(CoreModel, scale)
VRJS_GETSET(CoreModel, collideTag)
VRJS_GETSET(CoreModel, collidesWith)
VRJS_GETSET(CoreModel, uniforms)
VRJS_GETSET(CoreModel, onFrame)
VRJS_GETSET(CoreModel, onGazeHoverOver)
VRJS_GETSET(CoreModel, onGazeHoverOut)
VRJS_GETSET(CoreModel, onGestureTouchDown)
VRJS_GETSET(CoreModel, onGestureTouchUp)
VRJS_GETSET(CoreModel, onGestureTouchCancel)
VRJS_GETSET(CoreModel, onCollideStart)
VRJS_GETSET(CoreModel, onCollideEnd)

static JSPropertySpec CoreModel_props[] = {
  VRJS_PROP(CoreModel, geometry),
  VRJS_PROP(CoreModel, program),
  VRJS_PROP(CoreModel, matrix),
  VRJS_PROP(CoreModel, position),
  VRJS_PROP(CoreModel, rotation),
  VRJS_PROP(CoreModel, scale),
  VRJS_PROP(CoreModel, collideTag),
  VRJS_PROP(CoreModel, collidesWith),
  VRJS_PROP(CoreModel, uniforms),
  VRJS_PROP(CoreModel, onFrame),
  VRJS_PROP(CoreModel, onGazeHoverOver),
  VRJS_PROP(CoreModel, onGazeHoverOut),
  VRJS_PROP(CoreModel, onGestureTouchDown),
  VRJS_PROP(CoreModel, onGestureTouchUp),
  VRJS_PROP(CoreModel, onGestureTouchCancel),
  VRJS_PROP(CoreModel, onCollideStart),
  VRJS_PROP(CoreModel, onCollideEnd),
  JS_PS_END
};

CoreGeometry* VRJS_MEMBER(CoreModel, geometry, GetCoreGeometry);
CoreProgram* VRJS_MEMBER(CoreModel, program, GetCoreProgram);
OVR::Matrix4f* VRJS_MEMBER(CoreModel, matrix, GetMatrix4f);
OVR::Vector3f* VRJS_MEMBER(CoreModel, position, GetVector3f);
OVR::Vector3f* VRJS_MEMBER(CoreModel, rotation, GetVector3f);
OVR::Vector3f* VRJS_MEMBER(CoreModel, scale, GetVector3f);

bool CoreModel_constructor(JSContext* cx, unsigned argc, JS::Value *vp) {
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
  JS::RootedValue sval(cx, JS::ObjectOrNullValue(self));
  model->selfVal = new JS::Heap<JS::Value>(sval);

  // Geometry
  JS::RootedValue geometry(cx);
  if (!JS_GetProperty(cx, opts, "geometry", &geometry)) {
    JS_ReportError(cx, "Could not find 'geometry' option");
    delete model;
    return false;
  }
  if (!EnsureJSObject(cx, &geometry)) {
    delete model;
    return false;
  }
  model->geometryVal = new JS::Heap<JS::Value>(geometry);

  // Program
  JS::RootedValue program(cx);
  if (!JS_GetProperty(cx, opts, "program", &program)) {
    JS_ReportError(cx, "Could not find 'program' option");
    delete model;
    return false;
  }
  if (!EnsureJSObject(cx, &program)) {
    delete model;
    return false;
  }
  model->programVal = new JS::Heap<JS::Value>(program);

  // Base transform matrix
  JS::RootedValue matrix(cx);
  if (!JS_GetProperty(cx, opts, "transform", &matrix) || matrix.isNullOrUndefined()) {
    matrix = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreMatrix4f(cx, new OVR::Matrix4f())));
  }
  if (!EnsureJSObject(cx, &matrix)) {
    delete model;
    return false;
  }
  model->matrixVal = new JS::Heap<JS::Value>(matrix);

  // Position
  JS::RootedValue position(cx);
  if (!JS_GetProperty(cx, opts, "position", &position) || position.isNullOrUndefined()) {
    position = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f())));
  }
  if (!EnsureJSObject(cx, &position)) {
    delete model;
    return false;
  }
  model->positionVal = new JS::Heap<JS::Value>(position);

  // Rotation
  JS::RootedValue rotation(cx);
  if (!JS_GetProperty(cx, opts, "rotation", &rotation) || rotation.isNullOrUndefined()) {
    rotation = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f())));
  }
  if (!EnsureJSObject(cx, &rotation)) {
    delete model;
    return false;
  }
  model->rotationVal = new JS::Heap<JS::Value>(rotation);

  // Scale
  JS::RootedValue scale(cx);
  if (!JS_GetProperty(cx, opts, "scale", &scale) || scale.isNullOrUndefined()) {
    scale = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f(1, 1, 1))));
  }
  if (!EnsureJSObject(cx, &scale)) {
    delete model;
    return false;
  }
  model->scaleVal = new JS::Heap<JS::Value>(scale);

  // CollideTag (defaults to "default")
  JS::RootedValue collideTag(cx);
  if (!JS_GetProperty(cx, opts, "collideTag", &collideTag) || collideTag.isNullOrUndefined()) {
    collideTag.setString(JS_NewStringCopyZ(cx, "default"));
  }
  model->collideTagVal = new JS::Heap<JS::Value>(collideTag);

  // CollidesWith
  JS::RootedValue collidesWith(cx);
  if (!JS_GetProperty(cx, opts, "collidesWith", &collidesWith) || collidesWith.isNullOrUndefined()) {
    JSObject* obj = JS_NewPlainObject(cx);
    collidesWith.setObject(*obj);
  }
  model->collidesWithVal = new JS::Heap<JS::Value>(collidesWith);

  // Uniforms
  JS::RootedValue uniforms(cx);
  if (!JS_GetProperty(cx, opts, "uniforms", &uniforms) || uniforms.isNullOrUndefined()) {
    JSObject* obj = JS_NewPlainObject(cx);
    uniforms.setObject(*obj);
  }
  model->uniformsVal = new JS::Heap<JS::Value>(uniforms);

  // Callbacks
  SetMaybeCallback(cx, &opts, "onFrame", &model->onFrameVal);
  SetMaybeCallback(cx, &opts, "onGazeHoverOver", &model->onGazeHoverOverVal);
  SetMaybeCallback(cx, &opts, "onGazeHoverOut", &model->onGazeHoverOutVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchDown", &model->onGestureTouchDownVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchUp", &model->onGestureTouchUpVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchCancel", &model->onGestureTouchCancelVal);
  SetMaybeCallback(cx, &opts, "onCollideStart", &model->onCollideStartVal);
  SetMaybeCallback(cx, &opts, "onCollideEnd", &model->onCollideEndVal);

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreModel_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete model;
}

void CoreModel_trace(JSTracer *tracer, JSObject *obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Tracing model id: %d\n", model->id);
  JS_CallValueTracer(tracer, model->geometryVal, "geometryVal");
  JS_CallValueTracer(tracer, model->programVal, "programVal");
  JS_CallValueTracer(tracer, model->matrixVal, "matrixVal");
  JS_CallValueTracer(tracer, model->positionVal, "positionVal");
  JS_CallValueTracer(tracer, model->rotationVal, "rotationVal");
  JS_CallValueTracer(tracer, model->scaleVal, "scaleVal");
  JS_CallValueTracer(tracer, model->collideTagVal, "collideTagVal");
  JS_CallValueTracer(tracer, model->collidesWithVal, "collidesWithVal");
  JS_CallValueTracer(tracer, model->uniformsVal, "uniformsVal");
  JS_CallValueTracer(tracer, model->onFrameVal, "onFrameVal");
  JS_CallValueTracer(tracer, model->onGazeHoverOverVal, "onGazeHoverOverVal");
  JS_CallValueTracer(tracer, model->onGazeHoverOutVal, "onGazeHoverOutVal");
  JS_CallValueTracer(tracer, model->onGestureTouchDownVal, "onGestureTouchDownVal");
  JS_CallValueTracer(tracer, model->onGestureTouchUpVal, "onGestureTouchUpVal");
  JS_CallValueTracer(tracer, model->onGestureTouchCancelVal, "onGestureTouchCancelVal");
  JS_CallValueTracer(tracer, model->onCollideStartVal, "onCollideStartVal");
  JS_CallValueTracer(tracer, model->onCollideEndVal, "onCollideEndVal");
  for (int i = 0; i < model->children.GetSizeI(); ++i) {
    char buffer[50];
    sprintf(buffer, "child%d", i);
    JS_CallValueTracer(tracer, &model->children[i], buffer);
  }
}

bool CoreModel_add(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Expected add to take a model argument");
    return false;
  }

  JS::RootedObject otherModelObj(cx, &args[0].toObject());
  CoreModel* otherModel = GetCoreModel(otherModelObj);

  // Read the model object in
  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  CoreModel* thisModel = GetCoreModel(thisObj);

  // Annotate the scene object on the model
  otherModel->scene = thisModel->scene;

  // Make sure collision detection is set up and configured
  otherModel->StartCollisions(cx);

  thisModel->children.PushBack(JS::Heap<JS::Value>(args[0]));

  return true;
}

bool CoreModel_remove(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

  // Check the arguments length
  if (args.length() != 1) {
    JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
    return false;
  }
  if (!args[0].isObject()) {
    JS_ReportError(cx, "Expected remove to take a model argument");
    return false;
  }

  JS::RootedObject otherModelObj(cx, &args[0].toObject());
  CoreModel* otherModel = GetCoreModel(otherModelObj);

  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  CoreModel* thisModel = GetCoreModel(thisObj);

  // Make sure collision detection is stopped
  otherModel->StopCollisions();

  // Remove the scene object from the model
  otherModel->scene = nullptr;

  if (!thisModel->RemoveModel(cx, otherModel)) {
    JS_ReportError(cx, "Could not find model to remove");
    return false;
  }

  return true;
}

void SetupCoreModel(JSContext* cx, JS::RootedObject *global, JS::RootedObject *core) {
  coreModelClass.finalize = CoreModel_finalize;
  coreModelClass.trace = CoreModel_trace;
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

JSObject* NewCoreModel(JSContext* cx, CoreModel* model) {
  JS::RootedObject self(cx, JS_NewObject(cx, &coreModelClass));
  if (!JS_DefineProperties(cx, self, CoreModel_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on model\n");
  }
  if (!JS_DefineFunction(cx, self, "add", &CoreModel_add, 0, 0)) {
    JS_ReportError(cx, "Could not create model.add function");
    return NULL;
  }
  if (!JS_DefineFunction(cx, self, "remove", &CoreModel_remove, 0, 0)) {
    JS_ReportError(cx, "Could not create model.remove function");
    return NULL;
  }
  JS_SetPrivate(self, (void *)model);
  return self;
}

CoreModel* GetCoreModel(JS::HandleObject obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  return model;
}

bool CallbackDefined(JS::Heap<JS::Value>* val) {
  return val != NULL && !val->isNullOrUndefined();
}