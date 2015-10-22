#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "App.h"
#include "GuiSys.h"
#include "OVR_Locale.h"
#include "Kernel/OVR_Geometry.h"
#include "SoundEffectContext.h"
#include <memory>
#include "jsapi.h"
#include "js/Conversions.h"
#include <mozilla/Maybe.h>
#include "CoreProgram.h"
#include "CoreVector3f.h"
#include "CoreVector4f.h"
#include "CoreMatrix4f.h"
#include "CoreGeometry.h"
#include "CoreModel.h"
#include "CoreScene.h"
#include "SceneGraph.h"
#include "bullet/btBulletCollisionCommon.h"

inline int bullet_btInfinityMask(){ return btInfinityMask; } // Hack to work around bullet bug

#if 0
	#define GL( func )		func; EglCheckErrors();
#else
	#define GL( func )		func;
#endif

#define ERROR_DISPLAY_SECONDS 10

#define LOG_COMPONENT "VrCubeWorld"

/*
================================================================================

VrCubeWorld

================================================================================
*/

namespace OVR
{

static const int CPU_LEVEL			= 1;
static const int GPU_LEVEL			= 1;

OVR::String PREVIOUS_ERROR;
OVR::String LATEST_ERROR;

void reportError(JSContext *cx, const char *message, JSErrorReport *report) {
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

class VrCubeWorld : public VrAppInterface
{
public:
						VrCubeWorld(AAssetManager *assetManager);
						~VrCubeWorld();

	virtual void 		Configure( ovrSettings & settings );

	virtual void		OneTimeInit( const char * fromPackage, const char * launchIntentJSON, const char * launchIntentURI );
	virtual void		OneTimeShutdown();
	virtual bool		OnKeyEvent( const int keyCode, const int repeatCount, const KeyEventType eventType );
	virtual Matrix4f	Frame( const VrFrame & vrFrame );
	virtual Matrix4f	DrawEyeView( const int eye, const float fovDegreesX, const float fovDegreesY, ovrFrameParms & frameParms );

	ovrLocale &			GetLocale() { return *Locale; }

private:
	std::unique_ptr<ovrSoundEffectContext> SoundEffectContext;
	std::unique_ptr<OvrGuiSys::SoundEffectPlayer> SoundEffectPlayer;
	OvrGuiSys *			   GuiSys;
	ovrLocale *			   Locale;
	unsigned int		   Random;
	ovrMatrix4f			   CenterEyeViewMatrix;
	AAssetManager 		   *AssetManager;
	JSRuntime * 		   SpidermonkeyJSRuntime;
	JSContext *            SpidermonkeyJSContext;
	mozilla::Maybe<JS::PersistentRootedObject> SpidermonkeyGlobal;
	float				RandomFloat();
	CoreScene* coreScene;
	mozilla::Maybe<JS::CompileOptions> CompileOptions;
};

// Build global JS object
static JSClass globalClass = {
    "global",
    JSCLASS_GLOBAL_FLAGS
};

VrCubeWorld::VrCubeWorld(AAssetManager *assetManager) :
	GuiSys( OvrGuiSys::Create() ),
	Locale( NULL ),
	Random( 2 )
{
	CenterEyeViewMatrix = ovrMatrix4f_CreateIdentity();
	AssetManager = assetManager;
}

VrCubeWorld::~VrCubeWorld()
{
	OvrGuiSys::Destroy( GuiSys );
}


float VrCubeWorld::RandomFloat()
{
	Random = 1664525L * Random + 1013904223L;
	unsigned int rf = 0x3F800000 | ( Random & 0x007FFFFF );
	return (*(float *)&rf) - 1.0f;
}

void VrCubeWorld::OneTimeInit( const char * fromPackageName, const char * launchIntentJSON, const char * launchIntentURI )
{
	auto java = app->GetJava();
	SoundEffectContext.reset( new ovrSoundEffectContext( *java->Env, java->ActivityObject ) );
	SoundEffectContext->Initialize();
	SoundEffectPlayer.reset( new OvrGuiSys::ovrDummySoundEffectPlayer() );

	Locale = ovrLocale::Create( *app, "default" );

	String fontName;
	GetLocale().GetString( "@string/font_name", "efigs.fnt", fontName );
	GuiSys->Init( this->app, *SoundEffectPlayer, fontName.ToCStr(), &app->GetDebugLines() );

	app->SetShowFPS(true);

	// Load the hello.js script into memory
	const char* filename = "hello.js";
	AAsset* asset = AAssetManager_open(AssetManager, filename, AASSET_MODE_BUFFER);
	if (NULL == asset) {
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_COMPONENT, "ASSET NOT FOUND: %s", filename);
		return;
	}

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
	JSContext *cx = SpidermonkeyJSContext;

	JS_SetErrorReporter(SpidermonkeyJSRuntime, reportError);

	// Create the global JS object
	JS::RootedObject global(cx, JS::PersistentRootedObject(cx, JS_NewGlobalObject(cx, &globalClass, nullptr, JS::FireOnNewGlobalHook)));

	JS::RootedValue rval(cx);
	{
		JSAutoCompartment ac(cx, global);
		JS_InitStandardClasses(cx, global);

		// Compile and execute the script that should export vrmain
		CompileOptions.emplace(cx);
		bool ok = JS::Evaluate(cx, CompileOptions.ref(), (const char*)AAsset_getBuffer(asset), AAsset_getLength(asset), &rval);
		AAsset_close(asset);
		if (!ok) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not evaluate script");
			app->ShowInfoText(ERROR_DISPLAY_SECONDS, "Could not evaluate script");
		}

		// Build the environment to send to vrmain
		JS::RootedObject core(cx, JS_NewObject(cx, nullptr));
		JS::RootedValue coreValue(cx, JS::ObjectOrNullValue(core));
		SetupCoreProgram(cx, &global, &core);
		SetupCoreVector3f(cx, &global, &core);
		SetupCoreVector4f(cx, &global, &core);
		SetupCoreMatrix4f(cx, &global, &core);
		SetupCoreGeometry(cx, &global, &core);
		SetupCoreModel(cx, &global, &core);
		JS::RootedObject env(cx, JS_NewObject(cx, nullptr));
		coreScene = SetupCoreScene(cx, &global, &core, &env);
		if (!JS_SetProperty(cx, env, "core", coreValue)) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core\n");
			return;
		}
		JS::RootedValue envValue(cx, JS::ObjectOrNullValue(env));

