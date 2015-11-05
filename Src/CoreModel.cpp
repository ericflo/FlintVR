#include "CoreModel.h"
#include "CoreScene.h"


static int CURRENT_MODEL_ID = 1;

CoreModel::CoreModel(void) :
  isHovered(false),
  isTouching(false),
  textSize(12.0f),
  textOutlineSize(0.0f),
  localMatrix(),
  worldMatrix() {
  id = CURRENT_MODEL_ID++;
  collisionShape = NULL;
  collisionObj = NULL;
  textureVal = NULL;
  textVal = NULL;
  textColorVal = NULL;
  collideTagVal = NULL;
  collidesWithVal = NULL;
}

CoreModel::~CoreModel(void) {
  delete selfVal;
  delete geometryVal;
  delete programVal;
  delete textureVal;
  delete textVal;
  delete textColorVal;
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

void CoreModel::CallGazeCallbacks(JSContext* cx, OVR::OvrGuiSys* guiSys, OVR::Vector3f* viewPos, OVR::Vector3f* viewFwd, const OVR::VrFrame& vrFrame, JS::HandleValue ev) {
  bool touchPressed = ( vrFrame.Input.buttonPressed & ( OVR::BUTTON_TOUCH | OVR::BUTTON_A ) ) != 0;
  bool touchReleased = !touchPressed && ( vrFrame.Input.buttonReleased & ( OVR::BUTTON_TOUCH | OVR::BUTTON_A ) ) != 0;
  bool touchDown = ( vrFrame.Input.buttonState & OVR::BUTTON_TOUCH ) != 0;

  if (HasGazeCallback() || HasGestureCallback()) {
    JS::RootedObject self(cx, &selfVal->toObject());

    bool foundIntersection = false;

    // First check for intersection via the regular geometry
    if (ValueDefined(geometryVal)) {
      CoreGeometry* geom = geometry(cx);
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
    }

    // Next check if text boundaries have been intersected
    if (ValueDefined(textVal)) {
      OVR::String txtStr;
      JS::RootedValue t(cx, *textVal);
      if (!GetOVRStringVal(cx, t, &txtStr)) {
        JS_ReportError(cx, "Could not get string data from text string");
        return;
      }

      // Get metrics from the text (width/height is all we care about for now)
      size_t txtLen;
      float txtWidth;
      float txtHeight;
      float txtAscent;
      float txtDescent;
      float txtFontHeight;
      int const TXT_MAX_LINES = 128;
      float txtLineWidths[TXT_MAX_LINES];
      int txtNumLines;
      guiSys->GetDefaultFont().CalcTextMetrics(txtStr.ToCStr(), txtLen,
        txtWidth, txtHeight, txtAscent, txtDescent, txtFontHeight,
        txtLineWidths, TXT_MAX_LINES, txtNumLines);

      // Build quad vertices from the metrics + the model matrix
      OVR::Vector3f bl = worldMatrix.GetTranslation();
      OVR::Vector3f br = worldMatrix.Transform(OVR::Vector3f(txtWidth * textSize, 0, 0));
      OVR::Vector3f tl = worldMatrix.Transform(OVR::Vector3f(0, txtHeight * textSize, 0));
      OVR::Vector3f tr = worldMatrix.Transform(OVR::Vector3f(txtWidth * textSize, txtHeight * textSize, 0));

      float t0, u, v;
      // Triangle 1
      if (OVR::Intersect_RayTriangle(*viewPos, *viewFwd, br, bl, tl, t0, u, v)) {
        foundIntersection = true;
      } else if (OVR::Intersect_RayTriangle(*viewPos, *viewFwd, tl, bl, br, t0, u, v)) {
        // Check the backface
        foundIntersection = true;
      } else if (OVR::Intersect_RayTriangle(*viewPos, *viewFwd, br, tr, tl, t0, u, v)) {
        // Triangle 2
        foundIntersection = true;
      } else if (OVR::Intersect_RayTriangle(*viewPos, *viewFwd, tl, tr, br, t0, u, v)) {
        // Check the backface
        foundIntersection = true;
      }
    }

    JS::RootedValue evVal(cx, ev);
    JS::RootedValue rval(cx);
    JS::RootedValue callback(cx);
    if (foundIntersection) {
      if (!isHovered) {
        isHovered = true;
        // Call the onGazeHoverOver callback
        if (ValueDefined(onGazeHoverOverVal)) {
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
        if (ValueDefined(onGestureTouchDownVal)) {
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
        if (ValueDefined(onGestureTouchUpVal)) {
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
        if (ValueDefined(onGestureTouchCancelVal)) {
          callback = JS::RootedValue(cx, *onGestureTouchCancelVal);
          if (!JS_CallFunctionValue(cx, self, callback,
                                    JS::HandleValueArray(evVal), &rval)) {
            JS_ReportError(cx, "Could not call onGestureTouchCancel callback");
          }
        }
      }

      if (isHovered) {
        isHovered = false;
        if (ValueDefined(onGazeHoverOutVal)) {
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
    child->CallGazeCallbacks(cx, guiSys, viewPos, viewFwd, vrFrame, ev);
  }
}

void CoreModel::DrawEyeView(JSContext* cx,
                            OVR::OvrGuiSys* guiSys,
                            const int eye,
                            const OVR::Matrix4f& eyeViewMatrix,
                            const OVR::Matrix4f& eyeProjectionMatrix,
                            const OVR::Matrix4f& eyeViewProjection,
                            ovrFrameParms& frameParms) {
  if (ValueDefined(geometryVal) && ValueDefined(programVal)) {
    // Extract the rendering primitives
    OVR::GlProgram* prog = program(cx)->program;
    OVR::GlGeometry* geom = geometry(cx)->geometry;

    // Switch to our program
    glUseProgram(prog->program);

    // Use the current texture
    if (textureVal != NULL && !textureVal->isNullOrUndefined() && textureVal->isObject()) {
      JS::RootedObject texObj(cx, &textureVal->toObject());
      CoreTexture* tex = GetCoreTexture(texObj);
      if (tex != NULL) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(tex->texture.target, tex->texture.texture);
      }
    }

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

    // Unbind
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
  }

  // Render text if we have some
  if (ValueDefined(textVal)) {
    OVR::String textStr;
    JS::RootedValue t(cx, *textVal);
    if (!GetOVRStringVal(cx, t, &textStr)) {
      JS_ReportError(cx, "Could not get string data from text string");
      return;
    }

    OVR::Vector4f textColor(0, 0, 0, 1);
    if (ValueDefined(textColorVal)) {
      JS::RootedObject textColorObj(cx, &textColorVal->toObject());
      textColor = *(GetVector4f(textColorObj));
    }

    OVR::fontParms_t fontParms;
    fontParms.AlphaCenter = 0.50f - textOutlineSize;
    fontParms.ColorCenter = 0.50f;

    guiSys->GetDefaultFontSurface().DrawText3D(
      guiSys->GetDefaultFont(),
      fontParms,
      worldMatrix.GetTranslation(),
      OVR::Vector3f(worldMatrix.M[2][0], worldMatrix.M[2][1], worldMatrix.M[2][2]),
      OVR::Vector3f(worldMatrix.M[1][0], worldMatrix.M[1][1], worldMatrix.M[1][2]),
      textSize,
      textColor,
      textStr.ToCStr()
    );
  }

  // Recurse
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->DrawEyeView(cx, guiSys, eye, eyeViewMatrix, eyeProjectionMatrix, eyeViewProjection, frameParms);
  }
}

bool CoreModel::HasFrameCallback() {
  return ValueDefined(onFrameVal);
}

bool CoreModel::HasGazeCallback() {
  return ValueDefined(onGazeHoverOverVal) || ValueDefined(onGazeHoverOutVal);
}

bool CoreModel::HasGestureCallback() {
  return (
    ValueDefined(onGestureTouchDownVal) ||
    ValueDefined(onGestureTouchUpVal) ||
    ValueDefined(onGestureTouchCancelVal)
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
  if (ValueDefined(geometryVal) && ValueDefined(collideTagVal)) {
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
  }

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
  if (collisionObj != NULL) {
    collisionObj->setWorldTransform(GetTransform());
  }
  for (int i = 0; i < children.GetSizeI(); ++i) {
    JS::RootedObject childObj(cx, &children[i].toObject());
    CoreModel* child = GetCoreModel(childObj);
    child->UpdateCollisionObjects(cx);
  }
}

bool CoreModel::CheckCollision(JSContext* cx, CoreModel* otherModel) {
  if (!ValueDefined(collideTagVal) || !ValueDefined(otherModel->collidesWithVal)) {
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
  if (ValueDefined(onCollideStartVal)) {
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
      if (ValueDefined(onCollideEndVal)) {
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
  JSCLASS_HAS_PRIVATE,   /* flags */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  CoreModel_finalize,
  NULL,
  NULL,
  NULL,
  CoreModel_trace
};

VRJS_GETSET(CoreModel, geometry)
VRJS_GETSET(CoreModel, program)
VRJS_GETSET(CoreModel, matrix)
VRJS_GETSET(CoreModel, position)
VRJS_GETSET(CoreModel, rotation)
VRJS_GETSET(CoreModel, scale)
VRJS_GETSET(CoreModel, texture)
VRJS_GETSET(CoreModel, text)
VRJS_GETSET(CoreModel, textColor)
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

static bool CoreModel_get_textSize(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreModel* item = GetCoreModel(self);
  args.rval().setNumber(item->textSize);
  return true;
}

static bool CoreModel_set_textSize(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreModel* item = GetCoreModel(self);
  item->textSize = args[0].toNumber();
  return true;
}

static bool CoreModel_get_textOutlineSize(JSContext* cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreModel* item = GetCoreModel(self);
  args.rval().setNumber(item->textOutlineSize);
  return true;
}

static bool CoreModel_set_textOutlineSize(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args[0].isNumber()) {
    JS_ReportError(cx, "Invalid number specified");
    return false;
  }
  JS::RootedObject self(cx, &args.thisv().toObject());
  CoreModel* item = GetCoreModel(self);
  item->textOutlineSize = args[0].toNumber();
  return true;
}

static JSPropertySpec CoreModel_props[] = {
  VRJS_PROP(CoreModel, geometry),
  VRJS_PROP(CoreModel, program),
  VRJS_PROP(CoreModel, matrix),
  VRJS_PROP(CoreModel, position),
  VRJS_PROP(CoreModel, rotation),
  VRJS_PROP(CoreModel, scale),
  VRJS_PROP(CoreModel, texture),
  VRJS_PROP(CoreModel, text),
  VRJS_PROP(CoreModel, textColor),
  VRJS_PROP(CoreModel, textSize),
  VRJS_PROP(CoreModel, textOutlineSize),
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
  if (JS_GetProperty(cx, opts, "geometry", &geometry) && !geometry.isNullOrUndefined() && geometry.isObject()) {
    model->geometryVal = new JS::Heap<JS::Value>(geometry);
  }

  // Program
  JS::RootedValue program(cx);
  if (JS_GetProperty(cx, opts, "program", &program) && !program.isNullOrUndefined() && program.isObject()) {
    model->programVal = new JS::Heap<JS::Value>(program);
  }

  // Texture
  JS::RootedValue texture(cx);
  if (JS_GetProperty(cx, opts, "texture", &texture) && !texture.isNullOrUndefined() && texture.isObject()) {
    model->textureVal = new JS::Heap<JS::Value>(texture);
  }

  // Text
  JS::RootedValue text(cx);
  if (JS_GetProperty(cx, opts, "text", &text) && !text.isNullOrUndefined() && text.isString()) {
    model->textVal = new JS::Heap<JS::Value>(text);
  }

  // TextColor
  JS::RootedValue textColor(cx);
  if (JS_GetProperty(cx, opts, "textColor", &textColor) && !textColor.isNullOrUndefined() && textColor.isObject()) {
    model->textColorVal = new JS::Heap<JS::Value>(textColor);
  }

  // TextSize
  JS::RootedValue textSizeVal(cx);
  if (JS_GetProperty(cx, opts, "textSize", &textSizeVal) && !textSizeVal.isNullOrUndefined() && textSizeVal.isNumber()) {
    model->textSize = textSizeVal.toNumber();
  }

  // TextOutlineSize
  JS::RootedValue textOutlineSizeVal(cx);
  if (JS_GetProperty(cx, opts, "textOutlineSize", &textOutlineSizeVal) && !textOutlineSizeVal.isNullOrUndefined() && textOutlineSizeVal.isNumber()) {
    model->textOutlineSize = textOutlineSizeVal.toNumber();
  }

  // Base transform matrix
  JS::RootedValue matrix(cx);
  if (JS_GetProperty(cx, opts, "transform", &matrix) || matrix.isNullOrUndefined() || !matrix.isObject()) {
    matrix = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreMatrix4f(cx, new OVR::Matrix4f())));
  }
  model->matrixVal = new JS::Heap<JS::Value>(matrix);

  // Position
  JS::RootedValue position(cx);
  if (!JS_GetProperty(cx, opts, "position", &position) || position.isNullOrUndefined() || !position.isObject()) {
    position = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f())));
  }
  model->positionVal = new JS::Heap<JS::Value>(position);

  // Rotation
  JS::RootedValue rotation(cx);
  if (!JS_GetProperty(cx, opts, "rotation", &rotation) || rotation.isNullOrUndefined() || !rotation.isObject()) {
    rotation = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f())));
  }
  model->rotationVal = new JS::Heap<JS::Value>(rotation);

  // Scale
  JS::RootedValue scale(cx);
  if (!JS_GetProperty(cx, opts, "scale", &scale) || scale.isNullOrUndefined() || !scale.isObject()) {
    scale = JS::RootedValue(cx,
      JS::ObjectOrNullValue(NewCoreVector3f(cx, new OVR::Vector3f(1, 1, 1))));
  }
  model->scaleVal = new JS::Heap<JS::Value>(scale);

  // CollideTag
  JS::RootedValue collideTag(cx);
  if (JS_GetProperty(cx, opts, "collideTag", &collideTag) && !collideTag.isNullOrUndefined() && collideTag.isString()) {
    model->collideTagVal = new JS::Heap<JS::Value>(collideTag);
  }

  // CollidesWith
  JS::RootedValue collidesWith(cx);
  if (!JS_GetProperty(cx, opts, "collidesWith", &collidesWith) || collidesWith.isNullOrUndefined() || !collidesWith.isObject()) {
    JSObject* obj = JS_NewPlainObject(cx);
    collidesWith.setObject(*obj);
  }
  model->collidesWithVal = new JS::Heap<JS::Value>(collidesWith);

  // Uniforms
  JS::RootedValue uniforms(cx);
  if (!JS_GetProperty(cx, opts, "uniforms", &uniforms) || uniforms.isNullOrUndefined() || !uniforms.isObject()) {
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
  JS_CallValueTracer(tracer, model->textureVal, "textureVal");
  JS_CallValueTracer(tracer, model->textVal, "textVal");
  JS_CallValueTracer(tracer, model->textColorVal, "textColorVal");
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
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Finished tracing model id: %d\n", model->id);
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