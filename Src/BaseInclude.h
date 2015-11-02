#ifndef BASE_INCLUDE_H
#define BASE_INCLUDE_H

#include <android/log.h>
#include "jsapi.h"
#include "js/Conversions.h"
#include <mozilla/Maybe.h>
#include "App.h"
#include "GuiSys.h"
#include "OVR_Locale.h"
#include "CoreCommon.h"
#include "bullet/btBulletDynamicsCommon.h"
#include "PackageFiles.h"

inline int bullet_btInfinityMask(){ return btInfinityMask; } // Hack to work around bullet bug

#define LOG_COMPONENT "VrCubeWorld"

#endif