		// Call vrmain from script
		if (ok) {
			ok = JS_CallFunctionName(cx, global, "vrmain", JS::HandleValueArray(envValue), &rval);
			if (!ok) {
				__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not call vrmain\n");
			}
		}

		SpidermonkeyGlobal.reset();
		SpidermonkeyGlobal.emplace(cx, global);
	}
}

void VrCubeWorld::OneTimeShutdown()
{
	JS_DestroyContext(SpidermonkeyJSContext);
	JS_DestroyRuntime(SpidermonkeyJSRuntime);
	JS_ShutDown();
}

void VrCubeWorld::Configure( ovrSettings & settings )
{
	settings.PerformanceParms.CpuLevel = CPU_LEVEL;
	settings.PerformanceParms.GpuLevel = GPU_LEVEL;
	settings.EyeBufferParms.multisamples = 2;
}

bool VrCubeWorld::OnKeyEvent( const int keyCode, const int repeatCount, const KeyEventType eventType )
{
	if ( GuiSys->OnKeyEvent( keyCode, repeatCount, eventType ) )
	{
		return true;
	}
	return false;
}

Matrix4f VrCubeWorld::Frame( const VrFrame & vrFrame )
{
	CenterEyeViewMatrix = vrapi_GetCenterEyeViewMatrix( &app->GetHeadModelParms(), &vrFrame.Tracking, NULL );

	// Show any errors
	if (!LATEST_ERROR.IsEmpty()) {
		app->ShowInfoText(ERROR_DISPLAY_SECONDS, "%s", LATEST_ERROR.ToCStr());
		LATEST_ERROR.Clear();
	}

	// Call the function returned from vrmain
	JSContext *cx = SpidermonkeyJSContext;

	JS::RootedObject global(cx, SpidermonkeyGlobal.ref());
	{
		JSAutoCompartment ac(cx, global);

		OVR::Vector3f* viewPos = new OVR::Vector3f( GetViewMatrixPosition( CenterEyeViewMatrix ) );
		OVR::Vector3f* viewFwd = new OVR::Vector3f( GetViewMatrixForward( CenterEyeViewMatrix ) );

		bool touchPressed = ( vrFrame.Input.buttonPressed & ( OVR::BUTTON_TOUCH | OVR::BUTTON_A ) ) != 0;
		bool touchReleased = !touchPressed && ( vrFrame.Input.buttonReleased & ( OVR::BUTTON_TOUCH | OVR::BUTTON_A ) ) != 0;
		bool touchDown = ( vrFrame.Input.buttonState & BUTTON_TOUCH ) != 0;

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
		JS::RootedValue nowVal(cx, JS::NumberValue(vrapi_GetTimeInSeconds()));
		if (!JS_SetProperty(cx, ev, "now", nowVal)) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not set ev.now\n");
			JS_ReportError(cx, "Could not set ev.now");
			return CenterEyeViewMatrix;
		}
		JS::RootedValue evValue(cx, JS::ObjectOrNullValue(ev));


		// We're gonna have some rvals going on in here, guess we'll make one for reference
		JS::RootedValue rval(cx);

		// Compute each model's transform matrix
		for (int i = 0; i < coreScene->graph->count(); ++i) {
			CoreModel* model = coreScene->graph->at(i);
			model->ComputeMatrix(cx);
		}

		// Call the frame callbacks
		for (int i = 0; i < coreScene->graph->count(); ++i) {
			CoreModel* model = coreScene->graph->at(i);
			if (!model->HasFrameCallback()) {
				continue;
			}
			JS::RootedObject modelSelf(cx, &model->selfVal->toObject());

			JS::RootedValue callback(cx, model->onFrameVal.ref());
			if (!JS_CallFunctionValue(cx, modelSelf, callback, JS::HandleValueArray(evValue), &rval)) {
				__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not call onFrame callback\n");
			}
		}

		// Call hover and collision callbacks
		for (int i = 0; i < coreScene->graph->count(); ++i) {
			CoreModel* model = coreScene->graph->at(i);
			if (!model->HasGazeCallback() && !model->HasGestureCallback()) {
				continue;
			}
			JS::RootedObject modelSelf(cx, &model->selfVal->toObject());

			// Check for an intersection of one of the triangles of the model
			bool foundIntersection = false;
			OVR::Array<OVR::Vector3f> vertices = model->geometry(cx)->vertices->position;
			for (int j = 0; j < model->geometry(cx)->indices.GetSizeI(); j += 3) {
				OVR::Vector3f v0 = model->computedMatrix->Transform(vertices[model->geometry(cx)->indices[j]]);
				OVR::Vector3f v1 = model->computedMatrix->Transform(vertices[model->geometry(cx)->indices[j + 1]]);
				OVR::Vector3f v2 = model->computedMatrix->Transform(vertices[model->geometry(cx)->indices[j + 2]]);
				float t0, u, v;
				if (OVR::Intersect_RayTriangle(*viewPos, *viewFwd, v0, v1, v2, t0, u, v)) {
					foundIntersection = true;
					break;
				}
				// Check the backface (is this necessary?)
				if (OVR::Intersect_RayTriangle(*viewPos, *viewFwd, v2, v1, v0, t0, u, v)) {
					foundIntersection = true;
					break;
				}
			}

			JS::RootedValue callback(cx);

			// If we found an intersection
			if (foundIntersection) {

				if (!model->isHovered) {
					model->isHovered = true;
					// Call the onGazeHoverOver callback
					if (CallbackDefined(model->onGazeHoverOverVal)) {
						callback = JS::RootedValue(cx, model->onGazeHoverOverVal.ref());
						// TODO: Construct an object (with t0, u, v ?) to add to env
						if (!JS_CallFunctionValue(cx, modelSelf, callback, JS::HandleValueArray(evValue), &rval)) {
							JS_ReportError(cx, "Could not call onGazeHoverOver callback");
						}
					}
				}

				if (!model->isTouching && touchPressed) {
					model->isTouching = true;
					// TODO: Construct an object (with t0, u, v ?) to add to env
					if (CallbackDefined(model->onGestureTouchDownVal)) {
						callback = JS::RootedValue(cx, model->onGestureTouchDownVal.ref());
						if (!JS_CallFunctionValue(cx, modelSelf, callback, JS::HandleValueArray(evValue), &rval)) {
							JS_ReportError(cx, "Could not call onGestureTouchDown callback");
						}
					}
				}

				if ((model->isTouching && touchReleased) || (model->isTouching && !touchDown)) {
					model->isTouching = false;
					// TODO: Construct an object (with t0, u, v ?) to add to env
					if (CallbackDefined(model->onGestureTouchUpVal)) {
						callback = JS::RootedValue(cx, model->onGestureTouchUpVal.ref());
						if (!JS_CallFunctionValue(cx, modelSelf, callback, JS::HandleValueArray(evValue), &rval)) {
							JS_ReportError(cx, "Could not call onGestureTouchUp callback");
						}
					}
				}

			} else {
				if (model->isTouching) {
					model->isTouching = false;
					if (CallbackDefined(model->onGestureTouchCancelVal)) {
						callback = JS::RootedValue(cx, model->onGestureTouchCancelVal.ref());
						if (!JS_CallFunctionValue(cx, modelSelf, callback, JS::HandleValueArray(evValue), &rval)) {
							JS_ReportError(cx, "Could not call onGestureTouchCancel callback");
						}
					}
				}

				if (model->isHovered) {
					model->isHovered = false;
					if (CallbackDefined(model->onGazeHoverOutVal)) {
						callback = JS::RootedValue(cx, model->onGazeHoverOutVal.ref());
						if (!JS_CallFunctionValue(cx, modelSelf, callback, JS::HandleValueArray(evValue), &rval)) {
							JS_ReportError(cx, "Could not call onGazeHoverOut callback");
						}
					}
				}
			}
		}
	}

	// Update GUI systems last, but before rendering anything.
	GuiSys->Frame( vrFrame, CenterEyeViewMatrix );

	return CenterEyeViewMatrix;
}

