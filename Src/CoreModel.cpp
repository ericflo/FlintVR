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
  texturesVal = NULL;
  textVal = NULL;
  textColorVal = NULL;
  collideTagVal = NULL;
  matrixVal = NULL;
  positionVal = NULL;
  rotationVal = NULL;
  scaleVal = NULL;
  collidesWithVal = NULL;
  uniformsVal = NULL;
  fileVal = NULL;
  scene = NULL;
  programVal = NULL;
}

CoreModel::~CoreModel(void) {
  delete selfVal;
  delete geometryVal;
  delete programVal;
  delete texturesVal;
  delete fileVal;
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

void CoreModel::AddModel(JSContext* cx, JS::HandleObject otherModelObj) {
  CoreModel* otherModel = GetCoreModel(otherModelObj);

  // TODO: Is this the behavior we want? Propagate the program from the parent?
  if (ValueDefined(programVal) && !ValueDefined(otherModel->programVal)) {
    JS::RootedValue prog(cx, *programVal);
    otherModel->programVal = new JS::Heap<JS::Value>(prog);
  }

  // Annotate the scene object on the model
  otherModel->scene = scene;

  // Make sure collision detection is runninggeom->indexCount
  otherModel->StartCollisions(cx);

  JS::RootedValue otherModelVal(cx, JS::ObjectOrNullValue(otherModelObj));
  children.PushBack(JS::Heap<JS::Value>(otherModelVal));
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
  if (mat != NULL) {
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
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "HMMMMMMM\n");
  if (ValueDefined(geometryVal) && ValueDefined(programVal)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "BOOOOOO\n");

    // Extract the rendering primitives
    OVR::GlProgram* prog = program(cx)->program;
    OVR::GlGeometry* geom = geometry(cx)->geometry;

    // Switch to our program
    glUseProgram(prog->program);

    // Bind textures
    if (ValueDefined(texturesVal)) {
      JS::RootedValue textures(cx, *texturesVal);

      // Check that we were actually handed an array object
      bool isArray;
      if (!JS_IsArrayObject(cx, textures, &isArray)) {
        JS_ReportError(cx, "Couldn't determine whether textures was an array");
        return;
      }
      if (!isArray) {
        JS_ReportError(cx, "Textures should be an array");
        return;
      }

      JS::RootedObject texturesObj(cx, &textures.toObject());

      // Get the length of the textures array
      uint32_t texturesLength;
      if (!JS_GetArrayLength(cx, texturesObj, &texturesLength)) {
        JS_ReportError(cx, "Couldn't get textures array length");
        return;
      }

      // Iterate through the textures and bind each one
      JS::RootedValue texture(cx);
      for (size_t i = 0; i < texturesLength; ++i) {
        if (!JS_GetElement(cx, texturesObj, i, &texture)) {
          JS_ReportError(cx, "Couldn't get texture at index %d", i);
          return;
        }
        JS::RootedObject texObj(cx, &texture.toObject());
        CoreTexture* tex = GetCoreTexture(texObj);
        if (tex == NULL) {
          JS_ReportError(cx, "Texture was null at index %d", i);
          return;
        }
        glActiveTexture(GL_TEXTURE0 + i);
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

    OVR::GL_CheckErrors("Model - DrawEyeView");

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

bool CoreModel::LoadFile(JSContext* cx) {
  // Make sure there's a file to load
  if (!ValueDefined(fileVal)) {
    __android_log_print(ANDROID_LOG_WARN, LOG_COMPONENT, "Tried to load undefined model file\n");
    return true;
  }

  // Get the string
  JS::RootedValue file(cx, *fileVal);
  OVR::String fileStr;
  if (!GetOVRStringVal(cx, file, &fileStr)) {
    JS_ReportError(cx, "Could not get file string from file variable");
    return false;
  }
  OVR::String fullFileStr = FullFilePath(fileStr);

  // Load it into a mem buffer
  OVR::MemBufferFile buf(OVR::MemBufferFile::NoInit);
  if (CURRENT_BASE_DIR.IsEmpty()) {
    if (!OVR::ovr_ReadFileFromApplicationPackage(fileStr.ToCStr(), buf)) {
      JS_ReportError(cx, "Could not load model file %s", fileStr.ToCStr());
      return false;
    }
  } else {
    if (!buf.LoadFile(fullFileStr.ToCStr())) {
      JS_ReportError(cx, "Could not load model file %s", fileStr.ToCStr());
      return false;
    }
  }
  
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "ASDF5\n");

  // Load the model file in the memory buffer via Assimp
  Assimp::Importer importer;
  const aiScene* scn = importer.ReadFileFromMemory(buf.Buffer, buf.Length,
    /*aiProcessPreset_TargetRealtime_MaxQuality*/aiProcessPreset_TargetRealtime_Quality);
  __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "ASDF6\n");

  // Build a map of all textures
  OVR::Hash<OVR::String, OVR::GlTexture> textureMap;
  OVR::Hash<OVR::String, int> textureWidths;
  OVR::Hash<OVR::String, int> textureHeights;
  aiString path;
  for (unsigned int materialNum = 0; materialNum < scn->mNumMaterials; ++materialNum) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "ASDF7 iteration %d\n", materialNum);
    aiMaterial* material = scn->mMaterials[materialNum];
    int textureCount = material->GetTextureCount(aiTextureType_DIFFUSE);
    for (int textureNum = 0; textureNum < textureCount; ++textureNum) {
      __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "ASDF8 iteration %d\n", textureNum);
      if (AI_SUCCESS != material->GetTexture(aiTextureType_DIFFUSE, textureNum, &path)) {
        continue;
      }
      __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "ASDF9 %s\n", path.data);

      OVR::String pathStr(path.data, path.length);
      __android_log_print(ANDROID_LOG_WARN, LOG_COMPONENT, "pathStr: %s\n", pathStr.ToCStr());

      if (pathStr.GetCharAt(0) != '*') {
        __android_log_print(ANDROID_LOG_WARN, LOG_COMPONENT, "Skipping model texture because path is %s\n", pathStr.ToCStr());
        continue;
      }

      OVR::String sub = pathStr.Substring(1, pathStr.GetLength());
      unsigned int textureIdx = atoi(sub.ToCStr());
      aiTexture* texture = scn->mTextures[textureIdx];

      OVR::GlTexture tex;
      if (texture->mHeight == 0) {
        OVR::String fakeFilename;
        if (texture->achFormatHint[0] == '\0') {
          fakeFilename = OVR::String("fakefallback.jpg");
        } else {
          fakeFilename = OVR::String::Format("fake.%s", texture->achFormatHint);
        }
        OVR::MemBuffer texBuf(texture->pcData, texture->mWidth);
        int width;
        int height;
        tex = OVR::LoadTextureFromBuffer(fakeFilename.ToCStr(), texBuf, OVR::TextureFlags_t(OVR::TEXTUREFLAG_NO_DEFAULT), width, height);
        textureWidths.Set(pathStr, width);
        textureHeights.Set(pathStr, height);
      } else {
        tex = OVR::LoadRGBATextureFromMemory((const unsigned char*)texture->pcData, texture->mWidth, texture->mHeight, true);
        textureWidths.Set(pathStr, texture->mWidth);
        textureHeights.Set(pathStr, texture->mHeight);
      }
      textureMap.Set(pathStr, tex);
      __android_log_print(ANDROID_LOG_WARN, LOG_COMPONENT, "textureMap.Set %s %d\n", pathStr.ToCStr(), tex.texture);
    }
  }

  // Each mesh gets its own model, to be added as a submodel to this one
  for (unsigned int meshNum = 0; meshNum < scn->mNumMeshes; ++meshNum) {
    aiMesh* mesh = scn->mMeshes[meshNum];

    // If there are no position vertices, we don't have anything to do, so skip
    if (!mesh->HasPositions()) {
      continue;
    }

    // Build up our vertex attributes
    OVR::VertexAttribs* vertices = new OVR::VertexAttribs();

    vertices->position.Resize(mesh->mNumVertices);
    if (mesh->HasNormals()) {
      vertices->normal.Resize(mesh->mNumVertices);
    }
    if (mesh->HasTangentsAndBitangents()) {
      vertices->tangent.Resize(mesh->mNumVertices);
      vertices->binormal.Resize(mesh->mNumVertices);
    }
    vertices->color.Resize(mesh->mNumVertices);
    if (mesh->GetNumUVChannels() > 0) {
      vertices->uv0.Resize(mesh->mNumVertices);
    }
    if (mesh->GetNumUVChannels() > 1) {
      vertices->uv1.Resize(mesh->mNumVertices);
    }
    // TODO: Joint indices & weights

    for (unsigned int vertexNum = 0; vertexNum < mesh->mNumVertices; ++vertexNum) {
      aiVector3D vertex = mesh->mVertices[vertexNum];
      vertices->position[vertexNum] = OVR::Vector3f(vertex.x, vertex.y, vertex.z);

      if (mesh->HasNormals()) {
        aiVector3D normal = mesh->mNormals[vertexNum];
        vertices->normal[vertexNum] = OVR::Vector3f(normal.x, normal.y, normal.z);
      }

      if (mesh->HasTangentsAndBitangents()) {
        aiVector3D tangent = mesh->mTangents[vertexNum];
        vertices->tangent[vertexNum] = OVR::Vector3f(tangent.x, tangent.y, tangent.z);

        aiVector3D binormal = mesh->mBitangents[vertexNum];
        vertices->binormal[vertexNum] = OVR::Vector3f(binormal.x, binormal.y, binormal.z);
      }
      
      if (mesh->GetNumColorChannels() > 0) {
        aiColor4D* color = mesh->mColors[vertexNum];
        vertices->color[vertexNum] = OVR::Vector4f(color[0].r, color[0].g, color[0].b, color[0].a);
      }
      if (mesh->GetNumColorChannels() > 1) {
        __android_log_print(ANDROID_LOG_WARN, LOG_COMPONENT, "Discarding extra color channels\n");
      }

      // TODO: Figure out how to not throw away a whole dimension for these UVs
      if (mesh->GetNumUVChannels() > 0) {
        aiVector3D uv = mesh->mTextureCoords[0][vertexNum];
        vertices->uv0[vertexNum] = OVR::Vector2f(uv.x, uv.y);
      }
      if (mesh->GetNumUVChannels() > 1) {
        aiVector3D uv = mesh->mTextureCoords[1][vertexNum];
        vertices->uv1[vertexNum] = OVR::Vector2f(uv.x, uv.y);
      }
      if (mesh->GetNumUVChannels() > 2) {
        __android_log_print(ANDROID_LOG_WARN, LOG_COMPONENT, "Discarding extra UV channels\n");
      }
    }

    // Build up our geometry indices
    OVR::Array<OVR::TriangleIndex> indices;
    indices.Resize(mesh->mNumFaces * 3);
    int indexIdx = 0; // lol
    for (unsigned int faceNum = 0; faceNum < mesh->mNumFaces; ++faceNum) {
      aiFace face = mesh->mFaces[faceNum];
      if (face.mNumIndices != 3) { // Skip anything that's not a triangle
        continue;
      }
      for (unsigned int indexNum = 0; indexNum < face.mNumIndices; ++indexNum) {
        unsigned int index = face.mIndices[indexNum];
        indices[indexIdx] = index;
        indexIdx++;
      }
    }

    // Create CoreTexture array
    JS::RootedObject textureArray(cx, JS_NewArrayObject(cx, 0));
    int textureArrayCount = 0;
    aiMaterial* material = scn->mMaterials[mesh->mMaterialIndex];
    int textureCount = material->GetTextureCount(aiTextureType_DIFFUSE);
    aiString texturePath;
    for (int i = 0; i < textureCount; ++i) {
      if (material->GetTexture(aiTextureType_DIFFUSE, i, &texturePath) == AI_SUCCESS) {
        OVR::String texturePathStr(texturePath.data, texturePath.length);
        if (texturePathStr.GetCharAt(0) != '*') {
          continue;
        }
        OVR::GlTexture tex = *(textureMap.Get(texturePathStr));
        int width = *(textureWidths.Get(texturePathStr));
        int height = *(textureHeights.Get(texturePathStr));
        CoreTexture* coreTex = new CoreTexture(tex, width, height);
        JS::RootedObject coreTexObj(cx, NewCoreTexture(cx, coreTex));
        JS::RootedValue coreTexVal(cx, JS::ObjectOrNullValue(coreTexObj));
        if (!JS_SetElement(cx, textureArray, textureArrayCount, coreTexVal)) {
          JS_ReportError(cx, "Could not place texture in textures array");
          delete vertices;
          return false;
        }
        textureArrayCount++;
      }
    }

    CoreModel* model = new CoreModel();
    
    // Add the model's geometry
    JS::RootedValue geometry(cx, JS::ObjectOrNullValue(
      NewCoreGeometry(cx, new CoreGeometry(vertices, indices))));
    model->geometryVal = new JS::Heap<JS::Value>(geometry);

    // Add the model's textures
    if (textureArrayCount > 0) {
      model->texturesVal = new JS::Heap<JS::Value>(
        JS::ObjectOrNullValue(textureArray));
    }

    // Fill any defaults we haven't filled in (all of them)
    model->FillDefaults(cx);

    JS::RootedObject submodel(cx, NewCoreModel(cx, model));
    JS::RootedValue sval(cx, JS::ObjectOrNullValue(submodel));
    model->selfVal = new JS::Heap<JS::Value>(sval);

    AddModel(cx, submodel);
  }

  OVR::GL_CheckErrors("LoadFile");

  return true;
}

