#include "CoreScene.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

CoreScene::CoreScene(void) {
	graph = new SceneGraph();

	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new	btCollisionDispatcher(collisionConfiguration);
	overlappingPairCache = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver;
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache,
		solver, collisionConfiguration);
}

CoreScene::~CoreScene(void) {
	clearColorVal.reset();
	delete graph;

	delete dynamicsWorld;
	delete solver;
	delete overlappingPairCache;
	delete dispatcher;
	delete collisionConfiguration;
}

OVR::Vector4f* VRJS_MEMBER(CoreScene, clearColor, GetVector4f);

VRJS_GETSET(CoreScene, clearColor)

static JSPropertySpec CoreScene_props[] = {
  VRJS_PROP(CoreScene, clearColor),
  JS_PS_END
};

static JSClass coreSceneClass = {
	"Scene",                /* name */
	JSCLASS_HAS_PRIVATE    /* flags */
};

JSObject* NewCoreScene(JSContext *cx, CoreScene* scene) {
	JS::RootedObject self(cx, JS_NewObject(cx, &coreSceneClass));
	
	JS_SetPrivate(self, (void *)scene);

	if (!JS_DefineProperties(cx, self, CoreScene_props)) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not define properties on scene\n");
  }
	
	if (!JS_DefineFunction(cx, self, "add", &CoreScene_add, 0, 0)) {
		JS_ReportError(cx, "Could not create scene.add function");
		return NULL;
	}
	if (!JS_DefineFunction(cx, self, "remove", &CoreScene_remove, 0, 0)) {
		JS_ReportError(cx, "Could not create scene.remove function");
		return NULL;
	}
	if (!JS_DefineFunction(cx, self, "setClearColor", &CoreScene_setClearColor, 0, 0)) {
		JS_ReportError(cx, "Could not create scene.setClearColor function");
		return NULL;
	}

	JS::RootedObject objects(cx, JS_NewArrayObject(cx, 0));
	JS::RootedValue objectsVal(cx, JS::ObjectOrNullValue(objects));
	if (!JS_SetProperty(cx, self, "objects", objectsVal)) {
		JS_ReportError(cx, "Could not set scene.objects");
		return NULL;
	}

	return self;
}

CoreScene* GetCoreScene(JS::HandleObject obj) {
	CoreScene* scene = (CoreScene*)JS_GetPrivate(obj);
	return scene;
}

bool CoreScene_constructor(JSContext *cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	JS::RootedObject self(cx, NewCoreScene(cx, new CoreScene()));	

	args.rval().set(JS::ObjectOrNullValue(self));
	return true;
}

void CoreScene_finalize(JSFreeOp *fop, JSObject *obj) {
	CoreScene* scene = (CoreScene*)JS_GetPrivate(obj);
	JS_SetPrivate(obj, NULL);
	delete scene;
}

bool CoreScene_add(JSContext *cx, unsigned argc, JS::Value *vp) {
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

	// Read the model object in
	JS::RootedObject model(cx, &args[0].toObject());
	JS::RootedObject thisObj(cx, &args.thisv().toObject());
	CoreScene* scene = (CoreScene*)JS_GetPrivate(thisObj);
	scene->graph->add(GetCoreModel(model));

	// Push the geometry object to our objects array, first by getting the objects object
	JS::RootedValue objectsVal(cx);
	if (!JS_GetProperty(cx, thisObj, "objects", &objectsVal)) {
		JS_ReportError(cx, "Could not get scene.objects");
		return false;
	}
	JS::RootedObject objects(cx, &objectsVal.toObject());

	// Then getting the "push" method from it
	JS::RootedValue pushFuncVal(cx);
	if (!JS_GetProperty(cx, objects, "push", &pushFuncVal)) {
		JS_ReportError(cx, "Could not get scene.push");
		return false;
	}
	JS::RootedObject pushFuncObj(cx, &pushFuncVal.toObject());

	// Check to make sure we've found a function object
	if (!JS_ObjectIsFunction(cx, pushFuncObj)) {
		JS_ReportError(cx, "scene.objects.push isn't a function for some reason");
		return false;
	}

	// Construct our args to pass to push
	JS::AutoValueArray<1> pushArgs(cx);
	pushArgs[0].set(args[0]);
	JS::RootedValue rval(cx);
	if (!JS_CallFunctionValue(cx, objects, pushFuncVal, pushArgs, &rval)) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not call scene.objects.push\n");
	}

	return true;
}