Matrix4f VrCubeWorld::DrawEyeView( const int eye, const float fovDegreesX, const float fovDegreesY, ovrFrameParms & frameParms )
{
	const Matrix4f eyeViewMatrix = vrapi_GetEyeViewMatrix( &app->GetHeadModelParms(), &CenterEyeViewMatrix, eye );
	const Matrix4f eyeProjectionMatrix = ovrMatrix4f_CreateProjectionFov( fovDegreesX, fovDegreesY, 0.0f, 0.0f, 0.2f, 0.0f );
	const Matrix4f eyeViewProjection = eyeProjectionMatrix * eyeViewMatrix;

	// Call the function returned from vrmain
	JSContext *cx = SpidermonkeyJSContext;

	JS::RootedObject global(cx, SpidermonkeyGlobal.ref());
	{
		JSAutoCompartment ac(cx, global);

		GL( glClearColor(
			coreScene->clearColor->x,
			coreScene->clearColor->y,
			coreScene->clearColor->z,
			coreScene->clearColor->w
		) );
		GL( glClear( GL_COLOR_BUFFER_BIT ) );

		for (int i = 0; i < coreScene->graph->count(); ++i) {
			CoreModel* model = coreScene->graph->at(i);
			if (model == NULL) {
				continue;
			}

			// Now submit the draw calls
			GL( glUseProgram( model->program(cx)->program ) );
			GL( glUniformMatrix4fv( model->program(cx)->uModel, 1, GL_TRUE, model->computedMatrix->M[0] ) );
			GL( glUniformMatrix4fv( model->program(cx)->uView, 1, GL_TRUE, eyeViewMatrix.M[0] ) );
			GL( glUniformMatrix4fv( model->program(cx)->uProjection, 1, GL_TRUE, eyeProjectionMatrix.M[0] ) );
			GL( glBindVertexArray( model->geometry(cx)->geometry->vertexArrayObject ) );
			GL( glDrawElements( GL_TRIANGLES, model->geometry(cx)->geometry->indexCount, GL_UNSIGNED_SHORT, NULL ) );
		}

		GL( glBindVertexArray( 0 ) );
		GL( glUseProgram( 0 ) );
	}

	GuiSys->RenderEyeView( CenterEyeViewMatrix, eyeViewProjection );

	return eyeViewProjection;
}

} // namespace OVR

extern "C"
{

long Java_com_oculus_vrcubeworld_MainActivity_nativeSetAppInterface( JNIEnv *jni, jclass clazz, jobject activity,
	jstring fromPackageName, jstring commandString, jstring uriString, jobject mgr )
{
	// This is called by the java UI thread.
	LOG( "nativeSetAppInterface" );
	AAssetManager *assetManager = AAssetManager_fromJava(jni, mgr);
	OVR::VrCubeWorld* cubeWorld = new OVR::VrCubeWorld(assetManager);
	return cubeWorld->SetActivity( jni, clazz, activity, fromPackageName, commandString, uriString );
}

} // extern "C"