void CoreModel::FillDefaults(JSContext* cx) {
  if (matrixVal == NULL) {
    JS::RootedValue matrix(cx, JS::ObjectOrNullValue(
      NewCoreMatrix4f(cx, new OVR::Matrix4f())));
    matrixVal = new JS::Heap<JS::Value>(matrix);
  }

  if (positionVal == NULL) {
    JS::RootedValue position(cx, JS::ObjectOrNullValue(
      NewCoreVector3f(cx, new OVR::Vector3f())));
    positionVal = new JS::Heap<JS::Value>(position);
  }
  
  if (rotationVal == NULL) {
    JS::RootedValue rotation(cx, JS::ObjectOrNullValue(
      NewCoreVector3f(cx, new OVR::Vector3f())));
    rotationVal = new JS::Heap<JS::Value>(rotation);
  }

  if (scaleVal == NULL) {
    JS::RootedValue scale(cx, JS::ObjectOrNullValue(
      NewCoreVector3f(cx, new OVR::Vector3f(1, 1, 1))));
    scaleVal = new JS::Heap<JS::Value>(scale);
  }

  if (collidesWithVal == NULL) {
    JS::RootedValue collidesWith(cx, JS::ObjectOrNullValue(
      JS_NewPlainObject(cx)));
    collidesWithVal = new JS::Heap<JS::Value>(collidesWith);
  }

  if (uniformsVal == NULL) {
    JS::RootedValue uniforms(cx, JS::ObjectOrNullValue(
      JS_NewPlainObject(cx)));
    uniformsVal = new JS::Heap<JS::Value>(uniforms);
  }
}

