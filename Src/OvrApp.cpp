#include "BaseInclude.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "Android/JniUtils.h"
#include "SoundEffectContext.h"
#include <memory>
#include "CoreProgram.h"
#include "CoreVector3f.h"
#include "CoreVector4f.h"
#include "CoreMatrix4f.h"
#include "CoreGeometry.h"
#include "CoreModel.h"
#include "CoreScene.h"
#include "CoreTexture.h"

#define ERROR_DISPLAY_SECONDS 10

#define LOAD_FROM_FILE true
#define SCRIPT_PATH "assets/example1_cubes_and_stars.js"
#define SCRIPT_URL "http://flint-hello.ngrok.com"

static const int CPU_LEVEL = 1;
static const int GPU_LEVEL = 1;

OVR::String PREVIOUS_ERROR;
OVR::String LATEST_ERROR;

void reportError(JSContext* cx, const char *message, JSErrorReport *report) {
  OVR::String err = OVR::String::Format("%s:%u:%s\n",
      report->filename ? report->filename : "[no filename]",
      (unsigned int) report->lineno,
      message);
  if (err != PREVIOUS_ERROR) {
    PREVIOUS_ERROR = err;
    LATEST_ERROR = err;
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "%s", err.ToCStr());
  }
}

class OvrApp : public OVR::VrAppInterface {
public:
  OvrApp(AAssetManager *assetManager);
  ~OvrApp();

  virtual void Configure(OVR::ovrSettings & settings);

  virtual void OneTimeInit(const char* fromPackage, const char* launchIntentJSON, const char* launchIntentURI);
  virtual void OneTimeShutdown();
  virtual bool OnKeyEvent(const int keyCode, const int repeatCount, const OVR::KeyEventType eventType);
  virtual OVR::Matrix4f Frame(const OVR::VrFrame& vrFrame );
  virtual OVR::Matrix4f DrawEyeView(const int eye, const float fovDegreesX, const float fovDegreesY, ovrFrameParms& frameParms);

  OVR::ovrLocale& GetLocale() {
    return *Locale;
  }

  void LoadURL(OVR::String & url);
  void LoadAssetFile(OVR::String & path);

private:
  std::unique_ptr<OVR::ovrSoundEffectContext> SoundEffectContext;
  std::unique_ptr<OVR::OvrGuiSys::SoundEffectPlayer> SoundEffectPlayer;
  OVR::OvrGuiSys* GuiSys;
  OVR::ovrLocale* Locale;
  ovrMatrix4f CenterEyeViewMatrix;
  AAssetManager* AssetManager;
  JSRuntime* SpidermonkeyJSRuntime;
  JSContext* SpidermonkeyJSContext;
  mozilla::Maybe<JS::PersistentRootedObject> SpidermonkeyGlobal;
  CoreScene* scene;
  mozilla::Maybe<JS::CompileOptions> CompileOptions;
  JS::Heap<JS::Value>* envValue;
};

// Build global JS object
static JSClass globalClass = {
  "global",
  JSCLASS_GLOBAL_FLAGS
};

OvrApp::OvrApp(AAssetManager *assetManager) :
  GuiSys(OVR::OvrGuiSys::Create()),
  Locale(NULL) {
  CenterEyeViewMatrix = ovrMatrix4f_CreateIdentity();
  AssetManager = assetManager;
}

OvrApp::~OvrApp() {
  OVR::OvrGuiSys::Destroy(GuiSys);
  delete envValue;
}

