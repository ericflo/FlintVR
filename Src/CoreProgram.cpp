#include "CoreProgram.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

static JSClass coreProgramClass = {
	"Program",              /* name */
	JSCLASS_HAS_PRIVATE    /* flags */
};

OVR::GlProgram* GetProgram(JS::HandleObject obj) {
	OVR::GlProgram* program = (OVR::GlProgram*)JS_GetPrivate(obj);
	return program;
}

bool CoreProgram_constructor(JSContext *cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	// Check the arguments length
	if (args.length() < 2) {
		JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 2);
		return false;
	}

	// Check to make sure the args are strings
	if (!args[0].isString() || !args[1].isString()) {
		JS_ReportError(cx, "Vertex and fragment shaders must be strings");
		return false;
	}

	// Grab rooted copies of the vertex and fragment shader strings
	JS::RootedString vertex(cx, JS::ToString(cx, args[0]));
	JS::RootedString fragment(cx, JS::ToString(cx, args[1]));

	// Go ahead and create our self object and attach the vertex and fragment shaders passed by the user
	JS::RootedObject self(cx, JS_NewObject(cx, &coreProgramClass));
	if (!JS_SetProperty(cx, self, "vertex", args[0])) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not set program.vertex\n");
		JS_ReportError(cx, "Could not set program.vertex");
		return false;
	}
	if (!JS_SetProperty(cx, self, "fragment", args[1])) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not set program.fragment\n");
		JS_ReportError(cx, "Could not set program.fragment");
		return false;
	}

	// Copy the vertex and fragment shaders into C string buffers whose memory we control
	size_t vertexLen = JS_GetStringEncodingLength(cx, vertex) * sizeof(char);
	char* vertexBuf = new char[vertexLen+1];
	size_t vertexCopiedLen = JS_EncodeStringToBuffer(cx, vertex, vertexBuf, vertexLen);
	if (vertexCopiedLen != vertexLen) {
		delete[] vertexBuf;
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not encode vertex string to program buffer\n");
		JS_ReportError(cx, "Could not encode vertex string to program buffer");
		return false;
	}
	vertexBuf[vertexLen] = '\0';

	size_t fragmentLen = JS_GetStringEncodingLength(cx, fragment) * sizeof(char);
	char* fragmentBuf = new char[fragmentLen+1];
	size_t fragmentCopiedLen = JS_EncodeStringToBuffer(cx, fragment, fragmentBuf, fragmentLen);
	if (fragmentCopiedLen != fragmentLen) {
		delete[] vertexBuf;
		delete[] fragmentBuf;
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not encode fragment string to program buffer\n");
		JS_ReportError(cx, "Could not encode fragment string to program buffer");
		return false;
	}
	fragmentBuf[fragmentLen] = '\0';

	// Create the actual program from these C string buffers
	OVR::GlProgram program = OVR::BuildProgram(vertexBuf, fragmentBuf);

	// Now we can free the buffers
	delete[] vertexBuf;
	delete[] fragmentBuf;

	// Make a copy of the GlProgram on the heap so we can put in private storage
	OVR::GlProgram* heapProgram = new OVR::GlProgram();
	heapProgram->program = program.program;
	heapProgram->vertexShader = program.vertexShader;
	heapProgram->uMvp = program.uMvp;
	heapProgram->uModel = program.uModel;
	heapProgram->uView = program.uView;
	heapProgram->uProjection = program.uProjection;
	heapProgram->uColor = program.uColor;
	heapProgram->uFadeDirection = program.uFadeDirection;
	heapProgram->uTexm = program.uTexm;
	heapProgram->uTexm2 = program.uTexm2;
	heapProgram->uJoints = program.uJoints;
	heapProgram->uColorTableOffset = program.uColorTableOffset;

	// Put the heap program in private storage
	JS_SetPrivate(self, (void *)heapProgram);

	// Return our self object
	args.rval().set(JS::ObjectOrNullValue(self));
	return true;
}

void CoreProgram_finalize(JSFreeOp *fop, JSObject *obj) {
	OVR::GlProgram* program = (OVR::GlProgram*)JS_GetPrivate(obj);
	JS_SetPrivate(obj, NULL);
	delete program;
}

void SetupCoreProgram(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core) {
	coreProgramClass.finalize = CoreProgram_finalize;
	JSObject *obj = JS_InitClass(
			cx,
			*core,
			*global,
			&coreProgramClass,
			CoreProgram_constructor,
			2,
			nullptr, /* Properties */
			nullptr, /* Methods */
			nullptr, /* Static Props */
			nullptr /* Static Methods */);
	if (!obj) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not construct env.core.Program class\n");
		return;
	}
}
