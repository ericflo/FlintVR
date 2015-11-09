#ifndef BASE_INCLUDE_H
#define BASE_INCLUDE_H

#include <android/log.h>

#include "App.h"
#include "GuiSys.h"
#include "PackageFiles.h"
#include "OVR_Locale.h"

#include "Kernel/OVR_String_Utils.h"
#include "Kernel/OVR_Geometry.h"

#include "jsapi.h"
#include "js/Conversions.h"
#include <mozilla/Maybe.h>

#include "bullet/btBulletDynamicsCommon.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "CoreCommon.h"

inline int bullet_btInfinityMask(){ return btInfinityMask; } // Hack to work around bullet bug

#define LOG_COMPONENT "Flint"

#endif