void OvrApp::OneTimeInit(const char* fromPackageName, const char* launchIntentJSON, const char* launchIntentURI) {
  auto java = app->GetJava();
  SoundEffectContext.reset(new OVR::ovrSoundEffectContext(*java->Env, java->ActivityObject));
  SoundEffectContext->Initialize();
  SoundEffectPlayer.reset(new OVR::OvrGuiSys::ovrDummySoundEffectPlayer());

  Locale = OVR::ovrLocale::Create(*app, "default");

  OVR::String fontName;
  GetLocale().GetString("@string/font_name", "efigs.fnt", fontName);
  GuiSys->Init(this->app, *SoundEffectPlayer, fontName.ToCStr(), &app->GetDebugLines());

  //app->SetShowFPS(true);

  // Initialize JS engine
  JS_Init();

  // Set up the JS runtime and context
  SpidermonkeyJSRuntime = JS_NewRuntime(32L * 1024 * 1024); // Garbage collect at 32MB
  if (!SpidermonkeyJSRuntime) {
    return;
  }
  SpidermonkeyJSContext = JS_NewContext(SpidermonkeyJSRuntime, 8192);
  if (!SpidermonkeyJSContext) {
    return;
  }
  JSContext* cx = SpidermonkeyJSContext;

  JS_SetErrorReporter(SpidermonkeyJSRuntime, reportError);

  // Create the global JS object
  JS::RootedObject global(cx, JS::PersistentRootedObject(cx, JS_NewGlobalObject(cx, &globalClass, nullptr, JS::FireOnNewGlobalHook)));
  {
    JSAutoCompartment ac(cx, global);
    JS_InitStandardClasses(cx, global);

    // Create CompileOptions on the stack
    SpidermonkeyGlobal.reset();
    CompileOptions.emplace(cx);

    // Build the environment to send to vrmain
    JS::RootedObject core(cx, JS_NewObject(cx, nullptr));
    JS::RootedValue coreValue(cx, JS::ObjectOrNullValue(core));
    SetupCoreProgram(cx, &global, &core);
    SetupCoreVector2f(cx, &global, &core);
    SetupCoreVector3f(cx, &global, &core);
    SetupCoreVector4f(cx, &global, &core);
    SetupCoreMatrix4f(cx, &global, &core);
    SetupCoreGeometry(cx, &global, &core);
    SetupCoreTexture(cx, &global, &core);
    SetupCoreModel(cx, &global, &core);
    JS::RootedObject env(cx, JS_NewObject(cx, nullptr));
    scene = SetupCoreScene(cx, &global, &core, &env);
    if (!JS_SetProperty(cx, env, "Core", coreValue)) {
      __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core\n");
      return;
    }
    envValue = new JS::Heap<JS::Value>(JS::ObjectOrNullValue(env));

    // Now set up the Flint object in the global namespace
    JS::RootedValue rootedEnv(cx, JS::ObjectOrNullValue(env));
    if (!JS_SetProperty(cx, global, "Flint", rootedEnv)) {
      __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core\n");
      return;
    }

    SpidermonkeyGlobal.reset();
    SpidermonkeyGlobal.emplace(cx, global);
  }

  if (LOAD_FROM_FILE) {
    OVR::String assetPath(SCRIPT_PATH);
    LoadAssetFile(assetPath);
  } else {
    OVR::String scriptUrl(SCRIPT_URL);
    LoadURL(scriptUrl);
  }
}

void OvrApp::LoadAssetFile(OVR::String & path) {
  //auto java = app->GetJava();
  JSContext* cx = SpidermonkeyJSContext;

  {
    JSAutoCompartment ac(cx, SpidermonkeyGlobal.ref());
    JS::RootedValue env(cx, *envValue);

    CURRENT_BASE_DIR.Clear();

    OVR::MemBufferFile buf(OVR::MemBufferFile::NoInit);
    if (!OVR::ovr_ReadFileFromApplicationPackage(path.ToCStr(), buf)) {
      __android_log_print(ANDROID_LOG_VERBOSE, LOG_COMPONENT, "Could not load model file %s", path.ToCStr());
      return;
    }

    // Now eval our script so we can get at the vrmain function
    JS::RootedValue rval(cx);
    bool ok = JS::Evaluate(cx, CompileOptions.ref(), (char *)buf.Buffer, buf.Length, &rval);
    if (!ok) {
      __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not evaluate script");
      app->ShowInfoText(ERROR_DISPLAY_SECONDS, "Could not evaluate script");
    }

    // Call vrmain
    if (ok) {
      ok = JS_CallFunctionName(cx, SpidermonkeyGlobal.ref(), "vrmain", JS::HandleValueArray(env), &rval);
      if (!ok) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not call vrmain\n");
      }
    }
  }
}