VRJS_GETSET(CoreModel, geometry)
VRJS_GETSET(CoreModel, program)
VRJS_GETSET(CoreModel, matrix)
VRJS_GETSET(CoreModel, position)
VRJS_GETSET(CoreModel, rotation)
VRJS_GETSET(CoreModel, scale)
VRJS_GETSET(CoreModel, textures)
VRJS_GETSET_POST(CoreModel, file, item->LoadFile(cx))
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
  VRJS_PROP(CoreModel, textures),
  VRJS_PROP(CoreModel, file),
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

  // Textures
  JS::RootedValue textures(cx);
  if (JS_GetProperty(cx, opts, "textures", &textures) && !textures.isNullOrUndefined() && textures.isObject()) {
    model->texturesVal = new JS::Heap<JS::Value>(textures);
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
  if (JS_GetProperty(cx, opts, "transform", &matrix) && !matrix.isNullOrUndefined() && matrix.isObject()) {
    model->matrixVal = new JS::Heap<JS::Value>(matrix);
  }

  // Position
  JS::RootedValue position(cx);
  if (JS_GetProperty(cx, opts, "position", &position) && !position.isNullOrUndefined() && position.isObject()) {
    model->positionVal = new JS::Heap<JS::Value>(position);
  }

  // Rotation
  JS::RootedValue rotation(cx);
  if (JS_GetProperty(cx, opts, "rotation", &rotation) && !rotation.isNullOrUndefined() && rotation.isObject()) {
    model->rotationVal = new JS::Heap<JS::Value>(rotation);
  }

  // Scale
  JS::RootedValue scale(cx);
  if (JS_GetProperty(cx, opts, "scale", &scale) && !scale.isNullOrUndefined() && scale.isObject()) {
    model->scaleVal = new JS::Heap<JS::Value>(scale);
  }

  // CollideTag
  JS::RootedValue collideTag(cx);
  if (JS_GetProperty(cx, opts, "collideTag", &collideTag) && !collideTag.isNullOrUndefined() && collideTag.isString()) {
    model->collideTagVal = new JS::Heap<JS::Value>(collideTag);
  }

  // CollidesWith
  JS::RootedValue collidesWith(cx);
  if (JS_GetProperty(cx, opts, "collidesWith", &collidesWith) && !collidesWith.isNullOrUndefined() && collidesWith.isObject()) {
    model->collidesWithVal = new JS::Heap<JS::Value>(collidesWith);
  }

  // Uniforms
  JS::RootedValue uniforms(cx);
  if (JS_GetProperty(cx, opts, "uniforms", &uniforms) && !uniforms.isNullOrUndefined() && uniforms.isObject()) {
    model->uniformsVal = new JS::Heap<JS::Value>(uniforms);
  }

  // Callbacks
  SetMaybeCallback(cx, &opts, "onFrame", &model->onFrameVal);
  SetMaybeCallback(cx, &opts, "onGazeHoverOver", &model->onGazeHoverOverVal);
  SetMaybeCallback(cx, &opts, "onGazeHoverOut", &model->onGazeHoverOutVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchDown", &model->onGestureTouchDownVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchUp", &model->onGestureTouchUpVal);
  SetMaybeCallback(cx, &opts, "onGestureTouchCancel", &model->onGestureTouchCancelVal);
  SetMaybeCallback(cx, &opts, "onCollideStart", &model->onCollideStartVal);
  SetMaybeCallback(cx, &opts, "onCollideEnd", &model->onCollideEndVal);

  model->FillDefaults(cx);

  // Load file contents
  JS::RootedValue fileVal(cx);
  if (JS_GetProperty(cx, opts, "file", &fileVal) && !fileVal.isNullOrUndefined() && fileVal.isString()) {
    model->fileVal = new JS::Heap<JS::Value>(fileVal);
    if (!model->LoadFile(cx)) {
      return false;
    }
  }

  // Return our self object
  args.rval().set(JS::ObjectOrNullValue(self));
  return true;
}

