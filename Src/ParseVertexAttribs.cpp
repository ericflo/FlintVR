#include "ParseVertexAttribs.h"

#define MAX_COMPONENT_COUNT 10

OVR::VertexAttribs* ParseVertexAttribs(JSContext* cx, JS::HandleValue val) {
  // First make sure it's an object
  if (!val.isObject()) {
    JS_ReportError(cx, "Expected vertices to be an object");
    return NULL;
  }
  JS::RootedObject verticesObj(cx, &val.toObject());
  // Check that the single argument is an array object
  bool isArray;
  if (!JS_IsArrayObject(cx, val, &isArray) || !isArray) {
    JS_ReportError(cx, "Vertices must be an array");
    return NULL;
  }

  JS::RootedObject input(cx, &val.toObject());
  uint32_t inputLength;
  if (!JS_GetArrayLength(cx, input, &inputLength)) {
    JS_ReportError(cx, "Couldn't get vertices input array length");
    return NULL;
  }

  OVR::VertexAttribs* attribs = new OVR::VertexAttribs();
  bool metadataPhase = true;
  size_t vertexComponents = 0;
  int indices[MAX_COMPONENT_COUNT];
  int reverse[MAX_COMPONENT_COUNT];
  for (int i = 0; i < MAX_COMPONENT_COUNT; ++i) {
    indices[i] = -1;
    reverse[i] = -1;
  }
  int lastVertexIdx = -1;
  size_t vertexNum = 0;
  int totalVertices = -1;
  for (size_t i = 0; i < inputLength; ++i) {
    JS::RootedValue val(cx);
    if (!JS_GetElement(cx, input, i, &val)) {
      JS_ReportError(cx, "Couldn't get item from vertex input at index %d", i);
      return NULL;
    }

    if (val.isNullOrUndefined()) {
      continue;
    }

    // If we're still in the metadata gathering phase and we've found another piece
    if (metadataPhase && val.isInt32()) {
      switch (val.toInt32()) {
      case VERTEX_POSITION:
        reverse[i] = (int)VERTEX_POSITION;
        indices[VERTEX_POSITION] = (int)i;
        break;
      case VERTEX_NORMAL:
        reverse[i] = (int)VERTEX_NORMAL;
        indices[VERTEX_NORMAL] = (int)i;
        break;
      case VERTEX_TANGENT:
        reverse[i] = (int)VERTEX_TANGENT;
        indices[VERTEX_TANGENT] = (int)i;
        break;
      case VERTEX_BINORMAL:
        reverse[i] = (int)VERTEX_BINORMAL;
        indices[VERTEX_BINORMAL] = (int)i;
        break;
      case VERTEX_COLOR:
        reverse[i] = (int)VERTEX_COLOR;
        indices[VERTEX_COLOR] = (int)i;
        break;
      case VERTEX_UV0:
        reverse[i] = (int)VERTEX_UV0;
        indices[VERTEX_UV0] = (int)i;
        break;
      case VERTEX_UV1:
        reverse[i] = (int)VERTEX_UV1;
        indices[VERTEX_UV1] = (int)i;
        break;
      /*
      case VERTEX_JOINT_INDICES:
        reverse[i] = (int)VERTEX_JOINT_INDICES;
        indices[VERTEX_JOINT_INDICES] = (int)i;
        break;
      case VERTEX_JOINT_WEIGHTS:
        reverse[i] = (int)VERTEX_JOINT_WEIGHTS;
        indices[VERTEX_JOINT_WEIGHTS] = (int)i;
        break;
      */
      default:
        JS_ReportError(cx, "Unknown argument in position %d, expected constant flag", i);
        return NULL;
      }
      continue;
    } else if (metadataPhase) {
      // Looks like we're actually past the metadata phase, do some end-calculations
      metadataPhase = false;
      lastVertexIdx = i - 1;
      vertexComponents = i;
      totalVertices = (int)((inputLength - i) / i);
      if (indices[VERTEX_POSITION] != -1) {
        attribs->position.Resize(totalVertices);
      }
      if (indices[VERTEX_NORMAL] != -1) {
        attribs->normal.Resize(totalVertices);
      }
      if (indices[VERTEX_TANGENT] != -1) {
        attribs->tangent.Resize(totalVertices);
      }
      if (indices[VERTEX_BINORMAL] != -1) {
        attribs->binormal.Resize(totalVertices);
      }
      if (indices[VERTEX_COLOR] != -1) {
        attribs->color.Resize(totalVertices);
      }
      if (indices[VERTEX_UV0] != -1) {
        attribs->uv0.Resize(totalVertices);
      }
      if (indices[VERTEX_UV1] != -1) {
        attribs->uv1.Resize(totalVertices);
      }
      /*
      if (indices[VERTEX_JOINT_INDICES] != -1) {
        attribs->jointIndices.Resize(totalVertices);
      }
      if (indices[VERTEX_JOINT_WEIGHTS] != -1) {
        attribs->jointWeights.Resize(totalVertices);
      }
      */
    }

    // Looks like we're processing real data now

    JS::RootedObject argObj(cx, &val.toObject());

    int idx = reverse[i % vertexComponents];
    switch (idx) {
    case VERTEX_POSITION:
      attribs->position[vertexNum] = *(GetVector3f(argObj));
      break;
    case VERTEX_NORMAL:
      attribs->normal[vertexNum] = *(GetVector3f(argObj));
      break;
    case VERTEX_TANGENT:
      attribs->tangent[vertexNum] = *(GetVector3f(argObj));
      break;
    case VERTEX_BINORMAL:
      attribs->binormal[vertexNum] = *(GetVector3f(argObj));
      break;
    case VERTEX_COLOR:
      attribs->color[vertexNum] = *(GetVector4f(argObj));
      break;
    case VERTEX_UV0:
      __android_log_print(ANDROID_LOG_ERROR, "VrCubeWorld", "Adding UV\n");
      attribs->uv0[vertexNum] = *(GetVector2f(argObj));
      break;
    case VERTEX_UV1:
      attribs->uv1[vertexNum] = *(GetVector2f(argObj));
      break;
    /*
    case VERTEX_JOINT_INDICES:
      attribs->jointIndices[vertexNum] = *(GetVector4i(argObj));
      break;
    case VERTEX_JOINT_WEIGHTS:
      attribs->jointWeights[vertexNum] = *(GetVector4f(argObj));
      break;
    */
    }
    if (i % vertexComponents == (size_t)lastVertexIdx) {
      ++vertexNum;
    }
  }

  return attribs;
}