bool CoreScene_remove(JSContext *cx, unsigned argc, JS::Value *vp) {
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

	// Read the model object in
	JS::RootedObject model(cx, &args[0].toObject());
	JS::RootedObject thisObj(cx, &args.thisv().toObject());
	CoreScene* scene = (CoreScene*)JS_GetPrivate(thisObj);

	int idx = scene->graph->indexOf(GetCoreModel(model));
	if (idx == -1) {
		JS_ReportError(cx, "Could not find model to remove");
		return false;
	}
	scene->graph->remove(idx);

	JS::RootedValue rval(cx);

	OVR::String script;
	script.Format("this.objects.splice(%d, 1)", idx);
	const char* scriptStr = script.ToCStr();
	JS::CompileOptions compileOpts(cx);
	if (!JS::Evaluate(cx, compileOpts, scriptStr, OVR::OVR_strlen(scriptStr), &rval)) {
		JS_ReportError(cx, "Could not evaluate model remove script");
		return false;
	}

	return true;
}

bool CoreScene_setClearColor(JSContext *cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	// Check the arguments length
	if (args.length() != 1) {
		JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
		return false;
	}
	if (!args[0].isObject()) {
		JS_ReportError(cx, "Expected add to take a geometry argument");
		return false;
	}

	JS::RootedObject self(cx, &args.thisv().toObject());
	CoreScene* scene = GetCoreScene(self);

	SetMaybeValue(cx, args[0], scene->clearColorVal);

	return true;
}

bool Core_print(JSContext *cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	// Check the arguments length
	if (args.length() != 1) {
		JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
		return false;
	}
	
	// Convert the argument to a string
	JS::RootedString s(cx, JS::ToString(cx, args[0]));

	size_t sLen = JS_GetStringEncodingLength(cx, s) * sizeof(char);
	char* sBuf = new char[sLen+1];
	size_t sCopiedLen = JS_EncodeStringToBuffer(cx, s, sBuf, sLen);
	if (sCopiedLen != sLen) {
		delete[] sBuf;
		JS_ReportError(cx, "Could not encode output");
		return false;
	}
	sBuf[sLen] = '\0';

	__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "PRINT: %s\n", sBuf);

	delete[] sBuf;

	return true;
}

CoreScene* SetupCoreScene(JSContext* cx, JS::RootedObject* global, JS::RootedObject* core, JS::RootedObject* env) {
	coreSceneClass.finalize = CoreScene_finalize;
	JSObject *obj = JS_InitClass(
			cx,
			*core,
			*global,
			&coreSceneClass,
			CoreScene_constructor,
			0,
			nullptr, /* Properties */
			nullptr, /* Methods */
			nullptr, /* Static Props */
			nullptr  /* Static Methods */);
	if (!obj) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Scene class\n");
		return NULL;
	}

	// TODO: Free this somewhere, it's a singleton now but who knows later
	CoreScene* scene = new CoreScene();
	JS::RootedObject sceneObj(cx, NewCoreScene(cx, scene));
	JS::RootedValue sceneVal(cx, JS::ObjectOrNullValue(sceneObj));
	if (!JS_SetProperty(cx, *env, "scene", sceneVal)) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not set env.scene\n");
		JS_ReportError(cx, "Could not set env.scene");
		return NULL;
	}

	if (!JS_DefineFunction(cx, *global, "print", &Core_print, 1, 0)) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not create print function\n");
		JS_ReportError(cx, "Could not create print function");
		return NULL;
	}

	return scene;
}