void CoreModel_finalize(JSFreeOp *fop, JSObject *obj) {
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  JS_SetPrivate(obj, NULL);
  delete model;
}

void CoreModel_trace(JSTracer* tracer, JSObject* obj) {
  __android_log_print(ANDROID_LOG_DEBUG, LOG_COMPONENT, "Tracing model\n");
  CoreModel* model = (CoreModel*)JS_GetPrivate(obj);
  if (model != NULL) {
    TraceHeap(tracer, model->geometryVal, "model", "geometryVal");
    TraceHeap(tracer, model->programVal, "model", "programVal");
    TraceHeap(tracer, model->matrixVal, "model", "matrixVal");
    TraceHeap(tracer, model->positionVal, "model", "positionVal");
    TraceHeap(tracer, model->rotationVal, "model", "rotationVal");
    TraceHeap(tracer, model->scaleVal, "model", "scaleVal");
    TraceHeap(tracer, model->texturesVal, "model", "texturesVal");
    TraceHeap(tracer, model->fileVal, "model", "fileVal");
    TraceHeap(tracer, model->textVal, "model", "textVal");
    TraceHeap(tracer, model->textColorVal, "model", "textColorVal");
    TraceHeap(tracer, model->collideTagVal, "model", "collideTagVal");
    TraceHeap(tracer, model->collidesWithVal, "model", "collidesWithVal");
    TraceHeap(tracer, model->uniformsVal, "model", "uniformsVal");
    TraceHeap(tracer, model->onFrameVal, "model", "onFrameVal");
    TraceHeap(tracer, model->onGazeHoverOverVal, "model", "onGazeHoverOverVal");
    TraceHeap(tracer, model->onGazeHoverOutVal, "model", "onGazeHoverOutVal");
    TraceHeap(tracer, model->onGestureTouchDownVal, "model", "onGestureTouchDownVal");
    TraceHeap(tracer, model->onGestureTouchUpVal, "model", "onGestureTouchUpVal");
    TraceHeap(tracer, model->onGestureTouchCancelVal, "model", "onGestureTouchCancelVal");
    TraceHeap(tracer, model->onCollideStartVal, "model", "onCollideStartVal");
    TraceHeap(tracer, model->onCollideEndVal, "model", "onCollideEndVal");
    for (int i = 0; i < model->children.GetSizeI(); ++i) {
      char buffer[50];
      sprintf(buffer, "child%d", i);
      TraceHeap(tracer, &model->children[i], "model", buffer);
    }
  }
  __android_log_print(ANDROID_LOG_DEBUG, LOG_COMPONENT, "Finished tracing model\n");
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

  // Read the model object in
  JS::RootedObject thisObj(cx, &args.thisv().toObject());
  CoreModel* thisModel = GetCoreModel(thisObj);

  thisModel->AddModel(cx, otherModelObj);

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
  otherModel->scene = NULL;

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