void OvrApp::LoadURL(OVR::String & url) {
  auto java = app->GetJava();
  JSContext* cx = SpidermonkeyJSContext;

  //CURRENT_URL = url;

  {
    JSAutoCompartment ac(cx, SpidermonkeyGlobal.ref());
    JS::RootedValue env(cx, *envValue);

    // Load the remote app
    jclass cls = ovr_GetGlobalClassReference(java->Env, java->ActivityObject, "oculus/MainActivity");
    jmethodID loadApp = ovr_GetMethodID(java->Env, cls, "loadApp", "(Ljava/lang/String;)Z");
    jboolean loaded = java->Env->CallBooleanMethod(java->ActivityObject, loadApp,
      java->Env->NewStringUTF(url.ToCStr()));
    if (!loaded) {
      __android_log_print(ANDROID_LOG_VERBOSE, LOG_COMPONENT, "Could not load URL");
      return;
    }

    // Get the app's entrypoint
    jmethodID getAppEntrypoint = ovr_GetMethodID(java->Env, cls, "getAppEntrypoint", "()Ljava/lang/String;");
    jstring entrypoint = (jstring)java->Env->CallObjectMethod(java->ActivityObject, getAppEntrypoint);
    jboolean isCopy;
    const char* entrypointChars = java->Env->GetStringUTFChars(entrypoint, &isCopy);
    OVR::String entrypointStr(entrypointChars);
    java->Env->ReleaseStringUTFChars(entrypoint, entrypointChars);

    // Get the base directory
    jmethodID getBaseDir = ovr_GetMethodID(java->Env, cls, "getBaseDir", "()Ljava/lang/String;");
    jstring baseDir = (jstring)java->Env->CallObjectMethod(java->ActivityObject, getBaseDir);
    const char* baseDirChars = java->Env->GetStringUTFChars(baseDir, &isCopy);
    CURRENT_BASE_DIR = OVR::String(baseDirChars);
    java->Env->ReleaseStringUTFChars(baseDir, baseDirChars);

    // Now eval our script so we can get at the vrmain function
    JS::RootedValue rval(cx);
    bool ok = JS::Evaluate(cx, CompileOptions.ref(), entrypointStr.ToCStr(), &rval);
    if (!ok) {
      __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not evaluate script");
      app->ShowInfoText(ERROR_DISPLAY_SECONDS, "Could not evaluate script");
    }

    // Call vrmain
    if (ok) {
      ok = JS_CallFunctionName(cx, SpidermonkeyGlobal.ref(), "vrmain", JS::HandleValueArray(env), &rval);
      if (!ok) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not call vrmain\n");
      }
    }
  }
}

void OvrApp::OneTimeShutdown() {
  JS_DestroyContext(SpidermonkeyJSContext);
  JS_DestroyRuntime(SpidermonkeyJSRuntime);
  JS_ShutDown();
}

void OvrApp::Configure(OVR::ovrSettings & settings) {
  settings.PerformanceParms.CpuLevel = CPU_LEVEL;
  settings.PerformanceParms.GpuLevel = GPU_LEVEL;
  settings.EyeBufferParms.multisamples = 2;
}

bool OvrApp::OnKeyEvent(const int keyCode, const int repeatCount, const OVR::KeyEventType eventType) {
  if (GuiSys->OnKeyEvent(keyCode, repeatCount, eventType)) {
    return true;
  }
  return false;
}

