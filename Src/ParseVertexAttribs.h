#ifndef PARSE_VERTEX_ATTRIBS_H
#define PARSE_VERTEX_ATTRIBS_H

#include "BaseInclude.h"
#include "CoreVector2f.h"
#include "CoreVector3f.h"
#include "CoreVector4f.h"

OVR::VertexAttribs* ParseVertexAttribs(JSContext* cx, JS::HandleValue val);

#endif


