/************************************************************************************

Filename	:	VrCubeWorld_Framework.cpp
Content		:	This sample uses the application framework.
Created		:	March, 2015
Authors		:	J.M.P. van Waveren

Copyright	:	Copyright 2015 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

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
#include <mozilla/Maybe.h>
#include "CoreProgram.h"
#include "CoreVector3f.h"
#include "CoreVector4f.h"
#include "CoreMatrix4f.h"
#include "CoreGeometry.h"
#include "CoreModel.h"
#include "CoreScene.h"
#include "SceneGraph.h"

#if 0
	#define GL( func )		func; EglCheckErrors();
#else
	#define GL( func )		func;
#endif

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
	mozilla::Maybe<JS::PersistentRootedValue> RunloopCallback;
	float				RandomFloat();
	CoreScene* coreScene;
};

// Build global JS object
static JSClass globalClass = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    // [SpiderMonkey 38] Following Stubs are removed. Remove those lines.
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
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

void reportError(JSContext *cx, const char *message, JSErrorReport *report) {
	__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "%s:%u:%s\n",
			report->filename ? report->filename : "[no filename]",
            (unsigned int) report->lineno,
            message);
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

	JS_SetErrorReporter(cx, reportError);

	// Create the global JS object
	JS::RootedObject global(cx, JS::PersistentRootedObject(cx, JS_NewGlobalObject(cx, &globalClass, nullptr, JS::FireOnNewGlobalHook)));

	JS::RootedValue rval(cx);
	{
		JSAutoCompartment ac(cx, global);
		JS_InitStandardClasses(cx, global);

		// Compile and execute the script that should export vrmain
		int lineno = 1;
		bool ok = JS_EvaluateScript(cx, global, (const char*)AAsset_getBuffer(asset), AAsset_getLength(asset), filename, lineno, &rval);
		AAsset_close(asset);
		if (!ok) {
			return;
		}

		// Build the environment to send to vrmain
		JS::RootedObject core(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
		JS::RootedValue coreValue(cx, JS::ObjectOrNullValue(core));
		SetupCoreProgram(cx, &global, &core);
		SetupCoreVector3f(cx, &global, &core);
		SetupCoreVector4f(cx, &global, &core);
		SetupCoreMatrix4f(cx, &global, &core);
		SetupCoreGeometry(cx, &global, &core);
		SetupCoreModel(cx, &global, &core);
		JS::RootedObject env(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
		coreScene = SetupCoreScene(cx, &global, &core, &env);
		if (!JS_SetProperty(cx, env, "core", coreValue)) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core\n");
			return;
		}
		JS::RootedValue envValue(cx, JS::ObjectOrNullValue(env));

		// Call vrmain from script
		ok = JS_CallFunctionName(cx, global, "vrmain", JS::HandleValueArray(envValue), &rval);
		if (!ok) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not call vrmain\n");
			return;
		}

		// Check that the return type is an object (functions are objects too)
		if (!rval.isObject()) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Got invalid response from vrmain\n");
			return;
		}


		// Now make sure that it's actually a function
		JS::RootedObject func(cx, &rval.toObject());
		if (!JS_ObjectIsFunction(cx, func)) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Got non-function object response from vrmain\n");
			return;
		}

		RunloopCallback.destroyIfConstructed();
		RunloopCallback.construct(SpidermonkeyJSRuntime, JS::PersistentRootedValue(cx, rval));

		SpidermonkeyGlobal.destroyIfConstructed();
		SpidermonkeyGlobal.construct(SpidermonkeyJSRuntime, global);
	}

	__android_log_print(ANDROID_LOG_VERBOSE, LOG_COMPONENT, "Done setting up\n");
}

void VrCubeWorld::OneTimeShutdown()
{
	RunloopCallback.destroyIfConstructed();
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

	OVR::Vector3f* viewPos = new OVR::Vector3f( GetViewMatrixPosition( CenterEyeViewMatrix ) );
	OVR::Vector3f* viewFwd = new OVR::Vector3f( GetViewMatrixForward( CenterEyeViewMatrix ) );

	// Call the function returned from vrmain
	JSContext *cx = SpidermonkeyJSContext;

	JS::RootedObject global(cx, SpidermonkeyGlobal.ref());
	{
		JSAutoCompartment ac(cx, global);

		// Build the env
		JS::RootedObject env(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
		JS::RootedValue viewPosVal(cx, JS::ObjectOrNullValue(NewCoreVector3f(cx, viewPos)));
		if (!JS_SetProperty(cx, env, "viewPos", viewPosVal)) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not set env.viewPos\n");
			JS_ReportError(cx, "Could not set env.viewPos");
			return CenterEyeViewMatrix;
		}
		JS::RootedValue viewFwdVal(cx, JS::ObjectOrNullValue(NewCoreVector3f(cx, viewFwd)));
		if (!JS_SetProperty(cx, env, "viewFwd", viewFwdVal)) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not set env.viewFwd\n");
			JS_ReportError(cx, "Could not set env.viewFwd");
			return CenterEyeViewMatrix;
		}
		JS::RootedValue envValue(cx, JS::ObjectOrNullValue(env));

		// Call the frame callback
		JS::RootedValue callback(cx, RunloopCallback.ref());
		JS::RootedValue rval(cx);
		if (!JS_CallFunctionValue(cx, global, callback, JS::HandleValueArray(envValue), &rval)) {
			__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not call frame callback\n");
		}
	}

	// Call hover and collision callbacks
	for (int i = 0; i < coreScene->graph->count(); ++i) {
		Model* model = coreScene->graph->at(i);
		if (model == NULL) {
			continue;
		}
		if (model->onHoverOver.empty() && model->onHoverOut.empty()) {
			continue;
		}

		// Set up model matrix
		OVR::Matrix4f matrix = (
				OVR::Matrix4f::RotationX(model->rotation->x) *
				OVR::Matrix4f::RotationY(model->rotation->y) *
				OVR::Matrix4f::RotationZ(model->rotation->z)
		) * OVR::Matrix4f::Scaling(*(model->scale));
		matrix.SetTranslation(*(model->position));

		// Check for an intersection of one of the triangles of the model
		bool foundIntersection = false;
		OVR::Array<OVR::Vector3f> vertices = model->geometry->vertices->position;
		for (int j = 0; j < model->geometry->indices.GetSizeI(); j += 3) {
			OVR::Vector3f v0 = matrix.Transform(vertices[model->geometry->indices[j]]);
			OVR::Vector3f v1 = matrix.Transform(vertices[model->geometry->indices[j + 1]]);
			OVR::Vector3f v2 = matrix.Transform(vertices[model->geometry->indices[j + 2]]);
			float t0, u, v;
			if (OVR::Intersect_RayTriangle(*viewPos, *viewFwd, v0, v1, v2, t0, u, v)) {
				foundIntersection = true;
				break;
			}
		}

		// If we found an intersection
		if (foundIntersection) {
			// If we already knew about it, move on to the next
			if (model->isHovered) {
				continue;
			}
			// Otherwise, mark that we've seen the intersection
			model->isHovered = true;
			// If we have no callback, bail early
			if (model->onHoverOver.empty()) {
				continue;
			}
			// Call the onHoverOver callback
			{
				JSAutoCompartment ac(cx, global);
				JS::RootedValue callback(cx, model->onHoverOver.ref());
				JS::RootedValue rval(cx);
				// TODO: Construct an object (with t0, u, v ?) to pass in
				if (!JS_CallFunctionValue(cx, global, callback, JS::HandleValueArray::empty(), &rval)) {
					__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not call onHoverOver callback\n");
				}
			}
		} else {
			// If we found no intersection and we weren't hovering on it before, move on
			if (!model->isHovered) {
				continue;
			}
			// Otherwise, we've just hovered off of it, so mark that
			model->isHovered = false;
			// If we have no callback, bail early
			if (model->onHoverOut.empty()) {
				continue;
			}
			// Call the onHoverOut callback
			{
				JSAutoCompartment ac(cx, global);
				JS::RootedValue callback(cx, model->onHoverOut.ref());
				JS::RootedValue rval(cx);
				if (!JS_CallFunctionValue(cx, global, callback, JS::HandleValueArray::empty(), &rval)) {
					__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not call onHoverOut callback\n");
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

	GL( glClearColor(
		coreScene->clearColor->x,
		coreScene->clearColor->y,
		coreScene->clearColor->z,
		coreScene->clearColor->w
	) );
	GL( glClear( GL_COLOR_BUFFER_BIT ) );

	for (int i = 0; i < coreScene->graph->count(); ++i) {
		Model* model = coreScene->graph->at(i);
		if (model == NULL) {
			continue;
		}

		// Set up model matrix
		OVR::Matrix4f matrix = (
				OVR::Matrix4f::RotationX(model->rotation->x) *
				OVR::Matrix4f::RotationY(model->rotation->y) *
				OVR::Matrix4f::RotationZ(model->rotation->z)
		) * OVR::Matrix4f::Scaling(*(model->scale));
		matrix.SetTranslation(*(model->position));

		// Now submit the draw calls
		GL( glUseProgram( model->program->program ) );
		GL( glUniformMatrix4fv( model->program->uModel, 1, GL_TRUE, matrix.M[0] ) );
		GL( glUniformMatrix4fv( model->program->uView, 1, GL_TRUE, eyeViewMatrix.M[0] ) );
		GL( glUniformMatrix4fv( model->program->uProjection, 1, GL_TRUE, eyeProjectionMatrix.M[0] ) );
		GL( glBindVertexArray( model->geometry->geometry->vertexArrayObject ) );
		GL( glDrawElements( GL_TRIANGLES, model->geometry->geometry->indexCount, GL_UNSIGNED_SHORT, NULL ) );
	}

	GL( glBindVertexArray( 0 ) );
	GL( glUseProgram( 0 ) );

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