OVR::Matrix4f OvrApp::Frame(const OVR::VrFrame& vrFrame) {
  CenterEyeViewMatrix = vrapi_GetCenterEyeViewMatrix(&app->GetHeadModelParms(), &vrFrame.Tracking, NULL);

  // Show any errors
  if (!LATEST_ERROR.IsEmpty()) {
    app->ShowInfoText(ERROR_DISPLAY_SECONDS, "%s", LATEST_ERROR.ToCStr());
    LATEST_ERROR.Clear();
  }

  // Call the function returned from vrmain
  JSContext* cx = SpidermonkeyJSContext;

  {
    JS::RootedObject global(cx, SpidermonkeyGlobal.ref());
    JSAutoCompartment ac(cx, global);

    OVR::Vector3f* viewPos = new OVR::Vector3f(OVR::GetViewMatrixPosition(CenterEyeViewMatrix));
    OVR::Vector3f* viewFwd = new OVR::Vector3f(OVR::GetViewMatrixForward(CenterEyeViewMatrix));

    // Build the ev
    JS::RootedObject ev(cx, JS_NewObject(cx, nullptr));
    JS::RootedValue viewPosVal(cx, JS::ObjectOrNullValue(NewCoreVector3f(cx, viewPos)));
    if (!JS_SetProperty(cx, ev, "viewPos", viewPosVal)) {
      __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not set ev.viewPos\n");
      JS_ReportError(cx, "Could not set ev.viewPos");
      return CenterEyeViewMatrix;
    }
    JS::RootedValue viewFwdVal(cx, JS::ObjectOrNullValue(NewCoreVector3f(cx, viewFwd)));
    if (!JS_SetProperty(cx, ev, "viewFwd", viewFwdVal)) {
      __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not set ev.viewFwd\n");
      JS_ReportError(cx, "Could not set ev.viewFwd");
      return CenterEyeViewMatrix;
    }
    double now = vrapi_GetTimeInSeconds();
    JS::RootedValue nowVal(cx, JS::NumberValue(now));
    if (!JS_SetProperty(cx, ev, "now", nowVal)) {
      __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not set ev.now\n");
      JS_ReportError(cx, "Could not set ev.now");
      return CenterEyeViewMatrix;
    }
    JS::RootedValue evValue(cx, JS::ObjectOrNullValue(ev));

    scene->ComputeMatrices(cx);
    scene->CallFrameCallbacks(cx, evValue);
    scene->CallGazeCallbacks(cx, GuiSys, viewPos, viewFwd, vrFrame, evValue);
    scene->PerformCollisionDetection(cx, now, evValue);
  }

  // Update GUI systems last, but before rendering anything.
  GuiSys->Frame(vrFrame, CenterEyeViewMatrix);

  return CenterEyeViewMatrix;
}

OVR::Matrix4f OvrApp::DrawEyeView(const int eye, const float fovDegreesX, const float fovDegreesY, ovrFrameParms& frameParms) {
  const OVR::Matrix4f eyeViewMatrix = vrapi_GetEyeViewMatrix(&app->GetHeadModelParms(), &CenterEyeViewMatrix, eye);
  const OVR::Matrix4f eyeProjectionMatrix = ovrMatrix4f_CreateProjectionFov(fovDegreesX, fovDegreesY, 0.0f, 0.0f, VRAPI_ZNEAR, 0.0f);
  const OVR::Matrix4f eyeViewProjection = eyeProjectionMatrix * eyeViewMatrix;

  // Call our scene's DrawEyeView
  JSContext* cx = SpidermonkeyJSContext;
  JS::RootedObject global(cx, SpidermonkeyGlobal.ref());
  {
    JSAutoCompartment ac(cx, global);
    scene->DrawEyeView(cx, GuiSys, eye, eyeViewMatrix, eyeProjectionMatrix, eyeViewProjection, frameParms);
  }

  GuiSys->RenderEyeView( CenterEyeViewMatrix, eyeViewMatrix, eyeProjectionMatrix );

  return eyeViewProjection;
}

extern "C" {

jlong Java_oculus_MainActivity_nativeSetAppInterface(JNIEnv* jni, jclass clazz, jobject activity,
  jstring fromPackageName, jstring commandString, jstring uriString, jobject mgr) {
  // This is called by the java UI thread.
  LOG("nativeSetAppInterface");
  AAssetManager *assetManager = AAssetManager_fromJava(jni, mgr);
  OvrApp* app = new OvrApp(assetManager);
  return app->SetActivity(jni, clazz, activity, fromPackageName, commandString, uriString);
}

} // extern "C"
